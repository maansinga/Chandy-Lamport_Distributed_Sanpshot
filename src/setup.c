#ifndef __setup_c__
#define __setup_c__

#include "core.h"
#include "setup_helper.h"



void init_node(Node *n){
	n->id = 0;
	n->server_fd = -1;
	n->neighbors_count = -1;
	n->neighbors = NULL;
}

void init_config(Config *conf){
	conf->N = -1;
	conf->MinPerActive = -1;
	conf->MaxPerActive = -1;
	conf->MinSendDelay = -1;
	conf->SnapshotDelay = -1;
	conf->MaxNumber = -1;

	conf->LUT = NULL;
	conf->node.id = -1;
	conf->node.server_fd = -1;
	conf->node.neighbors_count = -1;
	conf->node.neighbors = NULL;
}

int setup_config(char *filename, int id, Config *conf){
	init_config(conf);
	conf->node.id = id;

	// The file
	FILE *config_fp = fopen(filename, "r");

	// If file opening fails
	if(config_fp==NULL){
		perror(DBG "fopen()");
		return(-1);
	}

	// Counter for read lines
	int line_number = 0;
	
	// Buffer pointer for line reading
	char* line_buffer=NULL;

	// Buffered string length after reading
	size_t line_length=64;

	// Reading all the lines
	while(getline(&line_buffer, &line_length, config_fp) != -1){
		// If the line is empty in general
		if(line_length <= 0)continue;

		// Preprocess the line
		preprocess_string(line_buffer);

		// If the string is empty, continue without bothering
		if(strlen(line_buffer) == 0)continue;

		// printf("%s\n", line_buffer);

		// Get total system configuration from 0th line
		if(line_number == 0){
			sscanf(line_buffer, "%d %d %d %d %d %d",
				&conf->N,
				&conf->MinPerActive,
				&conf->MaxPerActive,
				&conf->MinSendDelay,
				&conf->SnapshotDelay,
				&conf->MaxNumber);

			conf->LUT = (NodeDesc*)malloc(sizeof(NodeDesc) * conf->N);
			conf->clock = (int*)malloc(sizeof(int) * conf->N);

			conf->flag_markers_sent = 0;
			conf->flag_snapshots_alarm_active = 0;
			conf->flag_snapshots_session_active = 0;
			conf->snapshots_session_parent_id = 0;
			conf->snapshots_session_spark = 0;
			conf->snapshot_no = 0;
			conf->snapshots_session_children_count = 0;
			conf->snapshots_session_ccast_count = 0;

			conf->snapshots_session_monitor_punch_card = (int*)malloc(sizeof(int) * conf->N);
			conf->snapshots_clock = (int*)malloc(sizeof(int) * conf->N);
			conf->snapshots_session_children = (int*)malloc(sizeof(int) * conf->N);
			conf->snapshots_accumulated_timestamps = (int*)malloc(sizeof(int) * conf->N * conf->N);

		} else if(line_number > 0 && line_number < conf->N + 1){
			// Host descriptor
			NodeDesc entry;

			// Reading entries into LUT
			sscanf(line_buffer, "%d %s %d", &entry.id, entry.hostname, &entry.port);
			conf->LUT[entry.id] = entry;

		}else if(line_number == 1 + conf->N + conf->node.id){        // Setup neighbors
			// Get neighbors!!!
			if(strlen(line_buffer) > 0){
				// printf("These are my friends: %s\n", line_buffer);

				// Count the number of neighbors -- tab and space safe!!
				int neighbors_count = 0;

				int switch_flag = 0;	// 0 - is not number 1 - is number

				for(int i = 0; line_buffer[i] != '\0'; i++){
					if(switch_flag == 0 && line_buffer[i] != ' ' && line_buffer[i] != '\t' ){
						// printf("switch_flag  %d  %c  %d\n",switch_flag, line_buffer[i], neighbors_count);
						switch_flag = 1;
						neighbors_count++;
					}else if(switch_flag == 1 && (line_buffer[i] == ' ' || line_buffer[i] == '\t' )){
						switch_flag = 0;
					}
				}

				conf->node.neighbors = (Neighbor*)malloc(sizeof(Neighbor) * neighbors_count);
				conf->node.neighbors_count = neighbors_count;

				char *tok = NULL;
				char *str = line_buffer;

				for(int i=0; ;str=NULL){
					tok = strtok(str," \t\r");
					if(tok == NULL)break;

					preprocess_string(tok);

					if(strlen(tok)>0){
						// printf("%s %d\n", tok, strlen(tok));
						int tok_i = atoi(tok);

						conf->node.neighbors[i++].id = tok_i;
					}else continue;
				}
			}else{
				printf("No neighbors found for this node!!!\n");
				exit(0);
			}
		}

		// printf("%s\n", line_buffer);
		line_number+=1;
		
		// if(line_buffer != NULL)free(line_buffer);
	}
	// Close config file
	fclose(config_fp);

 	return(0);
}

