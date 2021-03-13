/*
* map.c -- implementation of map module
*
* See map.h for more details and ../IMPLEMENTATION.md for even more
*
* Nuggets: Bash Boys 
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "map.h"
#include "message.h"
#include "hashtable.h"
#include "file.h"

/**************** Private Functions ****************/
static map_t *map_copy(map_t *map);
static bool isObstruct(char c);
static bool canPlayerMoveTo(map_t *map, position_t *pos);
static void replaceBlocked(map_t *map, map_t *outMap, player_t *player);
static void map_calcVisPath(map_t *map, char *vis, position_t *pos1, position_t *pos2);
static char *initVisStr(int width, int height);
static void intersectVis(char *vis1, char *vis2);
position_t *map_intToPos(map_t *map, int i);
static void applyVis(map_t *map, char *vis);

/**************** Iterator Functions ****************/
void addPlayerITR(void *arg, const char *key, void *item);
void placeGold(void *arg, const char *key, void *item);
void isOnGoldITR(void *arg, const char *key, void *item);

/**************** map_new ****************/
map_t *map_new(FILE *fp)
{
	map_t *map = malloc(sizeof(map_t));
	if (map == NULL){
		return NULL;
	}

    // read map *.txt file into buffer
	char *buffer = freadfilep(fp);

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
				// this shifts the line over if for every new line we have taken out
				buffer[i - offset] = buffer[i];
				width += 1;
			}
		}
		buffer[i - offset] = '\0';
	}

	map->width = width / height;
	map->height = height;

    // copy buffer into mapstring
	char *mapStr = (char*) malloc( (strlen(buffer) * sizeof(char)) + 5); 
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
		
		replaceBlocked(map, outMap, player);
		applyVis(outMap, player->visibility);
	}

	outMap->mapStr = map_buildOutput(outMap);

	return outMap;
}

/********** helper: applyVis **********/
void applyVis(map_t *map, char *vis)
{
    if (strlen(map->mapStr) != strlen(vis)) {
        // defensive programming
        printf("mapStrLen:%ld , visLen:%ld\n", strlen(map->mapStr), strlen(vis));
        printf("map width: %d, height: %d\n", map->width, map->height);
        return;
    }

    int finalPos = map->height * map->width;
    int i = 0;

    // loop through the map, converting non-visble characters to empty spaces
    for (i = 0; i < finalPos; i++) {
        if (vis[i] == '0') {
            map->mapStr[i] = ' ';
        }
    }
}

/********** helper: replaceBlocked **********/
void replaceBlocked(map_t *map, map_t *outMap, player_t *player)
{
	char *visHere = initVisStr(map->width, map->height);
	map_calculateVisibility(map, visHere, player->pos);

    if (visHere != NULL) {
        for (int i = 0; i < strlen(visHere); i++) {
			// for any gold or players that should not be currently visible,
            //  convert them to their default symbol in the map
            if (visHere[i] == '0' && (isalpha(outMap->mapStr[i]) || outMap->mapStr[i] == '*')) {
                    outMap->mapStr[i] = map->mapStr[i]; 
            }
			// Or-ing the vis strings
			if(player->visibility[i] == '1' || visHere[i] == '1'){
				player->visibility[i] = '1';
			}
        }
    }
	free(visHere);
}

/********** helper: initVisStr **********/
char *initVisStr(int width, int height)
{	
	char *vis = calloc((width * height) + 1, sizeof(char));
	for (int i = 0; i < (width * height); i++) {
		strcat(vis, "0");
	}
	return vis;
}

/********** helper: intersectVis **********/
void intersectVis(char *vis1, char *vis2)
{
    for (int i = 0; i < strlen(vis1); i++) {
        // Or-ing the vis strings
        if(vis1[i] == '1' || vis2[i] == '1'){
            vis1[i] = '1';
        }
    }
}


