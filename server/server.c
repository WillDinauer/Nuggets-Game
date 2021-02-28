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
    char *map;
    int NR;
    int NC;
    addr_t specAddr;
} serverInfo_t;

typedef struct player {
    addr_t addr;
    int x;
    int y;
    int gold;
    char letter;
    bool isActive;
    char *visibility;
} player_t;

typedef struct pos {
    int x;
    int y;
} pos_t;


int server(char *argv[], int seed);
static bool handleMessage(void *arg, const addr_t from, const char *message);
void sendMaps(serverInfo_t *info);
bool validateAction(char *keyPress, addr_t from, serverInfo_t *info);
void splitline(char *message, char *words[]);
player_t *player_new(addr_t from, char letter, serverInfo_t *info);
pos_t *newPlayerPos(serverInfo_t *info);
counters_t *getDotsPos(char *map);
bool validateParameters(int argc, char *argv[], int *seed);
void sendGrid(const addr_t from, int NR, int NC);
void sendSpectatorView(serverInfo_t *info);
void mapSend(void *arg, const char* key, void *item);

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
    int NC = 0;
    int NR = 0;
    char *map = "    ";
    addr_t specAddr = message_noAddr();
    //char *map = map_new(argv[1]);
    if (map == NULL) {
        fprintf(stderr, "unable to load map");
        return 2;
    }

    counters_t *dotsPos = getDotsPos(map);
    serverInfo_t info = {&numPlayers, maxPlayers, playerInfo, dotsPos, map, NR, NC, specAddr};
    
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

    char *words[2];
    splitline(line, words);
    if (strcmp(words[0], "PLAY") == 0) {
        if (*numPlayers == maxPlayers) {
            message_send(from, "QUIT Game is full: no more players can join");
        } else {
            //TODO: Add check for blank player name
            char letter = 'A' + *numPlayers;
            player_t *player = player_new(from, letter, info);
            if (hashtable_insert(playerInfo, words[1], player)) {
                (*numPlayers)++;
                sendGrid(from, info->NR, info->NC);
                sendMaps(info);
            }
        }
    } else if (strcmp(words[0], "KEY") == 0) {
        if (validateAction(words[1], from, info)) {
            sendMaps(info);
        }
    } else if (strcmp(words[0], "SPECTATE") == 0) {
        addr_t specAddr = info->specAddr;
        if (message_isAddr(specAddr)) {
            message_send(specAddr, "QUIT You have been replaced by a new spectator.");
        }
        info->specAddr = from;
        sendSpectatorView(info);
    }
    free(line);
    return false;
}

void sendGrid(const addr_t from, int NR, int NC)
{
    int NRlen = snprintf(NULL, 0, "%d", NR);
    int NClen = snprintf(NULL, 0, "%d", NC);
    char *NRstr = malloc(NRlen + 1);
    char *NCstr = malloc(NClen + 1);
    snprintf(NRstr, NRlen + 1, "%d", NR);
    snprintf(NCstr, NClen + 1, "%d", NC);

    char *message = malloc(NRlen + NClen + 7);
    strcpy(message, "GRID ");
    strcat(message, NRstr);
    strcat(message, " ");
    strcat(message, NCstr);
    message_send(from, message);

    free(NRstr);
    free(NCstr);
    free(message);
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
    //TODO: Construct the spectator's map

    char *specMap = "spectator map";
    int len = strlen(specMap);

    char *message = malloc(len + 9);
    strcpy(message, "DISPLAY\n");
    strcat(message, specMap);
    message_send(specAddr, message);

    free(message);
}

void mapSend(void *arg, const char* key, void *item)
{
    //serverInfo_t *info = (serverInfo_t *)arg;
    player_t *player = (player_t *)item;
    //TODO: Construct the map for the individual user
    
    char* newMap = "player map";
    int len = strlen(newMap);

    char *message = malloc(len + 9);
    strcpy(message, "DISPLAY\n");
    strcat(message, newMap);
    addr_t addr = player->addr;
    message_send(addr, message);

    free(message);
}

bool validateAction(char *keyPress, addr_t from, serverInfo_t *info)
{
    //TODO: Validate action based on the map, update playerinfo accordingly
    return true;
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

    pos_t *pos = newPlayerPos(info);

    player->x = pos->x;
    player->y = pos->y;

    return player;
}

pos_t *newPlayerPos(serverInfo_t *info)
{
    int x = 0;
    int y = 0;
    pos_t *pos = malloc(sizeof(pos_t));
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
