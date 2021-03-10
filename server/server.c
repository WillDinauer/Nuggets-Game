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
#include "file.h"
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

typedef struct playerBundle {
    position_t *pos1;
    position_t *pos2;
    addr_t addr;
} pb_t;

typedef struct twoints {
    int *x;
    int *y;
} twoints_t;

struct findPlayer {
	addr_t addr;
	player_t *result;
};

typedef struct goldBundle {
    player_t *player;
    int *goldCt;
} gb_t;

/**************** Functions ****************/
int server(char *argv[], int seed);
void splitline(char *message, char *words[]);
player_t *player_new(addr_t from, char letter, serverInfo_t *info);
counters_t *getDotsPos(char *map);
bool validateParameters(int argc, char *argv[], int *seed);
bool checkFile(char *fname, char *openParam);
hashtable_t *generateGold(map_t *map, int seed, int *goldCt, counters_t *dotsPos);
position_t *getRandomPos(map_t *map, counters_t *dotsPos, hashtable_t *goldInfo, hashtable_t *playerInfo);
gold_t *gold_new();


/**************** Server Communication Functions ****************/
void sendInitialInfo(const addr_t from, serverInfo_t *info, char letter);
void sendSpectatorView(serverInfo_t *info);
static bool handleInput(void *arg);
static bool handleMessage(void *arg, const addr_t from, const char *message);
void sendMaps(serverInfo_t *info);
void sendQuit(serverInfo_t *info);
void sendGoldMessage(addr_t from, int collected, int purse, int remain);


/**************** Iterators ****************/
void findPlayerITR(void *arg, const char *key, void *item);
void buildGameOverString(void *arg, const char *key, void *item);
void mapSend(void *arg, const char* key, void *item);
void quitFunc(void *arg, const char *key, void *item);
void searchActivePlayers(void *arg, const char *key, void *item);
void checkPlayerCollision(void *arg, const char *key, void *item);
void sendOthersGold(void *arg, const char *key, void *item);
void goldFill(void *arg, const char *key, void *item);
void playerFill(void *arg, const char *key, void *item);
void recountGold(void *arg, const char *key, void *item);
void findPos(void *arg, int key, int count);
void onlyDots(void *arg, int key, int count);
void keyCount(void *arg, int key, int count);
void playerDelete(void *item);
void goldDelete(void *item);


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
    // initialize variables to be stored as the server information
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
    fclose(fp);
    free(mapfile);


    if (map == NULL) {
        fprintf(stderr, "unable to load map");
        return 2;
    }
    // create the counters which holds the integer positions of '.' in the map
    counters_t *dotsPos = getDotsPos(map->mapStr);
    // generate the gold randomly (or based on the seed) and store in a hashtable
    hashtable_t *goldData = generateGold(map, seed, &goldCt, dotsPos);

    // construct the serverInfo object which holds all the relevant data for the server
    serverInfo_t info = {&numPlayers, &goldCt, maxPlayers, playerInfo, goldData, dotsPos, map, specAddr};
    
    // start logging
    log_init(stderr);
    // initialize messages; listen on a port
    int serverPort = message_init(stderr);
    if (serverPort == 0) {
        return 3;
    }
    printf("waiting for connections on port %d\n", serverPort);

    // continue looping, listening for messages until the end of the game is triggered
    message_loop(&info, 0, NULL, handleInput, handleMessage);

    // clean up
    message_done();
    log_done();
    map_delete(map);
    hashtable_delete(playerInfo, playerDelete);
    hashtable_delete(goldData, goldDelete);
    counters_delete(dotsPos);
    return 0;
}

static bool handleInput(void *arg)
{
    char *line;
    if ((line = freadlinep(stdin)) == NULL) {
        return true;
    }
    return false;
}

