/* 
 * map.h - header file map module in nuggets project
 *
 * A map is a single line string representation that is created from a map file.
 */

#ifndef __MAP_H
#define __MAP_H

/**************** position ****************/
typedef struct position {
	int x, y;
} position_t;


/**************** player ****************/
typedef struct player {
	int goldCollected;
	char *name;
	bool isActive;
	position_t *pos;

} player_t;


/**************** gold ****************/
typedef struct gold {
	int value;
	bool isCollected;
	position_t *pos;
} gold_t;

typedef struct map {
	char *mapStr;
	int width, height;
} map_t;

/**************** map_new ****************/
map_t *map_new(FILE *fp);


/**************** map_buildPlayerMap ****************/
map_t *map_buildPlayerMap(map_t *map, player_t *player, gold_t **goldArr);


/**************** map_calcPosition ****************/
int map_calcPosition(map_t *map, position_t *pos);


/**************** buildMap ****************/
map_t *map_buildOutput(map_t *map);


/**************** map_placeGold ****************/
map_t *map_placeGold(map_t *map, gold_t **goldArr);


/**************** map_delete ****************/
void map_delete(map_t *map);

#endif // __MAPmake_H