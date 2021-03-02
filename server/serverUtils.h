/* 
 * serverUtils.h - header file for serverUtils module
 *
 *
 */

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

typedef struct serverInfo {
	int *numPlayers;
	const int maxPlayers;
	hashtable_t *playerInfo;
	counters_t *dotsPos;
	map_t *map;
	addr_t specAddr;
} serverInfo_t;


bool validateAction(char *keyPress, player_t *player, serverInfo_t *info);