/*
* map.c implementation of map module
*
* Angus Emmett
* some sources from stackoverflow: https://stackoverflow.com/questions/174531/how-to-read-the-content-of-a-file-to-a-string-in-c
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "map.h"
#include "message.h"
#include "hashtable.h"

/**************** Private Functions ****************/
map_t *map_copy(map_t *map);
char *map_calculateVisibility(map_t *map, player_t *player, char *ogMapStr);
bool canPlayerCanMoveTo(map_t *map, position_t *pos);
void passageVis(char *currentVis, int i, map_t *map, char *mapstring, player_t *player);
void wallChar(char *mapstring, char *currentVis, int index);
bool isVisible(map_t *map, int i, player_t *player);
bool checkObstruct(int i, map_t *map);

/**************** Iterator Functions ****************/
void addPlayerITR(void *arg, const char *key, void *item);
void placeGold(void *arg, const char *key, void *item);
position_t *map_intToPos(map_t *map, int i);

/**************** map_new ****************/
map_t *map_new(FILE *fp)
{

	map_t *map = malloc(sizeof(map_t));
	if (map == NULL){
		return NULL;
	}

	char *buffer;
	long length;

	// Loading string into buffer 
	if (fp){
		fseek (fp, 0, SEEK_END);
		length = ftell (fp);
		fseek (fp, 0, SEEK_SET);
		buffer = malloc (length);
		if (buffer){
			fread (buffer, 1, length, fp);
		}
		fclose (fp);
	}

	int width = 0;
	int height = 0; 
	int offset = 0;

	if (buffer){
		// discarding the new lines and compressing the string while also calculating the w&h
		int i;
		for (i = 0; i < strlen(buffer); i++){

			if(buffer[i] == '\n'){
				height += 1;
				offset += 1;
			} else {
				// This shifts the line over if for every new line we have taken out
				buffer[i - offset] = buffer[i];
				width += 1;
			}
		}
		buffer[i - offset] = '\0';
	}

	map->width = width / height;
	map->height = height;

	char *mapStr = (char*) malloc( (length * sizeof(char)) + 5); 
	strcpy(mapStr, buffer);

	map->mapStr = buffer;
	
	return map;
}


/**************** map_buildPlayerMap ****************/
map_t *map_buildPlayerMap(map_t *map, player_t *player, hashtable_t *goldData, hashtable_t *players)
{
	map_t *outMap = map_copy(map);

	// Adding all the gold to the map
	if (goldData != NULL){
        hashtable_iterate(goldData, outMap, placeGold);
	}

	if (players != NULL){
		// Adding the players to the map
		hashtable_iterate(players, outMap, addPlayerITR);
	}

    // Replace this player's letter with '@'
    if (player != NULL) {
	    int plyIndx = map_calcPosition(outMap, player->pos);
	    outMap->mapStr[plyIndx] = '@';
    }
    outMap->mapStr = map_buildOutput(outMap);

    // figure out visibility and return the player's visibility mapstring
	outMap->mapStr = map_calculateVisibility(outMap, player, map->mapStr);
    return outMap;
}

void placeGold(void *arg, const char *key, void *item)
{
    map_t *outMap = arg;
    gold_t *g = item;
    if (!g->isCollected) {
        int gIndx = map_calcPosition(outMap, g->pos);
		outMap->mapStr[gIndx] = '*';
    }
}

void addPlayerITR(void *arg, const char *key, void *item)
{
	map_t *map = arg;
	player_t *player = item;
    if (player->isActive) {
	    int plyIndx = map_calcPosition(map, player->pos);
	    map->mapStr[plyIndx] = player->letter;
    }
}	


/**************** map_calcPosition ****************/
int map_calcPosition(map_t *map, position_t *pos)
{
	if (pos->x > map->width || pos->y > map->height || pos->x < 0 || pos->y < 0){
		return -1;
	}

	return (pos->y * map->width) + (pos->x + 1);
}

/**************** map_intToPos ****************/
position_t *map_intToPos(map_t *map, int i)
{
    position_t *pos = malloc(sizeof(position_t));

    i--;

    int width = map->width;

    pos->x = i%width;
    pos->y = i/width;

    return pos;
}

/**************** buildMap ****************/
char *map_buildOutput(map_t *map)
{

	if (map == NULL){
		return NULL;
	}

	int newLen = strlen(map->mapStr) + map->height;

	char *newMapStr = (char*) malloc( (newLen * sizeof(char)) + 5 ); 
	strcpy(newMapStr, map->mapStr);

	// Adding in new line characters 
	int offset = map->height;
	for(int i = strlen(map->mapStr); i >= 0; i--){

		newMapStr[i + offset] = newMapStr[i];
		if (i % map->width == 0 && i != 0){
			newMapStr[i + offset - 1] = '\n';
			offset -= 1;
		}
	}

	return newMapStr;
}


/**************** map_copy ****************/
map_t *map_copy(map_t *map)
{
	map_t *newMap = malloc(sizeof(map_t));

	newMap->width = map->width;
	newMap->height = map->height;

	char *newMapStr = (char*) malloc( (strlen(map->mapStr) * sizeof(char)) + 5); 
	strcpy(newMapStr, map->mapStr);
	newMap->mapStr = newMapStr;

	return newMap;
}


