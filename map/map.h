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
	position_t pos;

} player_t;


/**************** gold ****************/
typedef struct gold {
	int value;
	bool isCollected;
	position_t pos;
} gold_t;






#endif // __MAPmake_H