/**************** iterator: placeGold ****************/
void placeGold(void *arg, const char *key, void *item)
{
	map_t *outMap = arg;
	gold_t *g = item;
	if (!g->isCollected) {
		int gIndx = map_calcPosition(outMap, g->pos);
		outMap->mapStr[gIndx] = '*';
	}
}


/**************** iterator: addPlayerITR ****************/
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
	if (pos->x > map->width || pos->y > map->height || pos->x < -1 || pos->y < -1){
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
/* returned string must be freed by the caller */
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
	char *newMapStr = calloc((map->width * map->height) + 1, sizeof(char));
	strcpy(newMapStr, map->mapStr);
	newMap->mapStr = newMapStr;

	return newMap;
}


/**************** map_calculateVisibility ****************/
void map_calculateVisibility(map_t *map, char *vis, position_t *pos)
{

	position_t *newPos = malloc(sizeof(position_t));

	for (newPos->x = 0; newPos->x < map->width;  newPos->x++){
		for (newPos->y = 0; newPos->y < map->height; newPos->y++){

			// Calculating the visibility from player pos and updating visibility string
			map_calcVisPath(map, vis, pos, newPos);
		}
	}

	free(newPos);
}


/**************** map_calcVisPath ****************/
void map_calcVisPath(map_t *map, char *vis, position_t *pos1, position_t *pos2)
{

    int dx = abs(pos1->x - pos2->x);
    int dy = abs(pos1->y - pos2->y);

	position_t *newPos = malloc(sizeof(position_t));
	newPos->x = pos1->x;
	newPos->y = pos1->y;

    int i = 1 + dx + dy;
    int error = dx - dy;

	int x_dir, y_dir;
	
    // Checking direction of movement in both directions
	if (pos2->x > pos1->x){ x_dir = 1; } 
	else { x_dir = -1; }

	if (pos2->y > pos1->y){ y_dir = 1; } 
	else { y_dir = -1; }

	dx *= 2;
    dy *= 2;
	bool breakNext = false; 
    while (i > 0)
    {
        // check for visibility along this line and special
        //  cases (corners and passages)
		int indx = map_calcPosition(map,newPos);
		if (!breakNext){
			vis[indx] = '1';
		} 
		else if (map->mapStr[indx] == '+'){
			vis[indx] = '1';
			break;
		} else if (map->mapStr[indx] == '#' && map->mapStr[map_calcPosition(map,pos1)] == '#'){
			vis[indx] = '1';
			break;
		}
		else {
			break;
		}
		
		if(isObstruct(map->mapStr[indx])){
			breakNext = true;
		} 
		
        if (error > 0){
            newPos->x += x_dir;
            error -= dy;
        }
        else{
            newPos->y += y_dir;
            error += dx;
        }

		i -= 1;
    }
	free(newPos);
}


/**************** isObstruct ****************/
bool isObstruct(char c)
{
	if (c == '+' || c == '-' || c == '|' || c == '#' || c == ' '){
		return true;
	}
	return false;
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

			char *visHere = initVisStr(map->width, map->height);
            map_calculateVisibility(map,visHere, player->pos);
            intersectVis(player->visibility, visHere);
            free(visHere);
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

			char *visHere = initVisStr(map->width, map->height);
            map_calculateVisibility(map,visHere, player->pos);
            intersectVis(player->visibility, visHere);
            free(visHere);

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

			char *visHere = initVisStr(map->width, map->height);
            map_calculateVisibility(map,visHere, player->pos);
            intersectVis(player->visibility, visHere);
            free(visHere);

		}
	}

	if(player->pos->x < 0){ player->pos->x = -1; }
	if(player->pos->y < 0){ player->pos->y = 0; }

	if(player->pos->x >= map->width - 1){ player->pos->x = map->width - 2; }
	if(player->pos->y >= map->height){ player->pos->y = map->height - 1; }

	// set nextPos x and y to check if the player moved
	nextPos->x = player->pos->x;
	nextPos->y = player->pos->y;

	free(newPos);
	return;
}


/**************** canPlayerMoveTo ****************/
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


/********** iterator: isOnGoldITR **********/
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
