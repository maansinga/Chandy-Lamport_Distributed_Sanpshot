#include "service.h"

void dispclock(int *c, int n){
	printf("Clock: ");
	for(int i=0; i<n; i++)
		printf("%3d ",c[i]);
	printf("\n");
}

int compare_clocks_gte(int *clock1, int *clock2, int n){
	int greater_than=0, less_than=0, equal=0;
	for(int i = 0; i < n; i++){
		if(clock1[i] > clock2[i])greater_than++;
		else if(clock1[i] < clock2[i])less_than++;
		else equal++;
	}
	if((equal == n) || (greater_than != 0 && less_than == 0))return 1;
	else return -1;
}

void init_queue(MQueue *queue){
	queue->head = NULL;
	queue->tail = NULL;
}

void enqueue(MQueue *queue, SnapshotSession msg){
	MQNode *node = (MQNode*)malloc(sizeof(MQNode));
	node->msg = msg;
	if(queue->head == NULL){
		queue->head = node;
		queue->tail = node;
		node->prev = NULL;
		node->next = NULL;
	}else{
		queue->tail->next = node;
		node->prev = queue->tail;
		node->next = NULL;
		queue->tail = node;
	}
}

SnapshotSession dequeue(MQueue *queue){
	SnapshotSession ret = queue->head->msg;
	MQNode *next = queue->head->next;
	free(queue->head);
	queue->head = next;
	next->prev = NULL;

	return(ret);
}

MQNode *delete_queue_entry(MQueue *mq, MQNode *mqnode){
	MQNode *next_iter;
	if(mqnode->prev == NULL && mqnode->next == NULL){ // Case of only node
		mq->head = NULL;
		mq->tail = NULL;
		next_iter = NULL;
		free(mqnode);
	}else if(mqnode->prev == NULL && mqnode->next!=NULL){ // Case of Head
		mq->head = mqnode->next;
		mq->head->prev = NULL;
		next_iter = mq->head;
		free(mqnode);
	}else if(mqnode->prev != NULL && mqnode->next == NULL){ // Case of Tail
		mqnode->prev->next = NULL;
		mq->tail = mqnode->prev;
		next_iter = NULL;
		free(mqnode);
	}else{
		MQNode *nextp, *prevp;
		nextp = mqnode->next;
		prevp = mqnode->prev;

		nextp->prev = prevp;
		prevp->next = nextp;
		next_iter = nextp;
		free(mqnode);
	}

	return(next_iter);
}

void mattern_clock_update(int *local, int *recv, int j, int n){
	for(int i=0; i < n; i++){
		local[i] = local[i] > recv[i] ? local[i] : recv[i];
	}
	local[j]++;
}

void deliver_to_app(int pipefd, ServiceMessage msg){
	ApplicationHeader appheader;
	appheader.src_id = msg.header.src_id;
	appheader.dst_id = msg.header.dst_id;
	appheader.data_size = msg.header.data_size;


	write(pipefd, &appheader, sizeof(ApplicationHeader));
	write(pipefd, msg.data, msg.header.data_size);
}

void disp_queue(MQueue *q){
	MQNode *n = q->head;

	int i = 0;
	printf("------------------------------\n");
	// while(n!=NULL){
	// 	printf("Queue(%d): %s |", i++, n->msg.data);
	// 	for( int j = 0; j < (signed)(n->msg.header.clock_size/sizeof(int)); j++){
	// 		printf(" %d", n->msg.clock[j]);
	// 	}
	// 	printf("\n");
	// 	// disp_clock(n->msg.clock, n->msg.header.clock_size/sizeof(int));
	// 	n = n->next;
	// }
	printf("------------------------------\n");
}

FILE* configout = NULL;
void save_local_snapshot(Config* config){
	int *session_clock = config->snapshots_clock;
	if(configout==NULL){
		char snapshotsfile[128];
		sprintf(snapshotsfile, STR(DUMPDIR)"config-%d.out", config->node.id);

		configout = fopen(snapshotsfile, "a");
	}
	
	int i=0;
	for(i=0; i < config->N - 1; i++ ){
		fprintf(configout, "| %6d ", session_clock[i]);
	}
	fprintf(configout, "| %6d |\n", session_clock[i]);
	fflush(configout);
	// fclose(configout);
}

/******************************************************************/
void reset_snapshots_control_variables(Config* config){
	memset(config->snapshots_session_monitor_punch_card, 0, sizeof(int) * config->N);
	memset(config->snapshots_clock, 0, sizeof(int) * config->N);
	// memset(config->snapshots_session_children, 0, sizeof(int) * config->N);
	memset(config->snapshots_accumulated_timestamps, 0xff, sizeof(int) * config->N * config->N);
}

