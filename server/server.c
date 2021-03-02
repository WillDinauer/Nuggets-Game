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
#include <unistd.h>
#include "map.h"
#include "message.h"
#include "log.h"
#include "hashtable.h"
#include "set.h"
#include "counters.h"
#include "serverUtils.h"


typedef struct countersMap {
    counters_t *ctrs;
    map_t *map;
} ctrsmap_t;

typedef struct twoctrs {
    counters_t *ctrs1;
    counters_t *ctrs2;
} twoctrs_t;

typedef struct twoints {
    int *x;
    int *y;
} twoints_t;

struct findPlayer {
	addr_t addr;
	player_t *result;
};

/**************** Functions ****************/
int server(char *argv[], int seed);
void splitline(char *message, char *words[]);
player_t *player_new(addr_t from, char letter, serverInfo_t *info);
counters_t *getDotsPos(char *map);
bool validateParameters(int argc, char *argv[], int *seed);
bool checkFile(char *fname, char *openParam);
void findPos(void *arg, int key, int count);
void goldFill(void *arg, const char *key, void *item);
void playerFill(void *arg, const char *key, void *item);
void onlyDots(void *arg, int key, int count);
void keyCount(void *arg, int key, int count);
hashtable_t *generateGold(map_t *map, int seed, int *goldCt, counters_t *dotsPos);
position_t *getRandomPos(map_t *map, counters_t *dotsPos, hashtable_t *goldInfo, hashtable_t *playerInfo);
gold_t *gold_new();


/**************** Server Communication Functions ****************/
void sendInitialInfo(const addr_t from, serverInfo_t *info, char letter);
void sendSpectatorView(serverInfo_t *info);
static bool handleMessage(void *arg, const addr_t from, const char *message);
void sendMaps(serverInfo_t *info);
void mapSend(void *arg, const char* key, void *item);
void sendGoldMessage(addr_t from, int collected, int purse, int remain);


/**************** Iterators ****************/
void findPlayerITR(void *arg, const char *key, void *item);


int main(int argc, char *argv[])
{
    int seed = -1;
    if (!validateParameters(argc, argv, &seed)) {
        return 1;
    }

	return server(argv, seed);
}

int server(char *argv[], int seed)
{
    //static const int MaxNameLength = 50;   // max number of chars in playerName
    static const int maxPlayers = 26;      // maximum number of players
  
    int goldCt = 0;
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
    hashtable_t *goldData = generateGold(map, seed, &goldCt, dotsPos);

    serverInfo_t info = {&numPlayers, &goldCt, maxPlayers, playerInfo, goldData, dotsPos, map, specAddr};
    
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

hashtable_t *generateGold(map_t *map, int seed, int *goldCt, counters_t *dotsPos)
{
    static const int GoldTotal = 250;      // amount of gold in the game
    static const int GoldMinNumPiles = 10; // minimum number of gold piles
    static const int GoldMaxNumPiles = 30; // maximum number of gold piles

    if (seed == -1) {
        srand(getpid());
    } else {
        srand(seed);
    }

    *goldCt = GoldTotal;
    int goldToPlace = GoldTotal;
    int numPiles = 0;
    hashtable_t *goldInfo = hashtable_new(GoldMinNumPiles);
    
    while (goldToPlace != 0) {
        gold_t *gold = gold_new();

        // generate gold for a pile to ensure min num piles, and a pile has at least 1 gold
        int value = (rand() % GoldTotal/GoldMinNumPiles) + 1; 
        // generate a random position for the gold
        position_t *pos = getRandomPos(map, dotsPos, goldInfo, NULL);
        //TODO: HANDLE NULL POS: THERE ARE NO MORE VALID POSITIONS FOR GOLD (might have to make a nextPos)

        if (goldToPlace-value < 0 || numPiles+1 == GoldMaxNumPiles) {
            value = goldToPlace;
            goldToPlace = 0;
        } else {
            goldToPlace -= value;
        }

        gold->value = value;
        gold->pos = pos;

        int npLen = snprintf(NULL, 0, "%d", numPiles);
        char *numPileStr = malloc(npLen + 1);
        snprintf(numPileStr, npLen + 1, "%d", numPiles);

        hashtable_insert(goldInfo, numPileStr, gold);

        free(numPileStr);
        numPiles++;

    }
    return goldInfo;
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
				sendInitialInfo(from, info, letter);
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
		sendInitialInfo(from, info, 's');
		sendSpectatorView(info);
	}

	free(line);
	return false;
}

void sendInitialInfo(const addr_t from, serverInfo_t *info, char letter)
{
    int NR = info->map->height;
    int NC = info->map->width;

    int NRlen = snprintf(NULL, 0, "%d", NR);
    int NClen = snprintf(NULL, 0, "%d", NC);
    char *NRstr = malloc(NRlen + 1);
    if (NRstr == NULL) { // out of memory
        return;
    }
    char *NCstr = malloc(NClen + 1);
    if (NCstr == NULL) { // out of memory
        free(NRstr);
        return;
    }
    snprintf(NRstr, NRlen + 1, "%d", NR);
    snprintf(NCstr, NClen + 1, "%d", NC);

    // send the "GRID NR NC" message to the client
    char *message = malloc(NRlen + NClen + 7);
    if (message != NULL) {
        strcpy(message, "GRID ");
        strcat(message, NRstr);
        strcat(message, " ");
        strcat(message, NCstr);
        message_send(from, message);
        free(message);
    }
    free(NRstr);
    free(NCstr);

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

    // send the initial gold message
    sendGoldMessage(from, 0, 0, *info->goldCt);
}

