/*
 * server.c - 'server' module for the Nuggets Game project
 *
 * Dartmouth CS50, Winter 2021
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
#include "serverUtils.h"


struct findPlayer {
	addr_t addr;
	player_t *result;
};

/**************** Functions ****************/
int server(char *argv[], int seed);
void splitline(char *message, char *words[]);
player_t *player_new(addr_t from, char letter, serverInfo_t *info);
position_t *newPlayerPos(serverInfo_t *info);
counters_t *getDotsPos(char *map);
bool validateParameters(int argc, char *argv[], int *seed);
bool checkFile(char *fname, char *openParam);


/**************** Server Communication Functions ****************/
void sendGoldMessage(const addr_t from, serverInfo_t *info);
void sendInitialInfo(const addr_t from, int NR, int NC, char letter);
void sendSpectatorView(serverInfo_t *info);
static bool handleMessage(void *arg, const addr_t from, const char *message);
void sendMaps(serverInfo_t *info);
void mapSend(void *arg, const char* key, void *item);

/**************** Iterators ****************/
void findPlayerITR(void *arg, const char *key, void *item);

int main(int argc, char *argv[])
{
	int seed;
	if (!validateParameters(argc, argv, &seed)) {
		return 1;
	}

	return server(argv, seed);
}

int server(char *argv[], int seed)
{
	//static const int MaxNameLength = 50;   // max number of chars in playerName
	static const int maxPlayers = 26;      // maximum number of players
	//static const int GoldTotal = 250;      // amount of gold in the game
	//static const int GoldMinNumPiles = 10; // minimum number of gold piles
	//static const int GoldMaxNumPiles = 30; // maximum number of gold piles
	int numPlayers = 0;
	hashtable_t *playerInfo = hashtable_new(maxPlayers);
	addr_t specAddr = message_noAddr();

	// read the map file to create the map
	FILE *fp;
	int len = strlen(argv[1]);
	char *mapfile = calloc(len + 1, sizeof(char));
	strcpy(mapfile, argv[1]);
	fp = fopen(mapfile, "r");
	map_t *map = map_new(fp);
	free(mapfile);

	if (map == NULL) {
		fprintf(stderr, "unable to load map");
		return 2;
	}

	counters_t *dotsPos = getDotsPos(map->mapStr);
	serverInfo_t info = {&numPlayers, maxPlayers, playerInfo, dotsPos, map, specAddr};
	
	log_init(stderr);
	int serverPort = message_init(stderr);
	if (serverPort == 0) {
		return 3;
	}
	printf("waiting for connections on port %d\n", serverPort);

	message_loop(&info, 0, NULL, NULL, handleMessage);
	message_done();
	log_done();
	//TODO: add hashtable delete
	counters_delete(dotsPos);
	return 0;
}

static bool handleMessage(void *arg, const addr_t from, const char *message)
{
	serverInfo_t *info = (serverInfo_t *)arg;
	if (info == NULL) {
		log_v("handleMessage called with arg=NULL");
		return true;
	}
	hashtable_t *playerInfo = info->playerInfo;
	int *numPlayers = info->numPlayers;
	const int maxPlayers = info->maxPlayers;

	char *line = calloc(strlen(message) + 1, sizeof(char));
	strcpy(line, message);


	// Finding player from address
	struct findPlayer *f = malloc(sizeof(struct findPlayer));
	f->addr = from;
	hashtable_iterate(info->playerInfo, f, findPlayerITR);
	// Player that sent command
	player_t *fromPlayer = f->result;
	free(f);

	char *words[2];
	splitline(line, words);
	if (strcmp(words[0], "PLAY") == 0) {
		if (*numPlayers == maxPlayers) {
			message_send(from, "QUIT Game is full: no more players can join");
		} else {
			//TODO: Add check for blank player name
			char letter = 'A' + *numPlayers;
			player_t *newPlayer = player_new(from, letter, info);
			if (hashtable_insert(playerInfo, words[1], newPlayer)) {
				(*numPlayers)++;
				sendInitialInfo(from, info->map->height, info->map->width, letter);
				sendMaps(info);
			}
		}
	} else if (strcmp(words[0], "KEY") == 0) {
		if (validateAction(words[1], fromPlayer, info)) {
			sendMaps(info);
		}
	} else if (strcmp(words[0], "SPECTATE") == 0) {
		addr_t specAddr = info->specAddr;
		if (message_isAddr(specAddr)) {
			message_send(specAddr, "QUIT You have been replaced by a new spectator.");
		}
		info->specAddr = from;
		sendInitialInfo(from, info->map->height, info->map->width, 's');
		sendSpectatorView(info);
	}

	free(line);
	return false;
}