int accept_connections(Config *conf){
	Node *node = &conf->node;

	struct sockaddr_in saddr;

	node->server_fd = socket(AF_INET, SOCK_STREAM, 0);

	// Building complete hostname
	char complete_host_name[32];
	sprintf(complete_host_name, HN_TEMPLATE, conf->LUT[node->id].hostname);

	// Getting internal representation for the hostname
	struct hostent *serv;
	if ((serv = gethostbyname(complete_host_name)) == 0) {
		perror("Error on gethostbyname call");
		exit(1);
	}

	// Preparing socket for binding
	printf("%s\n",complete_host_name);
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(conf->LUT[node->id].port);
	saddr.sin_addr.s_addr = ((struct in_addr *)(serv->h_addr_list[0]))->s_addr;

	int reuse = 1;
	if (setsockopt(node->server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)
		perror("setsockopt(SO_REUSEADDR) failed");


	// Binding the socket to the node's address
	if(bind(node->server_fd, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in)) < 0){
		perror(DBG "bind");
		exit(-1);
	}

	// Listening to [neighbors_count] numbere of connections
	listen(node->server_fd, node->neighbors_count);

	// Accepted client's address datastructure
	struct sockaddr_in client_addr = {0};
	socklen_t client_addr_len;

	// As long as there are possible neighbors that can make a connection
	printf("Node#%d waiting for incoming connections\n", node->id);
	for(int i = 0; i < node->neighbors_count; i++){
		// Accept the connection
		int client_sock_fd = accept(node->server_fd,
			(struct sockaddr * restrict)&client_addr,
			(socklen_t*)&client_addr_len);

		// Read the first message on contact - self description
		printf("Received a connection!!\n");
		NodeDesc client_desc;
		read(client_sock_fd, (void*)&client_desc, sizeof(NodeDesc));

		// Announce connection
		printf("Received connection from: Node(%d)\t Host(%s)\t Port(%d)|fd(%d)\n",
		     client_desc.id, client_desc.hostname,
		     client_desc.port, client_sock_fd);

		// Match with the neighbors and update the information
		Neighbor *nb = get_neighbor(conf, client_desc.id);
		nb->in_fd = client_sock_fd;
	}
}

struct cnarg{
	Config *config;
	Neighbor *nb;
};

void *connect_to_neighbor(void *args){
	Config *config = ((struct cnarg *)args)->config;
	Neighbor *nb = ((struct cnarg *)args)->nb;

	struct sockaddr_in saddr;

	// Templating hostname
	char complete_host_name[32];
	sprintf(complete_host_name, HN_TEMPLATE, config->LUT[nb->id].hostname);
	struct hostent *serv = NULL;

	nb->out_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (nb->out_fd < 0){
		perror("Socket");
		exit(-1);
	}

	// Converts host in string format to internal representation
	if ((serv = gethostbyname(complete_host_name)) == 0) {
		perror("Error on gethostbyname call");
		exit(1);
	}

	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(config->LUT[nb->id].port);
	saddr.sin_addr.s_addr = ((struct in_addr *)(serv->h_addr_list[0]))->s_addr;

	// Making attempts to connect to the neighbor's socket
	int attempt = 1;

	while(attempt < 50){
		// printf("Node#%d attempting to connect to Node#%d  -- attempt %d\n",config->node.id, nb->id, attempt);
		if(connect(nb->out_fd, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in)) < 0){
			// perror(DBG "connect");
		}else
			break;

		sleep(1);
		// printf("Connecting to: %s:%d...[Attempt %d]\n", complete_host_name, n->desc.port, attempt++);

		attempt++;
	}
	if(attempt >= 50){
		printf("Failed to connect to node#%d!!\n", nb->id);
		exit(-1);
	}
	printf("Node#%d connected to Node#%d\n",config->node.id, nb->id);
	// Once connected, send self identification information
	write(nb->out_fd, (char*)&config->LUT[config->node.id], sizeof(NodeDesc));
}

int setup_connections(Config *config){
	Node* node = &config->node;
	pthread_t tids[node->neighbors_count];
	struct cnarg args[node->neighbors_count];
	for(int i = 0; i < node->neighbors_count; i++){
		args[i].config = config;
		args[i].nb = &node->neighbors[i];
		pthread_create(&tids[i], NULL, connect_to_neighbor, &args[i]);
	}

	accept_connections(config);

	for(int i = 0; i < node->neighbors_count; i++){
		pthread_join(tids[i], NULL);
	}
}


void display_config(Config *config){
  printf(""\
    "            This is the config\n"\
    "------------------------------------------\n"\
    "N\t\t\t\t:%-3d\n"\
    "MinPerActive\t\t\t:%-5d\n"\
    "MaxPerActive\t\t\t:%-5d\n"\
    "MinSendDelay\t\t\t:%-5d\n"\
    "SnapshotDelay\t\t\t:%-5d\n"\
    "MaxNumber\t\t\t:%-5d\n"
    "------------------------------------------\n"\
    "Node.ID\t\t\t\t:%-5d\n"\
    "Node.NeighborsCount\t\t:%-5d\n"\
    "------------------------------------------\n",
    config->N,
    config->MinPerActive,
    config->MaxPerActive,
    config->MinSendDelay,
    config->SnapshotDelay,
    config->MaxNumber,

    config->node.id,
    config->node.neighbors_count
    );
  printf("The neighbor LUT\n");
  for(int i=0; i<config->N; i++){
  	printf("%-3d\t%-16s\t%3d\n", config->LUT[i].id, config->LUT[i].hostname, config->LUT[i].port);
  }
  printf("------------------------------------------\n");

  Neighbor *n;
  n = config->node.neighbors;

  for(int i = 0; i< config->node.neighbors_count; i++){
    printf(""
      "Node.Neighbor[%2d]: id(%-5d) Hostname(%8s) Port(%5d) InFD(%2d) OutFD(%2d)\n",
      i,
      n[i].id,
      config->LUT[n[i].id].hostname,
      config->LUT[n[i].id].port,
      n[i].in_fd,
      n[i].out_fd);
  }
  printf("------------------------------------------\n");
}

#endif