/**************** map_calculateVisibility ****************/
char *map_calculateVisibility(map_t *map, player_t *player, char *ogMapStr)
{
    // get player position, map string, current
    //  player visibility string
	char *mapstring = map->mapStr;
    int playerpos = map_calcPosition(map, player->pos);
    char *currentVis = player->visibility;
    
    // check if this is a new player
    //  and create visibility string if so
    if (strcmp(currentVis, "") == 0) {
        currentVis = map_buildOutput(map);
        for (int i = strlen(mapstring); i >= 0; i--) {
            if (mapstring[i] == '\n' || mapstring[i] == '\0') {
                // keep important signpost characters
                continue;
            } else if (!isVisible(map, i, player)) {
                // ditch everything else that's not visible
                currentVis[i] = ' ';
            }
        }
        return currentVis;
    } else if (currentVis[playerpos] == '@') {
        // position hasn't changed
        // visibility stays the same
        return currentVis;
    }

    // loop through previous visibility string
    for (int i = strlen(currentVis); i >= 0; i--) {
        if (currentVis[i] == '*' && !isVisible(map, i, player)) {
            // reset out-of-sight gold visibility
            currentVis[i] = '.';
        } else if (isalpha(currentVis[i]) != 0 && !isVisible(map, i, player)) {
            // reset out-of-sight player locations
            if (ogMapStr[i] == '#') {
                currentVis[i] = '#';
            } else {
                currentVis[i] = '.';
            }
        }
    }

    // loop through map to decide what
    //  currentVis should contain
    for (int i = strlen(mapstring); i >= 0; i--) {
        if (mapstring[i] == '@') {
            // make sure to include player position
            currentVis[i] = '@';
        } else if (mapstring[i] == '#') {
            // figure out what's visible in the passage
            passageVis(currentVis, i, map, mapstring, player);
        } else if (mapstring[i] == currentVis[i]) {
            // don't touch what's been seen
            continue;
        } else if (isVisible(map, i, player)) {
            // add what's newly visible
            currentVis[i] = mapstring[i];
        } else {
            // make sure rock is rock
            //  and invisible is invisible
            currentVis[i] = ' ';
        }
    }
    return currentVis;
}


/**************** passageVis ****************/
void passageVis(char *currentVis, int i, map_t *map, char *mapstring, player_t *player)
{
    if (isVisible(map, i, player)) {
        currentVis[i] = '#';
    } else {
        // get position and then indices of the four
        //  surrounding non-diagonal gridpoints
        position_t *pos = map_intToPos(map, i);
        int plusx, minusx, plusy, minusy;
        pos->x += 1;
        plusx = map_calcPosition(map, pos);
        pos->x -= 2;
        minusx = map_calcPosition(map, pos);
        pos->y += 1;
        plusy = map_calcPosition(map, pos);
        pos->y -= 2;
        minusy = map_calcPosition(map, pos);
        
        if (currentVis[plusx] == '@' ||
            currentVis[minusx] == '@' ||
            currentVis[plusy] == '@' ||
            currentVis[minusy] == '@') {
            // the player is next to this passage
            //  gridpoint and should be able to see it
            currentVis[i] = '#';
            
            // hand off to helper to figure out
            //  whether to add wall characters
            wallChar(mapstring, currentVis, plusx);
            wallChar(mapstring, currentVis, minusx);
            wallChar(mapstring, currentVis, plusy);
            wallChar(mapstring, currentVis, minusy);
        }
    }
}


/**************** wallChar ****************/
void wallChar(char *mapstring, char *currentVis, int index)
{
    // add in wall characters adjacent
    //  to this passage gridpoint
    switch (mapstring[index]) {
        case '+' :
            currentVis[index] = '+';
            break;
        case '-' :
            currentVis[index] = '-';
            break;
        case '|' :
            currentVis[index] = '|';
            break;
    }
}