hashtable_t *generateGold(map_t *map, int seed, int *goldCt, counters_t *dotsPos)
{
    static const int GoldTotal = 250;      // amount of gold in the game
    static const int GoldMinNumPiles = 10; // minimum number of gold piles
    int GoldMaxNumPiles = 30; // maximum number of gold piles

    // randomize either based on the seed or the pid
    if (seed == -1) {
        srand(getpid());
    } else {
        srand(seed);
    }

    // if there are less dots than the max possible piles, 
    // allow for a maximum number of piles equal to the number of dots minus one, allowing one space for a player. 
    int numDots = 0;
    counters_iterate(dotsPos, &numDots, keyCount);
    if (numDots <= GoldMaxNumPiles) {
        GoldMaxNumPiles = numDots-1;
    }

    *goldCt = GoldTotal;            // tracks the uncollected gold in the server
    int goldToPlace = GoldTotal;    // amount of gold to place into the map
    int numPiles = 0;               // total number of gold piles; must ultimately be between goldMin to goldMax piles
    hashtable_t *goldInfo = hashtable_new(GoldMinNumPiles);     // initialize the goldInfo hashtable to store gold data
    
    while (goldToPlace != 0) {      // loop until all gold placed
        gold_t *gold = gold_new();  // create the new pile of gold to be placed

        // generate gold for a pile to ensure min num piles, and a pile has at least 1 gold
        int value = (rand() % GoldTotal/GoldMinNumPiles) + 1; 
        // generate a random position for the gold (must be an unoccupied '.' character)
        position_t *pos = getRandomPos(map, dotsPos, goldInfo, NULL);

        // if the random value is less than the remaining gold OR we have reached the max number of piles...
        if (goldToPlace-value < 0 || numPiles+1 == GoldMaxNumPiles) {
            value = goldToPlace;        // set the value to the remaining gold to place
            goldToPlace = 0;            // no more gold left to place!
        } else {
            goldToPlace -= value;       // otherwise, subtract the value from the remaining gold to place
        }

        gold->value = value;
        gold->pos = pos;

        // convert the pile number into a string
        int npLen = snprintf(NULL, 0, "%d", numPiles);
        char *numPileStr = malloc(npLen + 1);
        snprintf(numPileStr, npLen + 1, "%d", numPiles);

        // insert the gold into the hashtable
        hashtable_insert(goldInfo, numPileStr, gold);

        free(numPileStr);
        numPiles++;     // increment the number of piles

    }
    return goldInfo;    // return the hashtable with the newly generated gold data
}

