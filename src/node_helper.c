#include "core.h"

Neighbor* get_neighbor(Config *config, int id){
	for(int i = 0; i < config->node.neighbors_count; i++)
		if(config->node.neighbors[i].id == id)
			return(&config->node.neighbors[i]);
}