void handle_app_recv_message(Config *config, ServiceMessage smessage){
	config->snapshots_session_spark = 1;
	// Update the vector clock that is service_clock
	mattern_clock_update(config->clock, smessage.clock, smessage.header.src_id, config->N);

	if(config->flag_snapshots_session_active == 1 &&
	 config->flag_markers_sent == 1 &&
	  config->snapshots_session_monitor_punch_card[smessage.header.src_id] == 0){

		config->snapshots_clock[smessage.header.src_id]++;
	}

	// Send the application message to the application layer
	ApplicationHeader aheader;
	aheader.src_id = smessage.header.src_id;
	aheader.dst_id = smessage.header.dst_id;
	aheader.data_size = smessage.header.data_size;
	aheader.m_type = A_MESSAGE;

	write(config->app_write_fd, &aheader, sizeof(ApplicationHeader));
	write(config->app_write_fd, smessage.data, smessage.header.data_size);

	// Freeup the allocations made by the ReaderThread
	if(smessage.data!=NULL){
		free(smessage.data);
		smessage.data = NULL;
	}

	if(smessage.clock!=NULL){
		free(smessage.clock);
		smessage.clock = NULL;
	}
}

void seek_application_state(Config* config){
	ApplicationHeader aheader;
	aheader.src_id = config->node.id;
	aheader.dst_id = config->node.id;
	aheader.data_size = 0;
	aheader.m_type = A_STATE_QUERY;

	write(config->app_write_fd, &aheader, sizeof(ApplicationHeader));
}

int buffer_size = 64;
char *data_buffer_pointer=NULL;
void handle_app_send_message(Config *config, ApplicationHeader aheader){
	config->snapshots_session_spark = 1;

	if(data_buffer_pointer==NULL){
		data_buffer_pointer = (char*)malloc(sizeof(char) * buffer_size);
	}

	// pack and send
	ServiceHeader header;
	header.m_type = M_APPLICATION;
	header.syncbits = 0xaaaaaaaa;
	header.src_id = config->node.id;
	header.dst_id = aheader.dst_id;
	header.data_size = aheader.data_size;
	header.clock_size = sizeof(int) * config->N;
	
	if(aheader.data_size > 0){
		data_buffer_pointer = (char*)malloc(aheader.data_size);
		if(read(config->app_read_fd, data_buffer_pointer, aheader.data_size) != aheader.data_size)
			perror("read");
	}

	Neighbor *nb = get_neighbor(config, header.dst_id);
	printf( "Service     on node#%d sent message to  node#%d:          %s | ", config->node.id, nb->id, data_buffer_pointer);
	dispclock(config->clock, config->N);

	write(nb->out_fd, &header, sizeof(ServiceHeader));
	write(nb->out_fd, config->clock, header.clock_size);
	if(aheader.data_size > 0)write(nb->out_fd, data_buffer_pointer, header.data_size);

	config->clock[config->node.id]++;
}

void manage_marker(Config *config, ServiceMessage smessage){
	if(config->flag_snapshots_session_active == 1){
		printf( "Service     on node#%d rcvd (stale)MARKER  frm node#%d:          \tSnapshot(%d) | ", config->node.id, smessage.header.src_id, smessage.header.snap_no);
		dispclock(config->clock, config->N);
		config->snapshots_session_monitor_punch_card[smessage.header.src_id] = 1;
	}else{
		printf( "Service     on node#%d rcvd (new)MARKER  frm node#%d:          \t\tSnapshot(%d) | ", config->node.id, smessage.header.src_id, smessage.header.snap_no);
		dispclock(config->clock, config->N);
		printf("SNAPSHOTS session(%d) started on node#%d by node#%d\n", smessage.header.snap_no, config->node.id, smessage.header.src_id);

		// Send ACK to parent

		reset_snapshots_control_variables(config);
		memset(config->snapshots_session_children, 0, sizeof(int) * config->N);

		config->flag_snapshots_session_active = 1;
		config->snapshots_session_parent_id = smessage.header.src_id;
		config->snapshots_session_monitor_punch_card[smessage.header.src_id] = 1;
		config->snapshots_session_children_count = 0;
		config->snapshots_session_ccast_count = 0;
		config->snapshot_no = smessage.header.snap_no;

		memcpy(config->snapshots_clock, config->clock, sizeof(int) * config->N);

		// Send MARKERS to neighbors
		ServiceMessage snap = {{0xaaaaaaaaL, M_MARKER, config->node.id, 0, 0, 0, smessage.header.snap_no, 1}, NULL, NULL};

		for(int i = 0; i < config->node.neighbors_count; i++){
			Neighbor *nb = &config->node.neighbors[i];
			snap.header.dst_id = nb->id;

			if(nb->id == config->snapshots_session_parent_id){
				snap.header.m_type = M_MARKER_ACK;	
				write(nb->out_fd, &snap.header, sizeof(ServiceHeader));
			}

			snap.header.m_type = M_MARKER;
			snap.header.data_size = 0;
			snap.header.clock_size = 0;

			write(nb->out_fd, &snap.header, sizeof(ServiceHeader));
		}
		config->flag_markers_sent = 1;
	}
	if(smessage.data!=NULL){
		free(smessage.data);
		smessage.data = NULL;
	}

	if(smessage.clock!=NULL){
		free(smessage.clock);
		smessage.clock = NULL;
	}
};