/**************** isVisible ****************/
bool isVisible(map_t *map, int i, player_t *player)
{
    bool vis = true;

    // get position of the point in question
    position_t *gridpoint = map_intToPos(map, i);
    
    // establish slope m and constant b according to
    //  the standard linear equation y = mx + b
    //  from the player's position and that of the
    //  gridpoint in question
    int rise, run;
    float slope, constant;
    rise = player->pos->y - gridpoint->y;
    run = player->pos->x - gridpoint->x;
    slope = rise / run;
    constant = player->pos->y - (player->pos->x*slope);
    
    // loop through map to look for obstructions
    for (int j = strlen(map->mapStr); i >= 0; i--) {
        position_t *thispos = map_intToPos(map, j);
        // check whether this point is between
        //  the point of interest and the player's position
        if (!(((thispos->x <= player->pos->x &&
                thispos->x >= gridpoint->x) ||
               (thispos->x >= player->pos->x &&
                thispos->x <= gridpoint->x)) &&
              ((thispos->y <= player->pos->y &&
                thispos->y >= gridpoint->y) ||
               (thispos->y >= player->pos->y &&
                thispos->y <= gridpoint->y)))) {
            // not in the intervening rows/columns
            //  so not a concern for obstruction
            continue;

        } else if ((thispos->x*slope) + constant == (float)thispos->y) {
            // point directly on line; check if it obstructs
            if (checkObstruct(j, map)) {
                vis = false;
            }
        } else {
            // prepare quantities to compare to thispos->y
            float xslopeConst, plusxslopeConst, minusxslopeConst;
            xslopeConst = (thispos->x*slope) + constant;
            plusxslopeConst = ((thispos->x + 1)*slope) + constant;
            minusxslopeConst = ((thispos->x - 1)*slope) + constant;

            // various ways in which the line might pass between
            //  obstructing gridpoints; check obstruction accordingly
            if (xslopeConst <= (float)thispos->y &&
                plusxslopeConst >= (float)thispos->y) {
                thispos->x += 1;
                if (checkObstruct(j, map) &&
                    checkObstruct(map_calcPosition(map, thispos), map)) {
                    vis = false;
                }
            } else if (xslopeConst >= (float)thispos->y &&
                       plusxslopeConst <= (float)thispos->y) {
                thispos->x += 1;
                if (checkObstruct(j, map) &&
                    checkObstruct(map_calcPosition(map, thispos), map)) {
                    vis = false;
                }
            } else if (xslopeConst <= (float)thispos->y &&
                       minusxslopeConst >= (float)thispos->y) {
                thispos->x -= 1;
                if (checkObstruct(j, map) &&
                    checkObstruct(map_calcPosition(map, thispos), map)) {
                    vis = false;
                }
            } else if (xslopeConst >= (float)thispos->y &&
                       minusxslopeConst <= (float)thispos->y) {
                thispos->x -= 1;
                if (checkObstruct(j, map) &&
                    checkObstruct(map_calcPosition(map, thispos), map)) {
                    vis = false;
                }
            }
        }
    }
    return vis;
}


/**************** checkObstruct ****************/
bool checkObstruct(int i, map_t *map)
{
    bool obstruct = false;
    switch (map->mapStr[i]) {
        case '+' :
        case '-' :
        case '|' :
        case '#' :
        case ' ' :
            obstruct = true;
    }
    return obstruct;
}


/**************** map_movePlayer ****************/
void map_movePlayer(map_t *map, player_t *player, position_t *nextPos)
{
	// NULL check
	if (map == NULL || player == NULL || nextPos == NULL){
		return;
	}

	position_t *newPos = malloc(sizeof(position_t));
	if (newPos == NULL){ return; }

	newPos->x = player->pos->x;
	newPos->y = player->pos->y;



	int x_direction = 0;
	int y_direction = 0;

	// Checking direction of movement in x direction
	if (newPos->x < nextPos->x){ x_direction = 1; } 
	else { x_direction = -1; }

	// Checking direction of movement in y direction
	if (newPos->y < nextPos->y){ y_direction = 1; } 
	else { y_direction = -1; }
	
	// Diagonal
	if (nextPos->x - newPos->x != 0 && nextPos->y - newPos->y != 0) {

		// If movement isn't exactally diagonal return original position
		if ( abs(nextPos->x - newPos->x) != abs(nextPos->y - newPos->y) ){
			return;
		}

		// Adding direction to newPos as long as it is possible
		while(nextPos->x - newPos->x != 0 && nextPos->y - newPos->y != 0){
			
			newPos->y += y_direction;
			newPos->x += x_direction;

			if (! canPlayerCanMoveTo(map, newPos)){
				newPos->y -= y_direction;
				newPos->x -= x_direction;
				break;
			}
		}
	} 

	// Vertical
	else if (nextPos->y - newPos->y != 0) { 
		
		// Adding direction to newPos as long as it is possible
		while(nextPos->y - newPos->y != 0){
			
			newPos->y += y_direction;

			if (! canPlayerCanMoveTo(map, newPos)){
				newPos->y -= y_direction;
				break;
			}
		}

	} 

	// Horizontal
	else if (nextPos->x - newPos->x != 0) {
		
		// Adding direction to newPos as long as it is possible
		while(nextPos->x - newPos->x != 0){
			
			newPos->x += x_direction;

			if (! canPlayerCanMoveTo(map, newPos)){
				newPos->x -= x_direction;
				break;
			}
		}

	}

	player->pos->x = newPos->x;
	player->pos->y = newPos->y;

    // set nextPos x and y to check if the player moved
    nextPos->x = player->pos->x;
    nextPos->y = player->pos->y;

	free(newPos);
	return;
}

bool canPlayerCanMoveTo(map_t *map, position_t *pos)
{
	int indx = map_calcPosition(map, pos);
	char c = map->mapStr[indx];

	if (c != ' ' && c != '-' && c != '|' && c != '+'){
		return true;
	}

	return false;
}


/**************** map_delete ****************/
void map_delete(map_t *map)
{
	free(map->mapStr);
	free(map);
}