void sendInitialInfo(const addr_t from, int NR, int NC, char letter)
{
	int NRlen = snprintf(NULL, 0, "%d", NR);
	int NClen = snprintf(NULL, 0, "%d", NC);
	char *NRstr = malloc(NRlen + 1);
	char *NCstr = malloc(NClen + 1);
	snprintf(NRstr, NRlen + 1, "%d", NR);
	snprintf(NCstr, NClen + 1, "%d", NC);

	// send the "GRID NR NC" message to the client
	char *message = malloc(NRlen + NClen + 7);
	if (message != NULL && NRstr != NULL && NCstr != NULL) {
		strcpy(message, "GRID ");
		strcat(message, NRstr);
		strcat(message, " ");
		strcat(message, NCstr);
		message_send(from, message);
		free(NRstr);
		free(NCstr);
		free(message);
	}

	if (letter != 's') {    // indicates whether the client is a spectator or a player
	// send the "OK L" message to the client
		char *letterMessage = malloc(5);
		if (letterMessage != NULL) {
			strcpy(letterMessage, "OK ");
			letterMessage[3] = letter;
			letterMessage[4] = '\0';
			message_send(from, letterMessage);
			free(letterMessage);
		}
	}

	//TODO: general gold messages
	char *goldMessage = malloc(13);
	if (goldMessage != NULL) {
		strcpy(goldMessage, "GOLD 0 0 200");
		message_send(from, goldMessage);
		free(goldMessage);
	}
}

void sendGoldMessage(const addr_t from, serverInfo_t *info)
{
	char *goldMessage = malloc(13);
	if (goldMessage != NULL) {
		strcpy(goldMessage, "GOLD 0 0 200");
		message_send(from, goldMessage);
		free(goldMessage);
	}
}

void sendMaps(serverInfo_t *info)
{
	hashtable_t *playerInfo = info->playerInfo;
	hashtable_iterate(playerInfo, info, mapSend);

	if (message_isAddr(info->specAddr)) {
		sendSpectatorView(info);
	}
}

void sendSpectatorView(serverInfo_t *info)
{
	addr_t specAddr = info->specAddr;

	map_t *baseMap = info->map;
	map_t *specMap = map_buildPlayerMap(baseMap, NULL, NULL, info->playerInfo);
	int len = strlen(specMap->mapStr);

	char *message = malloc(len + 9);
	strcpy(message, "DISPLAY\n");
	strcat(message, specMap->mapStr);
	message_send(specAddr, message);

	free(message);
	map_delete(specMap);
}

void mapSend(void *arg, const char* key, void *item)
{
	serverInfo_t *info = (serverInfo_t *)arg;
	player_t *player = (player_t *)item;
	hashtable_t *playerInfo = info->playerInfo;
	//TODO: Construct the map for the individual user
	
	map_t *baseMap = info->map;
	map_t *playerMap = map_buildPlayerMap(baseMap, player, NULL, playerInfo); 
	int len = strlen(playerMap->mapStr);

	char *message = malloc(len + 9);
	strcpy(message, "DISPLAY\n");
	strcat(message, playerMap->mapStr);
	addr_t addr = player->addr;
	message_send(addr, message);

	free(message);
	map_delete(playerMap);
}


void splitline(char *message, char *words[])
{
	words[0] = &message[0];
	int pos = 0;
	while (!isspace(message[pos]) && message[pos] != '\0') {
		pos++;
	}
	if (message[pos] != '\0') {
		message[pos] = '\0';
		words[1] = &message[pos+1];
	}
}

player_t *player_new(addr_t from, char letter, serverInfo_t *info)
{
	player_t *player = malloc(sizeof(player_t));
	if (player == NULL) { // out of memory
		return NULL;
	} 
	player->addr = from;
	player->letter = letter;
	player->isActive = true;
	player->gold = 0;
	player->visibility = "";    //TODO: Visibility

	player->pos = newPlayerPos(info);

	return player;
}

position_t *newPlayerPos(serverInfo_t *info)
{
	int x = 5;
	int y = 3;
	position_t *pos = malloc(sizeof(position_t));
	//TODO: check existing player positions and gold positions and choose from remaining empty "." spaces to generate random x/y
	pos->x = x;
	pos->y = y;
	return pos;
}

counters_t *getDotsPos(char *map)
{
	counters_t *dotsPos = counters_new();
	int pos;
	int len = strlen(map);
	for (pos = 0; pos < len; pos++) {
		if (map[pos] == '.') {
			counters_add(dotsPos, pos);
		}
	}
	return dotsPos;
}

bool validateParameters(int argc, char *argv[], int *seed)
{
	// validate number of arguments
	if (argc < 2 || argc > 3) {
		fprintf(stderr, "usage: ./server map.txt [seed]\n");
		return false;
	}
	
	// validate the map file (ensure it is readable)
	if (!checkFile(argv[1], "r")) {
		fprintf(stderr, "map file '%s' is not a readable file\n", argv[1]);
		return false;
	}

	// validate seed, if provided
	if (argc == 3) {
		char val;
		if ((sscanf(argv[2], "%d%c", seed, &val)) != 1) {      // ensures optional seed parameter is solely an integer
			fprintf(stderr, "%s is not a valid integer\n", argv[3]);
			return false;
		} 
	}

	return true;
}

bool checkFile(char *fname, char *openParam)
{
	FILE *fp;
	int fileLen = strlen(fname);              // length of the file's name
	char *filename = calloc(fileLen + 1, sizeof(char));    // allocate memory to hold file plus '\0'
	if (filename == NULL) { // error allocating memory
		return false;
	}
	strcpy(filename, fname);        // concatenate the filename
	// try to open the file based on the openParam; on success clean-up and return true
	if ((fp = fopen(filename, openParam))) {
		fclose(fp);
		free(filename);
		return true;
	}
	// return false if the file could not be opened
	free(filename);
	return false;
}




void findPlayerITR(void *arg, const char *key, void *item)
{
	struct findPlayer *fp = arg;
	player_t *p = item;

	if (message_eqAddr(p->addr, fp->addr)){
		fp->result = p;
	}
}