void sendGoldMessage(addr_t address, int collected, int purse, int remain)
{
    int clen = snprintf(NULL, 0, "%d", collected);
    int plen = snprintf(NULL, 0, "%d", purse);
    int rlen = snprintf(NULL, 0, "%d", remain);
    char *cstr = malloc(clen + 1);
    if (cstr == NULL) { // out of memory
        return;
    }
    char *pstr = malloc(plen + 1);
    if (pstr == NULL) { // out of memory
        free(cstr);
        return;
    }
    char *rstr = malloc(rlen + 1);
    if (rstr == NULL) { // out of memory
        free(cstr);
        free(pstr);
        return;
    }
    snprintf(cstr, clen + 1, "%d", collected);
    snprintf(pstr, plen + 1, "%d", purse);
    snprintf(rstr, rlen + 1, "%d", remain);

    // send the "GOLD n p r" message to the client
    char *message = malloc(clen + plen + rlen + 8); 
    if (message != NULL) {
        strcpy(message, "GOLD ");
        strcat(message, cstr);
        strcat(message, " ");
        strcat(message, pstr);
        strcat(message, " ");
        strcat(message, rstr);

        message_send(address, message);
        free(message);
    }
    free(cstr);
    free(rstr);
    free(pstr);
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
    map_t *specMap = map_buildPlayerMap(baseMap, NULL, info->goldData, info->playerInfo);
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
    hashtable_t *goldData = info->goldData;
    
    map_t *baseMap = info->map;
    map_t *playerMap = map_buildPlayerMap(baseMap, player, goldData, playerInfo); 
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

    player->pos = getRandomPos(info->map, info->dotsPos, info->goldData, info->playerInfo);

    //TODO: HANDLE POS == NULL (player must quit)

    return player;
}

position_t *getRandomPos(map_t *map, counters_t *dotsPos, hashtable_t *goldInfo, hashtable_t *playerInfo)
{
    counters_t *filledPos = counters_new();
    if (filledPos == NULL) { // out of memory
        return NULL;
    }
    ctrsmap_t bundle = {filledPos, map};

    // iterate over gold, adding all occupied gold positions to filledPos
    hashtable_iterate(goldInfo, &bundle, goldFill);
    if (playerInfo != NULL) {
        // iterate over players, adding all occupied player positions to filledPos
        hashtable_iterate(playerInfo, &bundle, playerFill);
    }
    counters_t *validPositions = counters_new();
    if (validPositions == NULL) { // out of memory
        return NULL;
    }
    twoctrs_t ctrs = {filledPos, validPositions};
    counters_iterate(dotsPos, &ctrs, onlyDots);

    int numValidPos = 0;
    counters_iterate(validPositions, &numValidPos, keyCount);

    if (numValidPos != 0) {
        int val = rand() % numValidPos;
        int finalPos = 0;

        twoints_t t = {&val, &finalPos};
        counters_iterate(validPositions, &t, findPos);

        position_t *result = map_intToPos(map, finalPos);

        free(filledPos);
        free(validPositions);
        return result;
    } else {
        return NULL;
    }
}

void findPos(void *arg, int key, int count)
{
    twoints_t *t = arg;
    int *val = t->x;
    int *finalPos = t->y;

    if (*val != -1) {
        if (*val == *finalPos) {
            *finalPos = key;
            *val = -1;
        } else {
            (*finalPos)++;
        }
    }
}

void goldFill(void *arg, const char *key, void *item)
{
    ctrsmap_t *bundle = arg;
    counters_t *filled = bundle->ctrs;
    map_t *map = bundle->map;
    gold_t *gold = item;
    position_t *pos = gold->pos;

    int val = map_calcPosition(map, pos);

    counters_add(filled, val);
}

void playerFill(void *arg, const char *key, void *item)
{
    ctrsmap_t *bundle = arg;
    counters_t *filled = bundle->ctrs;
    map_t *map = bundle->map;
    player_t *player = item;
    position_t *pos = player->pos;

    int val = map_calcPosition(map, pos);

    counters_add(filled, val);
}

void onlyDots(void *arg, int key, int count)
{
    twoctrs_t *ctrs = arg;
    counters_t *filled = ctrs->ctrs1;
    counters_t *validPos = ctrs->ctrs2;

    if (counters_get(filled, key) == 0) {
        counters_add(validPos, key);
    }
}

void keyCount(void *arg, int key, int count)
{
    int *nkeys = arg;
    if (nkeys != NULL && key >= 0 && count >=0) {
        (*nkeys)++;
    }
}

gold_t *gold_new()
{
    gold_t *gold = malloc(sizeof(gold_t));
    if (gold == NULL) { // out of memory
        return NULL;
    }
    gold->value = 0;
    gold->isCollected = false;
    gold->pos = NULL;

    return gold;
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
