#ifndef __service_h__
#define __service_h__

#include "core.h"


typedef struct{
	int flag_snapshots_session_active;
	int flag_markers_sent;

	int snapshots_session_parent_id;
	int *snapshots_session_monitor_punch_card;
	int *snapshots_service_clock;
	int *snapshots_session_children;
	int *snapshots_accumulated_timestamps;
} SnapshotSession;

typedef struct mq_node{
	struct mq_node *prev;
	SnapshotSession msg;
	struct mq_node *next;
} MQNode;

typedef struct message_queue{
	MQNode *head;
	MQNode *tail;
} MQueue;

struct rsargs{
	Config *config;
	int pipe_fd[2];
};

struct trhargs{
	Config *config;
	Neighbor *nb;
	int thread_write;
};

struct saargs{
	Config *config;
	int *flag_snapshots_session_active;
};

SnapshotSession init_new_session(Config*, int);

int start_service(Config*, int[2]);

int compare_clocks_gte(int *, int *, int);
void init_queue(MQueue *);
void enqueue(MQueue *, SnapshotSession);
MQNode *delete_queue_entry(MQueue *, MQNode *);
SnapshotSession dequeue(MQueue *);
void mattern_clock_update(int *, int *, int, int);
void deliver_to_app(int, ServiceMessage);
void disp_queue(MQueue *);
void dispclock(int *, int);
void save_local_snapshot(Config*);

/******************************/
void reset_snapshots_control_variables(Config*);
void seek_application_state(Config* );

void handle_app_recv_message(Config *, ServiceMessage);
void handle_app_send_message(Config *, ApplicationHeader);

void manage_marker(Config *, ServiceMessage);
void manage_ack(Config *, ServiceMessage);
void manage_nack(Config *, ServiceMessage);
void manage_ccast(Config *, ServiceMessage);
void manage_term(Config *, ServiceMessage);
#endif