void manage_ack(Config *config, ServiceMessage smessage){
	// snapshots_session_children
	printf( "Service     on node#%d rcvd ACK  frm node#%d:          \t\tSnapshot(%d) | ", config->node.id, smessage.header.src_id, smessage.header.snap_no);
		dispclock(config->clock, config->N);
	config->snapshots_session_children[smessage.header.src_id] = 1;
	config->snapshots_session_children_count++;

	if(smessage.data!=NULL){
		free(smessage.data);
		smessage.data = NULL;
	}

	if(smessage.clock!=NULL){
		free(smessage.clock);
		smessage.clock = NULL;
	}
};
void manage_nack(Config *config, ServiceMessage smessage){
	printf( "Service     on node#%d rcvd NACK  frm node#%d:          \t\tSnapshot(%d) | ", config->node.id, smessage.header.src_id, smessage.header.snap_no);
		dispclock(config->clock, config->N);
	config->snapshots_session_children[smessage.header.src_id] = 0;

	if(smessage.data!=NULL){
		free(smessage.data);
		smessage.data = NULL;
	}

	if(smessage.clock!=NULL){
		free(smessage.clock);
		smessage.clock = NULL;
	}
};
void manage_ccast(Config *config, ServiceMessage smessage){
	printf( "Service     on node#%d rcvd CCAST  frm node#%d:          \t\tSnapshot(%d) | ", config->node.id, smessage.header.src_id, smessage.header.snap_no);
		dispclock(config->clock, config->N);

	if(config->node.id == 0){
		config->snapshots_session_ccast_count++;
		config->snapshots_session_spark += smessage.header.snap_spark;
		printf("config->snapshots_session_spark = %d\n",config->snapshots_session_spark );
		// printf("Node %d's CCAST received at node0\n", smessage.header.src_id);
		for(int i = 0; i < config->N; i++){
			// printf("%d ",((int*)smessage.data)[i]);
			config->snapshots_accumulated_timestamps[ (smessage.header.src_id * config->N) + i] = ((int*)smessage.data)[i];
		}
		// printf("\n");

	}else{
		smessage.header.dst_id = config->snapshots_session_parent_id;
		Neighbor* nb = get_neighbor(config, config->snapshots_session_parent_id);
		printf( "Service     on node#%d fwdg CCAST of node#%d message to  node#%d:          | ", config->node.id, smessage.header.src_id, nb->id);
		dispclock(config->clock, config->N);

		int fsnapshot_clock[config->N];
		for(int i = 0; i < config->N; i++){
			fsnapshot_clock[i] = ((int*)smessage.data)[i];
			// printf("%d ", fsnapshot_clock[i]);
		}
		// printf("\n\n");

		if(config->snapshots_session_children[smessage.header.src_id] == 1)config->snapshots_session_ccast_count+=1;

		write(nb->out_fd, &smessage.header, sizeof(ServiceHeader));
		write(nb->out_fd, fsnapshot_clock, smessage.header.data_size);
	}
	
	if(smessage.data!=NULL){
		free(smessage.data);
		smessage.data = NULL;
	}

	if(smessage.clock!=NULL){
		free(smessage.clock);
		smessage.clock = NULL;
	}
};
void manage_term(Config *config, ServiceMessage smessage){
	// printf("Termination after session(%d) started on node#%d\n", config->snapshot_no, config->node.id);
	printf("------------------TERMINATION determined-----------------------\n");
	printf("TERMINATION | session(%d) |  node#%d - CHILDREN - ", config->snapshot_no, config->node.id);
	for(int i = 0; i < config-> N; i++)if(config->snapshots_session_children[i]==1)printf("%d ", i);
		printf("\n");

	for(int i = 0; i < config->N; i++){
		ServiceMessage snap = {{0xaaaaaaaaL, M_MARKER_TERM, config->node.id, 0, 0, 0, ++(config->snapshot_no), config->snapshots_session_spark}, NULL, NULL};

		// printf("config->snapshots_session_children[%d] == 1 (%d)\n",i,  config->snapshots_session_children[i] == 1);
		if(config->snapshots_session_children[i] == 1){
			Neighbor *nb = get_neighbor(config, i);

			printf( "Service     on node#%d sent (TERM)MARKER  to node#%d:          \t\tSnapshot(%d) | ",
			 config->node.id,
			 nb->id,
			 smessage.header.snap_no
			);
			dispclock(config->clock, config->N);
			snap.header.dst_id = nb->id;
			write(nb->out_fd, &snap.header, sizeof(ServiceHeader));
		}
	}

	sleep(2);
	exit(0);
	if(smessage.data!=NULL){
		free(smessage.data);
		smessage.data = NULL;
	}

	if(smessage.clock!=NULL){
		free(smessage.clock);
		smessage.clock = NULL;
	}
};