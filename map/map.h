/* 
 * map.h -- header file for map module
 *
 * A map is a single-line string representing
 *  a playable game map that is created from a map file.
 *
 * Nuggets: Bash Boys
 */


#ifndef __MAP_H
#define __MAP_H


#include "hashtable.h"
#include "message.h"


/******************************** DATA STRUCTS ********************************/

/**************** position ****************/
typedef struct position {
	int x, y;
} position_t;

/**************** player ****************/
typedef struct player {
    addr_t addr;        // client address
    position_t *pos;
    int gold;
    char letter;        // public identifier
    bool isActive;      // current in-game status
    char *visibility;   // current field of vision
} player_t;

/**************** gold ****************/
typedef struct gold {
	int value;
	bool isCollected;
	position_t *pos;
} gold_t;

/**************** map ****************/
typedef struct map {
	char *mapStr;       // string representation of file input
	int width, height;
} map_t;


/******************************** FUNCTIONS ********************************/

/**************** map_new ****************/
/*
*	Initializes a new map from an open file to a map file
*	Assumes the file is a valid map
*
*	Mallocs new space for map struct and the map string, freed later on by map_delete
*	Returns NULL if fp is NULL of malloc error
*/
map_t *map_new(FILE *fp);


/**************** map_buildPlayerMap ****************/
/*
*	Takes in original map and produces a copy of a map for a the provided player 
*	Mallocs new space for newMap struct and the newMap string
* 
*	Returns NULL if map or player is NULL
*/
map_t *map_buildPlayerMap(map_t *map, player_t *player, hashtable_t *goldData, hashtable_t *players);


/**************** map_calcPosition ****************/
/*
*	calculates the index in the string from position coordinates
* 
*	Returns NULL if map or pos is NULL
*/
int map_calcPosition(map_t *map, position_t *pos);


/**************** buildMap ****************/
/*
*	Add back the new line characters for the client
* 	Mallocs a new string for this output and must be deleted after use
* 
*	Returns NULL if map is NULL
*/
char *map_buildOutput(map_t *map);


/***************** map_calculateVisibility *************/
/*
*   Loops through positions in map and passes each
*    to map_calcVisPath to determine visibility from the passed position
*/
void map_calculateVisibility(map_t *map, char *vis, position_t *pos);


/**************** map_movePlayer ****************/
/*
*	A function that moves the player to the given position if allowed
* 	Function will update player_t player position if allowed
* 	returns Nothing 
* 
*	Returns if map, player or nextPos is NULL
*/
void map_movePlayer(map_t *map, player_t *player, position_t *nextPos, hashtable_t *goldData);


/**************** map_intToPos ****************/
/*
*   Takes a mapstring index integer and converts
*    it to a position struct based on the passed map,
*    returning that position
*/
position_t *map_intToPos(map_t *map, int i);


/**************** map_delete ****************/
/*
*	Frees the map struct and the string inside it 
*/
void map_delete(map_t *map);


#endif // __MAPmake_H
