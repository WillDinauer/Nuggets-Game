/*
* map.c implementation of map module
*
* some sources used http://www.fundza.com/c4serious/fileIO_reading_all/index.html: 
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "map.h"
#include "message.h"
#include "hashtable.h"

/**************** Private Functions ****************/
map_t *map_copy(map_t *map);
char *map_calculateVisibility(map_t *map, player_t *player, hashtable_t *goldData, hashtable_t *players);
bool canPlayerMoveTo(map_t *map, position_t *pos);
/**************** Iterator Functions ****************/
void addPlayerITR(void *arg, const char *key, void *item);
void placeGold(void *arg, const char *key, void *item);
position_t *map_intToPos(map_t *map, int i);
void isOnGoldITR(void *arg, const char *key, void *item);

/**************** map_new ****************/
map_t *map_new(FILE *fp)
{

	map_t *map = malloc(sizeof(map_t));
	if (map == NULL){
		return NULL;
	}

	char *buffer;
	long numbytes;
		
	// Getting total num of bytes and moving ptr back to start of file
	fseek(fp, 0L, SEEK_END);
	numbytes = ftell(fp);
	fseek(fp, 0L, SEEK_SET);	
	
	// Allocating new mem for string
	buffer = (char*)calloc(numbytes, sizeof(char));	
	// Mem check
	if(buffer == NULL){
		return NULL;
	}
		
	// Copy all chars into string
	fread(buffer, sizeof(char), numbytes, fp);
	

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

	char *mapStr = (char*) malloc( (numbytes * sizeof(char)) + 5); 
	strcpy(mapStr, buffer);


	map->mapStr = mapStr;
	free(buffer);

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

	return outMap;
}

/**************** placeGold ****************/
void placeGold(void *arg, const char *key, void *item)
{
    map_t *outMap = arg;
    gold_t *g = item;
    if (!g->isCollected) {
        int gIndx = map_calcPosition(outMap, g->pos);
		outMap->mapStr[gIndx] = '*';
    }
}

/**************** addPlayerITR ****************/
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
	// checking that pos is not out of bounds
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
/*
* returned string must be freed by the caller
*/
char *map_buildOutput(map_t *map)
{

	if (map == NULL){
		return NULL;
	}
	// Getting len of built up map
	int newLen = strlen(map->mapStr) + map->height;

	// creating new map str in mem
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
  
    free(map->mapStr);
	return newMapStr;
}


/**************** map_copy ****************/
map_t *map_copy(map_t *map)
{
	// Creating new mem for map
	map_t *newMap = malloc(sizeof(map_t));

	// Copying the h and w
	newMap->width = map->width;
	newMap->height = map->height;

	// allocating new mem and copying into newMap
	char *newMapStr = (char*) malloc( (strlen(map->mapStr) * sizeof(char)) + 5); 
	strcpy(newMapStr, map->mapStr);
	newMap->mapStr = newMapStr;

	return newMap;
}


/**************** map_calculateVisibility ****************/
char *map_calculateVisibility(map_t *map, player_t *player, hashtable_t *goldData, hashtable_t *players)
{
	return "Place Holder";
}


/**************** map_movePlayer ****************/
void map_movePlayer(map_t *map, player_t *player, position_t *nextPos, hashtable_t *goldData)
{
	// NULL check
	if (map == NULL || player == NULL || nextPos == NULL){
		return;
	}

	// newPos is the pos that we update throughout the loop
	position_t *newPos = malloc(sizeof(position_t));
	if (newPos == NULL){ return; }

	newPos->x = player->pos->x;
	newPos->y = player->pos->y;

	int x_direction;
	int y_direction;

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

			if (! canPlayerMoveTo(map, newPos)){
				newPos->y -= y_direction;
				newPos->x -= x_direction;
				break;
			}

			// Checks if during this move they pick up gold
			player->pos->x = newPos->x;
			player->pos->y = newPos->y;
			hashtable_iterate(goldData, player, isOnGoldITR);

		}
	} 

	// Vertical
	else if (nextPos->y - newPos->y != 0) { 
		
		// Adding direction to newPos as long as it is possible
		while(nextPos->y - newPos->y != 0){
			
			newPos->y += y_direction;

			if (! canPlayerMoveTo(map, newPos)){
				newPos->y -= y_direction;
				break;
			}

			// Checks if during this move they pick up gold
			player->pos->x = newPos->x;
			player->pos->y = newPos->y;
			hashtable_iterate(goldData, player, isOnGoldITR);
		}
	} 

	// Horizontal
	else if (nextPos->x - newPos->x != 0) {
		
		// Adding direction to newPos as long as it is possible
		while(nextPos->x - newPos->x != 0){
			
			newPos->x += x_direction;

			if (! canPlayerMoveTo(map, newPos)){
				newPos->x -= x_direction;
				break;
			}

			// Checks if during this move they pick up gold
			player->pos->x = newPos->x;
			player->pos->y = newPos->y;
			hashtable_iterate(goldData, player, isOnGoldITR);

		}
	}

    // set nextPos x and y to check if the player moved
    nextPos->x = player->pos->x;
    nextPos->y = player->pos->y;

	free(newPos);
	return;
}


/**************** canPlayerCanMoveTo ****************/
bool canPlayerMoveTo(map_t *map, position_t *pos)
{	
	// Calculating the index in the string from the pos
	int indx = map_calcPosition(map, pos);
	char c = map->mapStr[indx];

	// Checking if pos is a space where you cant move to
	if (c != ' ' && c != '-' && c != '|' && c != '+'){
		return true;
	}

	return false;
}

void isOnGoldITR(void *arg, const char *key, void *item)
{
	// sendGoldMessage now happens after in server file after player move is complete
	player_t *player = arg;
	gold_t *goldItem = item;

	if(!goldItem->isCollected && player->pos->x == goldItem->pos->x && player->pos->y == goldItem->pos->y){
		player->gold += goldItem->value;
		goldItem->isCollected = true;
	}
}



/**************** map_delete ****************/
void map_delete(map_t *map)
{	
	// Deletes map str and map if not null
    if (map != NULL) {
        if (map->mapStr != NULL) {
            free(map->mapStr);
        }
        free(map);
    }
}
