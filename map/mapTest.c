#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "map.h"
#include "hashtable.h"

player_t *makePlayer(map_t *map);
void randPos(position_t *pos);
bool checkValidMove(map_t *map, player_t *p);

int main(const int argc, const char *argv[])
{

	FILE *fp = fopen("../maps/small.txt", "r");

	map_t *map = map_new(fp);
	printf("map width: %d, height: %d\n\n", map->width, map->height);

	
	player_t *p = makePlayer(map);

	int indx = map_calcPosition(map, p->pos);
	map->mapStr[indx] = '@';


	map_calculateVisibility(map, p);
	printf("Done\n");

	return 0;


	// map_t *plyrMap;

	// // Testing player movement 
	// for (int i = 0; i < 20; i++){
	// 	randPos(pos);
	// 	map_movePlayer(map, p, pos, hashtable_new(1));
	// 	plyrMap = map_buildPlayerMap(map,p,NULL, NULL);

	// 	if (checkValidMove(map,p)){
	// 		printf(" -- Valid Move\n");
	// 	} else {
	// 		printf(" -- Invalid Move\n");
	// 	}
	// }
	// printf("%s\n", plyrMap->mapStr);

	// // Testing map delete
	// if (map != NULL){
	// 	map_delete(map);
	// 	printf("map_delete() was successful\n");
	// }
	
}


player_t *makePlayer(map_t *map)
{
	player_t *player = malloc(sizeof(player_t));
	if (player == NULL) { // out of memory
		return NULL;
	} 
	// initialize player info
	player->isActive = true;
	player->gold = 0;
	player->visibility = calloc(map->width * map->height + 1, sizeof(char));

	player->pos = malloc(sizeof(position_t));
	player->pos->x = 6;
	player->pos->y = 3;


	for (int i = 0; i < map->width * map->height; i++) {
		strcat(player->visibility, "0");
	}

	return player;
}

void randPos(position_t *pos)
{

	int mv = rand() % 3;
	printf("Moving Player from: (%d,%d) to ", pos->x, pos->y);
	// Vertical
	if (mv == 0){
		pos->y = rand() % 10;
	} 
	// Horizontal
	else if (mv == 1){
		pos->x = rand() % 10;
	} 
	// Diagonal
	else{
		pos->x = rand() % 10;
		pos->y = rand() % 10;
	}
	printf("(%d,%d)", pos->x, pos->y);
}


bool checkValidMove(map_t *map, player_t *p)
{
	char c = map->mapStr[(p->pos->y * map->width) + (p->pos->x + 1)];
	if (c != ' ' && c != '-' && c != '|' && c != '+'){
		return true;
	}
	return false;
}