static bool handleMessage(void *arg, const addr_t from, const char *message)
{
	serverInfo_t *info = (serverInfo_t *)arg;
	if (info == NULL) {     // defensive programming
		log_v("handleMessage called with arg=NULL");
		return true;
	}
	hashtable_t *playerInfo = info->playerInfo;
	int *numPlayers = info->numPlayers;
	const int maxPlayers = info->maxPlayers;

    // create memory to pass the message to the splitline function
	char *line = calloc(strlen(message) + 1, sizeof(char));
	strcpy(line, message);

    // split the message into an array of two words (a message from the client is always 1-2 words)
	char *words[2];
	splitline(line, words);


    // call the appropriate function relevant to the first word provided by the client
    // new player
	if (strcmp(words[0], "PLAY") == 0) {
        // validate the number of players
		if (*numPlayers == maxPlayers) {
			message_send(from, "QUIT Game is full: no more players can join");
		} else {
			//TODO: Add check for blank player name

            // create a new player
			char letter = 'A' + *numPlayers;                // set the letter based on the number of players, starting at 'A'
			player_t *newPlayer = player_new(from, letter, info);
            if (newPlayer == NULL || newPlayer->pos == NULL) {
                message_send(from, "QUIT no available spaces in the game, sorry!");
            } else {
                if (hashtable_insert(playerInfo, words[1], newPlayer)) { // check for duplicate player name
                    (*numPlayers)++;
                    // send the necessary initial info to the new player
				    sendInitialInfo(from, info, letter);
                    // send the map with the added new player to all clients
				    sendMaps(info);
			    }
            }
		}
    // key press from player or spectator
	} else if (strcmp(words[0], "KEY") == 0) {

        // Finding player from address
	    struct findPlayer *f = malloc(sizeof(struct findPlayer));
	    f->addr = from;
	    hashtable_iterate(info->playerInfo, f, findPlayerITR);
	    // Player that sent command
	    player_t *fromPlayer = f->result;
	    free(f);

        int prevGold = 0;
        // Keeping track of prev gold to find the amount of gold collected on a move
        if (!message_eqAddr(from, info->specAddr)){
            prevGold = fromPlayer->gold;
        }
        

        // handle quit
        if (words[1][0] == 'Q') {
            if (message_eqAddr(from, info->specAddr)) { // quit message is from the spectator
                // set the specAddr to be noAddr
                info->specAddr = message_noAddr();
                // send a quit message to the spectator
                message_send(from, "QUIT Thanks for watching!");
            } else { 
                // the player is no longer active; they should not be displayed on the map
                fromPlayer->isActive = false;
                // send a quit message to the player
                message_send(from, "QUIT Thanks for playing!");

                bool activePlayers = false;
                hashtable_iterate(info->playerInfo, &activePlayers, searchActivePlayers);
                if (!activePlayers && !message_isAddr(info->specAddr)) {
                    return true;
                } else {
                    // send the updated maps to all clients
                    sendMaps(info);
                }
            }
        } else {
            // track the current position of the player before they move
            position_t *prePos = malloc(sizeof(position_t));
            prePos->x = fromPlayer->pos->x;
            prePos->y = fromPlayer->pos->y;
            if (prePos == NULL) {   // out of memory
                free(line);
                return true;
            }

            if (validateAction(words[1], fromPlayer, info)) {   // validate the input action of the player
                hashtable_t *goldData = info->goldData;
                
                // check if the player has collided with another player
                pb_t pb = {prePos, fromPlayer->pos, from};
                hashtable_iterate(info->playerInfo, &pb, checkPlayerCollision);

                // Recount gold availability         
                *info->goldCt = 0;       
                hashtable_iterate(goldData, info, recountGold);
               
                int justReceived = fromPlayer->gold - prevGold;

                if (justReceived > 0) {
                  gb_t goldBundle = {fromPlayer, info->goldCt};

                  // send the gold message to the player
                  sendGoldMessage(from, justReceived, fromPlayer->gold, *info->goldCt);
                  // send updated gold messages to other existing players...
                  hashtable_iterate(info->playerInfo, &goldBundle, sendOthersGold);
                  // send the gold message to the spectator (if there is one)
                  if (message_isAddr(info->specAddr)) {
                      sendGoldMessage(info->specAddr, 0, 0, *info->goldCt);
                  }
                }

                // if the gold remaining in the game has reached 0, send the game over screen to all clients
                if(*info->goldCt == 0) {
                    sendQuit(info);
                    free(line);
                    free(prePos);
                    return true;
                } else {
                    // otherwise, send the updated maps as usual
                    sendMaps(info);
                }
            }
            free(prePos);
		}
    // new spectator
	} else if (strcmp(words[0], "SPECTATE") == 0) {
		addr_t specAddr = info->specAddr;

        // check for an existing spectator
		if (message_isAddr(specAddr)) {
			message_send(specAddr, "QUIT You have been replaced by a new spectator.");
		}

        // update the spectator information
		info->specAddr = from;
        // send the new spectator the initial info they need
		sendInitialInfo(from, info, 's');
        // send the spectator the map
		sendSpectatorView(info);
	}

	free(line);
	return false;
}

void sendInitialInfo(const addr_t from, serverInfo_t *info, char letter)
{
    if (letter != 's') {    // indicates whether the client is a spectator or a player
    // send the "OK L" message to the player
        char *letterMessage = malloc(5);
        if (letterMessage != NULL) {
            strcpy(letterMessage, "OK ");
            
            // add the player's corresponding letter
            letterMessage[3] = letter;
            letterMessage[4] = '\0';

            // send the message
            message_send(from, letterMessage);
            free(letterMessage);
        }
    }

    int NR = info->map->height;     // map height
    int NC = info->map->width;      // map width

    // get the length of the integers (if they were a string)
    int NRlen = snprintf(NULL, 0, "%d", NR); 
    int NClen = snprintf(NULL, 0, "%d", NC);

    // create a string for each integer
    char *NRstr = malloc(NRlen + 1);
    if (NRstr == NULL) { // out of memory
        return;
    }
    char *NCstr = malloc(NClen + 1);
    if (NCstr == NULL) { // out of memory
        free(NRstr);
        return;
    }
    // write the integers to their corresponding string
    snprintf(NRstr, NRlen + 1, "%d", NR);
    snprintf(NCstr, NClen + 1, "%d", NC);

    // send the "GRID NR NC" message to the client
    char *message = malloc(NRlen + NClen + 7);
    if (message != NULL) {
        // build the message
        strcpy(message, "GRID ");
        strcat(message, NRstr);
        strcat(message, " ");
        strcat(message, NCstr);

        // send the message
        message_send(from, message);
        free(message);
    }
    free(NRstr);
    free(NCstr);
    
    // send the initial gold message
    sendGoldMessage(from, 0, 0, *info->goldCt);
}

