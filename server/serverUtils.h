/* 
 * serverUtils.h - header file for serverUtils module
 *
 * Group 7 - Bash Boys
 *
 * Dartmouth CS50, Winter 2021
 */

#ifndef __SERVERUTILS_H
#define __SERVERUTILS_H


#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include "map.h"
#include "message.h"
#include "log.h"
#include "hashtable.h"
#include "set.h"
#include "counters.h"

/********* Data Structures **********/
typedef struct serverInfo {
    int *numPlayers;
    int *goldCt;
    const int maxPlayers;
    hashtable_t *playerInfo;
    hashtable_t *goldData;
    counters_t *dotsPos;
    map_t *map;
    addr_t specAddr;
} serverInfo_t;

/*********** Functions ************/

/************** validateAction *******************/
/* validates the action of a player, returning true if that player
 * has moved as a result of their key press
 */
bool validateAction(char *keyPress, player_t *player, serverInfo_t *info);

#endif // __SERVERUTILS_H
