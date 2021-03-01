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
/*
*	Initializes a new map from an open file to a map file
*	Assumes the file is a valid map
*
*	Mallocs new space for map struct and the map string, freed later on by map_delete
*/
map_t *map_new(FILE *fp);


/**************** map_buildPlayerMap ****************/
/*
*	Initializes a new map from an open file to a map file
*	Assumes the file is a valid map
*
*	Mallocs new space for map struct and the map string, freed later on by map_delete
*/
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