void sendGoldMessage(addr_t address, int collected, int purse, int remain)
{
    // length of the integers if they were strings
    int clen = snprintf(NULL, 0, "%d", collected);
    int plen = snprintf(NULL, 0, "%d", purse);
    int rlen = snprintf(NULL, 0, "%d", remain);
    // allocate memory for the string form of the integers
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
    // write the integers to their corresponding string
    snprintf(cstr, clen + 1, "%d", collected);
    snprintf(pstr, plen + 1, "%d", purse);
    snprintf(rstr, rlen + 1, "%d", remain);

    // send the "GOLD n p r" message to the client
    char *message = malloc(clen + plen + rlen + 8); 
    if (message != NULL) {
        // build the message
        strcpy(message, "GOLD ");
        strcat(message, cstr);
        strcat(message, " ");
        strcat(message, pstr);
        strcat(message, " ");
        strcat(message, rstr);

        // send the message
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

    // for each player, construct their map and send it to their corresponding address
	hashtable_iterate(playerInfo, info, mapSend);

    // if there is an active spectator, send them the spectator view
	if (message_isAddr(info->specAddr)) {
		sendSpectatorView(info);
	}
}

void sendQuit(serverInfo_t *info)
{   
    // allocing result string and copying in the first line
    char *result = (char*) malloc(sizeof(char) * 1000);
    strcpy(result, "QUIT GAME OVER\n");

    hashtable_t *playerInfo = info->playerInfo;
    // Building new string iteratively
    hashtable_iterate(playerInfo, result, buildGameOverString);
    hashtable_iterate(playerInfo, result, quitFunc);
    if (message_isAddr(info->specAddr)) {
        message_send(info->specAddr, result);
    }
    free(result);
}

/*********** buildGameOverString ***********/
void buildGameOverString(void *arg, const char *key, void *item)
{
    char *result = arg;
    player_t *player = item;
    // Making this players score string
    int bufsize = 10 + strlen(key);
    char *plyRes = malloc(bufsize); 
    snprintf(plyRes, bufsize, "%c\t%d\t%s\n", player->letter, player->gold, key);
    
    // Adding the string to the result string
    strncat(result, plyRes, strlen(plyRes) + 1);
    free(plyRes);
}


void quitFunc(void *arg, const char *key, void *item)
{
    // Sending a player a quit message
    char *result = arg;
    player_t *player = item;
    message_send(player->addr, result);
}

void sendSpectatorView(serverInfo_t *info)
{
	addr_t specAddr = info->specAddr;

    // grab the default, unaltered map
    map_t *baseMap = info->map;
    // build the player map with a NULL player, indicating the spectator view
    map_t *specMap = map_buildPlayerMap(baseMap, NULL, info->goldData, info->playerInfo);
    if (specMap == NULL) {  // out of memory
        return;
    }

    int len = strlen(specMap->mapStr);
	char *message = malloc(len + 9);
    if (message != NULL) {
        // build the message
	    strcpy(message, "DISPLAY\n");
	    strcat(message, specMap->mapStr);

        // send the map
        message_send(specAddr, message);
	    free(message);
    }
	map_delete(specMap);
}

void mapSend(void *arg, const char* key, void *item)
{
    serverInfo_t *info = (serverInfo_t *)arg;
    player_t *player = (player_t *)item;
    hashtable_t *playerInfo = info->playerInfo;
    hashtable_t *goldData = info->goldData;
    
    // grab the base, unaltered map
    map_t *baseMap = info->map;
    // build the map specific to this player
    map_t *playerMap = map_buildPlayerMap(baseMap, player, goldData, playerInfo); 
    if (playerMap == NULL) {    // out of memory
        return;
    }

    printf("player pos x: %d, y: %d\n", player->pos->x, player->pos->y);

    int len = strlen(playerMap->mapStr);
    char *message = malloc(len + 9);
    if (message != NULL) {
        // build the message
        strcpy(message, "DISPLAY\n");
        strcat(message, playerMap->mapStr);

        // send the map
        addr_t addr = player->addr;
        message_send(addr, message);
        free(message);
    }
    map_delete(playerMap);
}


void splitline(char *message, char *words[])
{
    // the first pointer points to the first character in the message
	words[0] = &message[0];
	int pos = 0;
    // increment the position until we reach a space or the end of the message
	while (!isspace(message[pos]) && message[pos] != '\0') {
		pos++;
	}
    // check for a second word
	if (message[pos] != '\0') {
		message[pos] = '\0';        // add a '\0' to split the message into two words
		words[1] = &message[pos+1]; // store a pointer to the second word
	}
}

player_t *player_new(addr_t from, char letter, serverInfo_t *info)
{
    player_t *player = malloc(sizeof(player_t));
    if (player == NULL) { // out of memory
        return NULL;
    } 
    // initialize player info
    player->addr = from;
    player->letter = letter;
    player->isActive = true;
    player->gold = 0;
    player->visibility = calloc(info->map->width * info->map->height + 1, sizeof(char));

    // get a random unoccupied position in the map (where a '.' character is)
    player->pos = getRandomPos(info->map, info->dotsPos, info->goldData, info->playerInfo);

    // calcualte the initial visibility of the player
    map_calculateVisibility(info->map, player);

    return player;
}

position_t *getRandomPos(map_t *map, counters_t *dotsPos, hashtable_t *goldInfo, hashtable_t *playerInfo)
{
    counters_t *filledPos = counters_new();     // counters to store locations of occupied '.' spaces in the map
    if (filledPos == NULL) { // out of memory
        return NULL;
    }

    // bundle to be based to the hashtable_iterate functions, adding occupied spaces to filledPos
    ctrsmap_t bundle = {filledPos, map};

    // iterate over gold, adding all occupied gold positions to filledPos
    hashtable_iterate(goldInfo, &bundle, goldFill);
    if (playerInfo != NULL) {
        // iterate over players, adding all occupied player positions to filledPos
        hashtable_iterate(playerInfo, &bundle, playerFill);
    }

    counters_t *validPositions = counters_new();    // counters to store unoccupied '.' positions
    if (validPositions == NULL) { // out of memory
        counters_delete(filledPos);
        return NULL;
    }
    
    // for all '.' positions, check if that position is occupied (by gold or a player). If not, add it to validPositions
    twoctrs_t ctrs = {filledPos, validPositions};
    counters_iterate(dotsPos, &ctrs, onlyDots);

    // count the number of valid positions
    int numValidPos = 0;
    counters_iterate(validPositions, &numValidPos, keyCount);

    // there must be at least one valid position to return
    if (numValidPos != 0) {
        // select a random node in the counters
        int val = rand() % numValidPos;
        int finalPos = 0;

        // iterate through the counters until that node is reached; return its position
        twoints_t t = {&val, &finalPos};
        counters_iterate(validPositions, &t, findPos);

        // convert the integer value of the position to an actual (x, y) position in the map
        position_t *result = map_intToPos(map, finalPos);

        counters_delete(filledPos);
        counters_delete(validPositions);
        return result;
    } else {
        counters_delete(filledPos);
        counters_delete(validPositions);
        return NULL;
    }
}

void findPos(void *arg, int key, int count)
{
    twoints_t *t = arg;
    int *val = t->x;
    int *finalPos = t->y;

    // if we have not stored position information yet..
    if (*val != -1) {
        // if we are at our desired position...
        if (*val == *finalPos) {
            // store the integer value of the position in the map
            *finalPos = key;
            *val = -1;  // stop searching
        } else {
            // otherwise, increment the current position
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

    // calculate the integer position of the gold within the map
    int val = map_calcPosition(map, pos);

    // add this integer position to the filled counters
    counters_add(filled, val);
}

void playerFill(void *arg, const char *key, void *item)
{
    ctrsmap_t *bundle = arg;
    counters_t *filled = bundle->ctrs;
    map_t *map = bundle->map;
    player_t *player = item;
    position_t *pos = player->pos;

    // only consider a space occupied if the player is active
    if (player->isActive) {

        // get the integer position of their (x,y) position in the map
        int val = map_calcPosition(map, pos);

        // store the integer positon in the filled counters
        counters_add(filled, val);
    }
}

void searchActivePlayers(void *arg, const char *key, void *item)
{
    player_t *player = item;
    bool *active = arg;

    // if there is at least one active player, do not end the game
    if (player->isActive) {
        *active = true;
    }
}

void onlyDots(void *arg, int key, int count)
{
    twoctrs_t *ctrs = arg;
    counters_t *filled = ctrs->ctrs1;
    counters_t *validPos = ctrs->ctrs2;

    // if the integer position is not in the filled counters, it is unoccupied
    if (counters_get(filled, key) == 0) {
        // thus, add it to the counters of valid positions
        counters_add(validPos, key);
    }
}

void playerDelete(void *item)
{
    player_t *player = item;
    if (player != NULL) {
        if (player->pos != NULL) {
            free(player->pos);
        }
        if (player->visibility != NULL) {
            free(player->visibility);
        }
        free(player);
    }
}

void goldDelete(void *item)
{
    gold_t *gold = item;
    if (gold != NULL) {
        if (gold->pos != NULL) {
            free(gold->pos);
        }
        free(gold);
    }
}

/*
 * simple function to count the number of keys in a `counters` module
 */
void keyCount(void *arg, int key, int count)
{
    int *nkeys = arg;
    if (nkeys != NULL && key >= 0 && count >=0) {
        (*nkeys)++;
    }
}

void checkPlayerCollision(void *arg, const char *key, void *item)
{
    pb_t *pb = arg;
    position_t *originalPos = pb->pos1;
    position_t *newPos = pb->pos2;
    addr_t addr = pb->addr;
    player_t *player = item;

    if (!message_eqAddr(addr, player->addr) && player->pos->x == newPos->x && player->pos->y == newPos->y) {
        // move the x position closer until it is 1 space away from the player
        while (abs(originalPos->x - newPos->x) > 1) {
            originalPos->x -= 1;
        }

        // move the y position closer until it is 1 space away from the player
        while (abs(originalPos->y - newPos->y) > 1) {
            originalPos->y -= 1;
        }

        // swaps the player that's been collided with to their proper spot
        player->pos->x = originalPos->x;
        player->pos->y = originalPos->y;
    }
}

/*
 * An iterator to recount the gold that is left, called after a player moves
 * *info->goldCt must be set to 0 before the iterator is called
 */
void recountGold(void *arg, const char *key, void *item)
{
    serverInfo_t *info = arg;
    gold_t *goldItem = item;

    if (!goldItem->isCollected){
       *info->goldCt += goldItem->value;
    }
}

void sendOthersGold(void *arg, const char *key, void *item)
{
    gb_t *goldBundle = arg;
    int *goldCt = goldBundle->goldCt;
    player_t *alreadySent = goldBundle->player;
    player_t *player = item;

    // send the updated gold count to all other players
    if (!message_eqAddr(alreadySent->addr, player->addr) && player->isActive) {
        sendGoldMessage(player->addr, 0, player->gold, *goldCt);
    }
}

gold_t *gold_new()
{
    gold_t *gold = malloc(sizeof(gold_t));
    if (gold == NULL) { // out of memory
        return NULL;
    }
    // initialize gold values
    gold->value = 0;
    gold->isCollected = false;
    gold->pos = NULL;

    return gold;
}

counters_t *getDotsPos(char *map)
{
	counters_t *dotsPos = counters_new();   // counters to return
    if (dotsPos == NULL) {  // out of memory
        return NULL;
    }
	int pos;
	int len = strlen(map);

    // iterate through the entire map, adding positions holding a '.'
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
