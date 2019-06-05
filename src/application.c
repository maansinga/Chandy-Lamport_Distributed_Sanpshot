#ifndef __application_c__
#define __application_c__

#define _BSD_SOURCE

#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "application.h"

/*******************************************************************
MAP protocol implementation. This file - application.c implements the MAP 
protocol seperate from the ChandyLamport. This module is only concerned with sending
messages to the peers and not even bother about the timestamps and snapshots.
The messages are sent to service layer. There the actual timestamps and snapshots are
handled.
********************************************************************/

struct rfargs{
	Config* config;
	int write_fd;
	sem_t is_passive_access;
	int *is_passive;
	int *random_fire_running;
	int *total_sent;
};

/*
	This is a message generating thread. This shall be activated only if the MAP protocol's
	activation conditions are met. This thread sends minActive to maxActive messages per
	active state change.
*/
void *run_random_fire(void *args){
	static int rf_count = 1;
	struct rfargs *rf_args = args;
	Config* config = rf_args->config;
	sem_t is_passive_access = rf_args->is_passive_access;
	Node* node = &config->node;

	int *total_sent = rf_args->total_sent;
	int *volatile is_passive = rf_args->is_passive;
	int *random_fire_running = rf_args->random_fire_running;

	const int write_fd = rf_args->write_fd;

	const int neighbors_count = node->neighbors_count;
	const Neighbor* nbs = node->neighbors;
	const int node_id = node->id;
	char message_string[64];

	int m_count = config->MinPerActive + random()%(config->MaxPerActive - config->MinPerActive);

	m_count = (*total_sent) + m_count <= config->MaxNumber ? m_count : config->MaxNumber - (*total_sent);

	*total_sent = (*total_sent) + m_count;

	sem_wait(&is_passive_access);
	*random_fire_running = 1;
	*is_passive = 0;
	// printf("------Now going active\n");
	sem_post(&is_passive_access);

	srandom(time(NULL));

	// printf("Batch size is: %d\n", m_count);
	while(m_count>0 && !(*is_passive)){
		int r_nidx = random()%neighbors_count;

		sprintf(message_string, "Hello(%4d) from %2d to %2d", rf_count++, node_id, nbs[r_nidx].id);

		int size = ((strlen(message_string) + 1) * sizeof(char));
		ApplicationHeader header;
		header.src_id = node_id;
		header.dst_id = nbs[r_nidx].id;
		header.data_size = size;
		header.m_type = A_MESSAGE;
		header.state = 1;

		write(write_fd, (void*)&header, sizeof(ApplicationHeader));
		write(write_fd, message_string, header.data_size);

		// printf("Application on node#%d sent message to  node#%d:          %s(%d)\n", node->id, nbs[r_nidx].id, message_string, header.data_size);

		fsync(write_fd);

		m_count--;
		usleep(1000 *  (config->MinSendDelay + random()%config->MinSendDelay));
	}

	*random_fire_running = 0;

	sem_wait(&is_passive_access);
	*random_fire_running = 0;
	*is_passive = 1;
	// printf("------Now going passive\n");
	sem_post(&is_passive_access);

	// usleep(1000 *  (config->MinSendDelay + random()%config->MinSendDelay));

	// Tell service that application has gone passive
	ApplicationHeader header;
	header.src_id = node_id;
	header.dst_id = node_id;
	header.data_size = 0;
	header.m_type = A_STATE_REPLY;
	header.state = 0;

	write(write_fd, (void*)&header, sizeof(ApplicationHeader));
}

/**
Start Application takes the service configuration and runs MAP protocol on it. It receives messages from the
service layer and activates it's random_fire if applicable. It also informs the service layer about the
state of the application/MAP protocol upon request.

The application layer and service layer are connected through pipes. Service layer runs as a thread separate 
from the application thread.
*/
int start_application(Config *config, int read_write[2]){
	int read_fd = read_write[0];
	int write_fd = read_write[1];

	int buffer_size = sizeof(char)*64;
	char *buffer_pointer = (char*)malloc(buffer_size);

	Node* node = &config->node;
	
	int nbc = node->neighbors_count;

	int terminated = 0;
	int is_passive = node->id == 0 ? 0 : 1;

	fd_set read_set;
	struct timeval timeout;

	int total_sent = 0;
	int random_fire_running = 0;
	char message_string[32];

	struct rfargs args;

	args.config = config;
	args.write_fd = write_fd;
	sem_init(&args.is_passive_access, 0, 1);
	args.is_passive = &is_passive;
	pthread_t random_fire_tid;		
	args.random_fire_running = &random_fire_running;
	args.total_sent = &total_sent;
	random_fire_running = 0;

	while(!terminated){
		ApplicationHeader header = {0,0,0,0,0};

		if(total_sent > config->MaxNumber)is_passive = 1;
		else if(!is_passive && !random_fire_running && total_sent < config->MaxNumber){
			// printf("Starting random fire\n");
			is_passive = 0;
			random_fire_running = 1;
			pthread_create(&random_fire_tid, NULL, run_random_fire, &args);
		}



		FD_ZERO(&read_set);
		FD_SET(read_fd, &read_set);
		timeout.tv_sec = 0;
		timeout.tv_usec = 1000 * config->MinSendDelay;

		select(read_fd+1, &read_set, NULL, NULL, &timeout);
		if(FD_ISSET(read_fd, &read_set)){
			read(read_fd, &header, sizeof(ApplicationHeader));
			// Handling message reception from service layer
			switch(header.m_type){
				case A_STATE_QUERY:{
					// printf("Application says current spark is: %d total_sent: %d\n", 1 - is_passive, total_sent);
					header.m_type = A_STATE_REPLY;
					if(total_sent >= config->MaxNumber)header.state = 0;
					else header.state = 1 - is_passive;

					write(write_fd, (void*)&header, sizeof(ApplicationHeader));

					break;
				}
				case A_MESSAGE:{
					if(buffer_size < header.data_size){
						buffer_size *= 2;
						buffer_pointer = (char*)realloc(buffer_pointer, buffer_size);
					}
					read(read_fd, buffer_pointer, header.data_size);
					printf("Application on node#%d rcvd message frm node#%d:          %s\n", config->node.id, header.src_id, buffer_pointer);
					sem_wait(&args.is_passive_access);
					is_passive = 0;
					sem_post(&args.is_passive_access);

					break;
				}
			}
			
		}
	}

	free(buffer_pointer);
	return(0);
}

#endif