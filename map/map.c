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
#include "map.h"

/**************** Private Functions ****************/
map_t *map_copy(map_t *map);


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
map_t *map_buildPlayerMap(map_t *map, player_t *player, gold_t **goldArr)
{
	map_t *outMap = map_copy(map);

	// Adding player to map
	int plyIndx = map_calcPosition(outMap, player->pos);
	outMap->mapStr[plyIndx] = '@';

	gold_t *g;
	int i = 0;

	if (goldArr != NULL){
		while ((g = goldArr[i]) != NULL){
			if (g->isCollected == false){
				int gIndx = map_calcPosition(outMap, g->pos);
				outMap->mapStr[gIndx] = '*';
			}
			i++;
		} 
	}


	return outMap;
}


/**************** map_calcPosition ****************/
int map_calcPosition(map_t *map, position_t *pos)
{
	if (pos->x > map->width || pos->y > map->height || pos->x < 0 || pos->y < 0){
		return -1;
	}

	return (pos->y * map->width) + (pos->x + 1);
}


// /**************** buildMap ****************/
// map_t *map_buildOutput(map_t *map)
// {


// }


// /**************** map_placeGold ****************/
// map_t *map_placeGold(map_t *map, gold_t **goldArr)
// {

// }



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

void map_delete(map_t *map)
{

}