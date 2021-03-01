#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "map.h"
#include "hashtable.h"

player_t *makePlayer();

int main(const int argc, const char *argv[])
{

	FILE *fp = fopen("../maps/main.txt", "r");

	map_t *map = map_new(fp);

	player_t *p = makePlayer();

	map_t *plyrMap = map_buildPlayerMap(map,p,NULL, NULL);

	printf("map width: %d, height: %d\n\n", map->width, map->height);

	printf("%s\n", map_buildOutput(plyrMap));


}


player_t *makePlayer()
{

	position_t *pos = malloc(sizeof(position_t));
	pos->x = 10;
	pos->y = 3;

	player_t *p = malloc(sizeof(player_t));

	p->pos = pos;

	return p;
}