#ifndef __setup_h__
#define __setup_h__

int setup_config(char *, int, Config *);
int setup_service(Config *, int *);
void setup_connections(Config *);
void display_config(Config *);
#endif