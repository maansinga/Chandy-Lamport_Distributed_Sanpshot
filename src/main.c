#define _BSD_SOURCE

#include <stdlib.h>
#include <time.h>
#include "core.h"
#include "setup.h"
#include "service.h"
#include "application.h"

char *savepid_template = "echo \"%d\" > ../pids/pid_%d.txt";

// Global pointer for configuration
Config config;


int main (int argc, char *argv[]){
	int id = -1;
	if (argc<1){
		printf ("Usage: %s node_id\n", argv[0]);
		return (-1);
	}else{
		id = atoi(argv[1]);
	}
	int read_write[4];
	printf("Initializing node#%d\n", id);	
	srandom(time(NULL));

	setup_config(CONFIG, id, &config);
	setup_connections(&config);
	start_service(&config, read_write);

	display_config(&config);
	printf("Running application on node#%d\n", id);
	start_application(&config, read_write);

	// Make the application use read_write

	printf("Terminating node#%d\n", id);	
	return(0);
}	