#ifndef __core_h__
#define __core_h__

#define _POSIX_C_SOURCE 201806L
/**
This file contains all the data structure declarations required for all the protocols
*/
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

#define TOSTR(X) #X
#define STR(X) TOSTR(X)
#define DBG STR(__FILE__) " " STR(__LINE__) " "

#define HN_TEMPLATE "%s.utdallas.edu"

// Message types
#define M_APPLICATION 	0		// Normal application message
#define M_MARKER		1		// Marker message
#define M_MARKER_ACK	2		// Parent acknowledgement
#define M_MARKER_NACK	3		// Parent non-acknowledgement
#define M_MARKER_TERM	4 		// Termination 
#define M_CCAST			5 		// Converge cast

// A node descriptor - used for hostname lookup
typedef struct{
	int 	id;
	int 	port;
	char 	hostname[16];
} NodeDesc;

// Neighbor structure - every node has neighbors
typedef struct{
	int 	id;
	int 	is_child;		// ACK/NACK will decide the value - after first snapshot

	int 	in_fd;
	int 	out_fd;
} Neighbor;

// Node structure - represents the running node
typedef struct{
	int 		id;
	int 		server_fd;

	int 		neighbors_count;
	Neighbor 	*neighbors;
} Node;

// Configuration structure
typedef struct{
	int N;					// Total number of nodes in the distributed system
	int MinPerActive;		// Minimum numnber of messages sent when active
	int MaxPerActive;		// Maximum number of messages to be sent when active
	int MinSendDelay;		// Minimum sending delay
	int SnapshotDelay;		// Snapshot to snapshot delay
	int MaxNumber;			// Maximum number of messages a node can send

	NodeDesc	*LUT;
	Node 		node;

	int app_read_fd;
	int app_write_fd;

	int thread_read_fd;
	int thread_write_fd;

	int *clock;

	int flag_markers_sent;
	int flag_snapshots_alarm_active;
	int flag_snapshots_session_active;

	int snapshots_session_parent_id;
	int snapshots_session_spark;
	int snapshot_no;
	int snapshots_session_children_count;
	int snapshots_session_ccast_count;

	int *snapshots_session_monitor_punch_card;
	int *snapshots_clock;
	int *snapshots_session_children;	
	int *snapshots_accumulated_timestamps;
} Config;

// Message header for piggybacking timestamps on application messages
#pragma pack(16)
typedef struct{
	unsigned int syncbits;
	int m_type;

	int src_id;
	int dst_id;

	int clock_size;
	int data_size;

	// Dirty fix;
	int snap_no;
	int snap_spark;
} ServiceHeader;

// Internal representation of the messages that go on the network
typedef struct{
	ServiceHeader header;
	int *clock;
	void *data;
} ServiceMessage;

// Application message structure
typedef struct{
	int src_id;
	int dst_id;

	int data_size;
	
	void *data;
} AppMessage;

#define A_STATE_QUERY	1
#define A_STATE_REPLY	2
#define A_MESSAGE		3

typedef struct{
	int src_id;
	int dst_id;

	int m_type;
	int state;

	int data_size;
} ApplicationHeader;

// #define DESTROY_MSG(X) free(X->data);
// #define DESTROY_CLK(X) free(X->clock);

#define DEBUG 1

#define DBGONLY if(DEBUG)
Neighbor* get_neighbor(Config *, int);
#endif