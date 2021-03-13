# Implementation Spec

## Modules

### Server Module
The server initializes all required data for managing the nuggets game and communicating to clients.
The server listens for messages from clients, creating or removing new players/spectators and handling key actions as provided.
The server calls many of the map functions to construct and send DISPLAY messages to players and spectators.
Once all gold has been collected, the server sends a QUIT message to all players and the potential spectator, providing a GAME OVER screen with the final scores of each player.

#### Pseudocode
`main`
1. Call `validateParameters` to validate the map file and the seed
2. Call the server and return its return value

`server`
1. Initialize all information relevant to the server
2. Load the map from the map file, writing it to a one-line string and storing the height and width of the map
3. Create a `counters` module to store valid ‘.’ positions in the map
4. Generate random gold data based on the seed by calling `generateGold` and store in a `hashtable`
5. Construct the serverInfo to be passed to the message handler
6. Initialize logging and open a port by calling `message_init(stderr)`
7. Start listening for messages by calling `message_loop`
8. After finished looping, close the log and messages and free necessary initialized data
9. Return zero for no errors

`generateGold`
1. Initializes the gold total, min piles and max piles of gold in the game
2. Produces random behavior either by calling `srand(seed)` for a valid seed or `srand(getpid())` otherwise
3. Initializes the goldInfo hashtable to store gold data
4. WHILE there is more gold to place…
	* a. Create a new pile of gold, `gold_t`
	* b. Generate a random value for the gold pile, between 1 and goldTotal/goldMinNumPiles to ensure there are at least goldMinNumPiles piles of gold and each has at least 1 piece of gold
	* c. Get a random position in the map for this gold by calling `getRandomPos`
	* d. IF the random value of the gold pile is less than the gold left to place or there are now goldMaxNumPiles piles, set the gold pile’s value equal to the remaining goldTotal and set goldTotal equal to zero
	* e. Otherwise, subtract the value from the goldTotal
	* f. Store the gold in the hashtable with the pile number as the key and the gold as the item
	* g. Increment the number of piles
5. Return the goldInfo hashtable

`handleMessage`
1. Split the message into an array of up to two words, stored in words[]
2. Based on words[0] (the first word provided by the client), call the relevant function:
3. IF words[0] is “PLAY”...
	* a. First validate the number of players is not equal to the maximum number of allowable players; if so, send a quit message to the client indicating that the game is full
	* b. Check that the player’s name is not an empty string or is longer than the maximum player name size. (if so, truncate to the max size)
	* c. Give the player a letter based on the number of players in the game
	* d. Create a new `player` struct for the player
	* e. IF they can be inserted into the playerData hashtable with their name as the key and their struct as the item…
		* i. Increment the number of players
		* ii. Send the initial necessary information to the player by calling `SendInitialInfo`
		* iii. Send the map with the new player to all existing clients
4. IF words[0] is “KEY”...
	* a. Look for the player with the given address that passed the message
	* b. IF words[1] is “Q”, check if the the message is coming from a player or a spectator
		* i. If from a spectator, send the spectator a quit message and set the stored spectator address to be no address
		* ii. Otherwise, set the player to be inactive by setting their isActive bool to false. Send them a message to quit.
		* iii. IF there are no more players left in the game, close the server
		* iv. Otherwise, send the map with the player that quit to all existing clients
	* c. Validate the action of the player by calling `validateAction`
	* d. Check if the player collected gold by iterating over the gold positions and comparing the player’s position, sending gold messages and updating gold data if so (covered in `checkGoldCollect`)
	* e. IF the gold remaining to be collected reaches 0…send the GAME OVER screen to all clients and return true to stop looping
	* f. Otherwise, send the update maps as usual
5. IF words[1] is “SPECTATE”
	a. If there is an existing spectator (checked by calling message_isAddr on the spectator address), send them a quit message
	b. Update the spectator address to the address the message came from
	c. Send the spectator the initial required info by calling `sendInitialInfo`
	d. Send the spectator the full spectator view of the map by calling `sendSpectatorView`
6. Return false to continue looping

`validateAction` 
1. Takes a keypress as an input checks if it is a valid key of movement
2. Valid keys are “h,l,j,k,y,u,b,n” and the corresponding capital of each key move the player as far as possible in that direction
3. Calls `map_movePlayer` to move the player in the specified direction

`sendInitialInfo`
1. Convert the integer values of the map’s height and width into strings
2. Build up the `GRID NC NR` message and send it to the provided address using `message_send`
3. If the method call is coming from a player, indicated by a valid letter parameter, send the build and send the `OK L` message to the player to tell them their player letter
4. Send the initial gold message by calling `sendGoldMessage`

`sendGoldMessage`
1. Convert the provided integers into strings
2. Build and send the GOLD n p r string to tell the player or spectator the amount of gold collected, in their purse, and left in the game

`sendMaps`
1. Iterate over each player in the playerInfo hashtable, calling ‘mapSend’ to construct and send their individualized map
2. IF there is a spectator (by checking for a valid stored spectator address), send the spectator view to the spectator’s address

`sendQuit`
1. Construct the GAME OVER screen by building into a string starting with “QUIT GAME OVER”
2. Iterate over the player hashtable to add each player as a line to the GAME OVER string, reallocating memory to build the string as it goes along
3. Iterate over the player hashtable again to send each player the GAME OVER string
4. IF there is a spectator, send the spectator the GAME OVER string as well

`sendSpectatorView`
1. Get the unaltered map stored by the server
2. Build the spectator specific map by calling the `map_buildPlayerMap` function with NULL as a player parameter
3. Build the DISPLAY message and send it to the spectator’s address

`mapSend`
1. Get the map stored by the server
2. Build the player specific map by calling the `map_buildPlayerMap` function
3. Build the DISPLAY message and send it to the player’s address

`player_new`
1. Malloc data for a new `player_t` struct
2. Initialize player info, setting isActive to true and their initial gold to 0. Also start with a visibility string of all zeros
3. Get a random, unoccupied position for the player by calling `getRandomPos`
4. Return the player

`gold_new`
1. Malloc data for a new `gold_t` struct
2. Initialize value to 0, isCollected to false, and its position to NULL
3. Return the gold

`getRandomPos`
1. Create a `counters` structure to store the occupied ‘.’ spaces in the map
2. Create a `ctrsmap` bundle to be passed to `hashtable_iterate`, storing the new counters and the server’s stored map
3. Iterate through the gold and player hashtables, adding the positions of all uncollected gold and active players to the occupied `counters`
4. Create another `counters` structure to store the valid, unoccupied positions
5. Create a `twoctrs` structure to hold the occupied counters and the validPositions counters
6. Iterate through the counters of all positions of ‘.’ in the map, adding any positions that are not in the occupied counters to the valid positions counters
7. Iterate through the valid positions counters to get the number of nodes (# of valid positions) in the counters, numValidPos
8. Select a random node by calling rand()%numValidPos
9. Iterate through the valid positions counters until that specific node is reached, grabbing and storing its integer value (the key)
10. Convert the integer value to an (x, y) position in the map, and return this `position` struct

`goldFill`
1. Calculate the integer position of the gold struct within the map by calling `map_calcPosition`
2. Add this integer position to the occupied `counters` to indicate it’s space is occupied

`playerFill`
1. Calculate the integer position of the player struct within the map by calling `map_calcPosition`
2. Add this integer position to the occupied `counters` to indicate it’s space is occupied

`onlyDots`

If `counters_get` returns 0 for a call on the occupied `counters` and this key (this integer position), add the position to the valid position `counters`

`keyCount`

Increments the provided argument, nkeys, by 1 to indicate another node in the `counters`

`validateParameters`
1. Validate the number of arguments is between 2-3
2. Validate the map file by checking if it is readable
3. Validate the seed using sscanf, making sure it is solely an integer
4. Return true if all parameters are valid


### Map Module

The file map.c contains all the code necessary for making, copying, and updating maps and converting between position and integer. Various functions (as detailed below) allow the server to create new maps and pass them back to clients with appropriate player and gold data. Player movement feasibility and gold placement are also encapsulated here.

#### Pseudocode
`map_new()`
1. allocates new map and checks for validity
2. loads map string, which is the content of the text file fp, into a buffer
	* a. ftell() at SEEK_END to get length
	* b. fread() from SEEK_SET to convert file text to string
3. discards newlines to compress map into one-line string while saving width/height
	* a. increments height integer by one for every newline encountered
	* b. increments width by one for every other character
	* c. keeps track of string length for null termination
	* d. stores width divided by height as width and height as height in the map
4. copies buffer into mapStr and sets map string accordingly
5. returns map

`map_buildPlayerMap()`:
1. creates output map and copies passed map into it via map_copy()
2. use hashtable_iterate() and placeGold() to get goldData hashtable data into output map
3. do the same with addPlayerITR() to get players hashtable data into output map
4. make the current player’s identifier ‘@’
	* a. get player position plyIndx from map_calcPosition and player->pos
	* b. convert outMap->mapStr[plyIndx] to ‘@’
5. assign the output map string to map_buildOutput
6. return the output map

`placeGold()`:
1. assign map struct outMap to arg and gold struct g to item
2. get gold index gIndx from map_calcPosition()/g->pos for all uncollected gold
3. convert outMap->mapStr[gIndx] to `'*'` to represent unclaimed pile

`addPlayerITR()`:
1. assign map struct map to arg and player struct player to item
2. get player position plyIndx from map_calcPosition()/player->pos for every active player
3. convert map->mapStr[plyIndx] to the players identity character

`applyVis()`:

see description below

`replaceBlocked()`:

see description below

`initVisStr()`:

see description below

`intersectVis()`:

see description below

`map_calcPosition()`:

see description below

`map_intToPos()`:

see description below

`map_buildOutput()`:
1. copies map string into newly-allocated string (of length strlen(map->mapStr) plus map height) newMapStr with strcpy()
2. adds newline characters into newMapStr whenever i modulo map width is zero for every position in map->mapStr decremented from the end
3. returns newMapStr

`map_copy()`:
1. allocates new map struct newMap and map string newMapStr
2. copies width/height data into newMap and map->mapStr into newMapStr
3. returns newMap

`map_calculateVisibility()`:
1. create a `position` struct and FOR every border position...
2. call `map_calcVisPath` to alter the visibility string for the line from the player to the border `position` struct
3. free the `position` struct

`map_calcVisPath()`:
loop through all points in the map
* a. calculate dx dy, calculating ray between current point and point of interest
* b. get directions of x and y
* c. define error as dx-dy
* d. for every box between the two points:
	* i. if box obstructs (`isObstruct`), set visibility string to 1 and break
	* ii. if error is greater than zero, move the box horizontally
	* iii. else move the box vertically

`map_movePlayer()`:
1. copies position struct from player struct
2. checks for movement in positive or negative x or y directions with less than operators
3. checks for proper diagonal movement (differences between both coordinate values when comparing new and player positions)
	* a. if not equal movements in both directions (absolute values of differences equal), return original player position (failed movement)
	* b. increment copied player position by difference until the differences are gone
	* c. check that player can be in the new position; if not, decrement and exit the loop
4. checks for vertical movement and increments until y difference is gone or hits a wall
5. checks for horizontal movement and increments until x difference is gone or hits a wall
6. translates copied data back into passed player struct’s position struct and returns

canPlayerMoveTo():
1. uses map_calcPosition to find index
2. gets character from that index in the map string
3. returns true for characters other than space, hyphen, pipe, and plus
4. returns false by default

map_delete():

see description below


## Description

The primary delineation in our implementation of the Nuggets game is between Server and Map. While Map is responsible for creating and advertising game data to players in the form of a graphical user interface, Server is responsible for syncing player behavior, responding to client requests from players, and mediating general gameplay. The two modules need to communicate when Server uses Map structs to keep track of gold and player positions.

Apart from `hashtable` and `counters` modules, the server utilizes the following data structure to store data:

```
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
```


The server utilizes the following functions: 
```c
int server(char *argv[], int seed);
void splitline(char *message, char *words[]);
player_t *player_new(addr_t from, char letter, serverInfo_t *info);
counters_t *getDotsPos(char *map);
bool validateParameters(int argc, char *argv[], int *seed);
bool checkFile(char *fname, char *openParam);
void findPos(void *arg, int key, int count);
void goldFill(void *arg, const char *key, void *item);
void playerFill(void *arg, const char *key, void *item);
void checkGoldCollect(void *arg, const char *key, void *item);
void onlyDots(void *arg, int key, int count);
void keyCount(void *arg, int key, int count);
hashtable_t *generateGold(map_t *map, int seed, int *goldCt, counters_t *dotsPos);
position_t *getRandomPos(map_t *map, counters_t *dotsPos, hashtable_t *goldInfo, hashtable_t *playerInfo);
gold_t *gold_new();
void sendInitialInfo(const addr_t from, serverInfo_t *info, char letter);
void sendSpectatorView(serverInfo_t *info);
static bool handleMessage(void *arg, const addr_t from, const char *message);
void sendMaps(serverInfo_t *info);
void sendQuit(serverInfo_t *info);
void sendGoldMessage(addr_t from, int collected, int purse, int remain);
void findPlayerITR(void *arg, const char *key, void *item);
void buildGameOverString(void *arg, const char *key, void *item);
void mapSend(void *arg, const char* key, void *item);
void quitFunc(void *arg, const char *key, void *item);
bool validateAction(char *keyPress, player_t *player, serverInfo_t *info);
```

`server` initializes and stores all the data for the game and handles the looping of messages coming from clients. The server returns non-zero if an error occurs or zero otherwise. Argv[] is passed to provide the map file for reading and the seed provides a set random behavior

`splitline` splits the given line, char *line, into one or two words. The pointers to these words are then stored in char *words[]

`player_new` creates and returns a new player with address from, letter equal to the provided char letter, bool isActive set to true, gold set to 0, and their visibility set to a string of all zeros. 

`getDotsPos` takes a `map` struct to look at all the positions in the map string, constructing and returning a `counters` with all the integer positions of ‘.’ characters. 

`validateParameters` takes the command-line arguments argv and the count of arguments argc to ensure the user has made a valid call to the server

`checkFile` checks that the given file char *fname can be opened for reading or writing, based on the openParam. In this context, checkFile checks that the map file can be opened for reading to validate it as a parameters

`findPos` is an iterator function passed to `counters_iterate` which finds the specific node of a `counters` module and stores the value of its key

`goldFill` is an iterator function passed to `hashtable_iterate` which take a `counters` as the arg  and places the integer position of all gold into the counters module

`playerFill` is an iterator function passed to `hashtable_iterate` which take a `counters` as the arg  and places the integer position of all players into the counters module

`checkGoldCollect` is an iterator function passed to `hashtable_iterate` which takes a player, address, and integer as arguments to check if a player has collected a given gold piece.

`onlyDots` is an iterator function passed to `counters_iterate` which takes two `counters` as arguments and adds a node not in the occupied counters to the valid position counters

`keyCount` is an iterator function passed to `counters_iterate` which increments an integer for every node in the `counters`

`generateGold` takes a map to look for positions, seed for randomization purposes, goldCt to update the server’s remaining gold count, and the position of dots in the map stored as a `counters`: dotsPos. The function creates gold piles of random values and returns a hashtable of the goldData.

`getRandomPos` is a function that iterates through the goldInfo and playerInfo to identify occupied ‘.’ spaces in the map, and returns a position in dotsPos that is not occupied.

`gold_new` creates and returns a `gold` struct with value set to 0, isCollected set to false, and position set to NULL. 

`sendInitialInfo` takes an address to know where to send the data, a letter corresponding to a player or indicating that the address is of a spectator, and the server info for being processed and sent. The function sends the GRID, OK (only for a player), and GOLD messages.

`sendSpectatorView` constructs and sends the map of the game to the spectator by utilizing the serverInfo, if one exists

`handleMessage` is the main looping function which handles messages from clients by calling the relevant functions. The function takes an address `from`, where the char *message is coming from in order to create new players or spectators, or to handle a key press.

`sendMaps’ calls the `hashtable_iterate` function to iterate over the player `hashtable`, constructing and sending the map as a DISPLAY message to each player. It also sends the spectator its map if there is a valid spectator.

`sendQuit` constructs the GAME OVER screen using all the server information (info), and sends it to all players and the potential spectator, telling them to quit.

`sendGoldMessage` takes integer parameters and an address to construct and send the GOLD n p r message. In this case, collected = n, purse = p, and remain = r.

`findPlayerITR` is an iterator function for use in `hashtable_iterate` which searches for a player with the given address (passed as the argument), storing that player’s struct

`buildGameOverString` is an iterator function for use in `hashtable_iterate` which builds the GAME OVER screen line-by-line for each player in the player hashtable.

`mapSend` is an iterator function for use in `hashtable_iterate` which constructs and sends the specific map for an active player to that player.

`quitFunc` is an iterator function for use in `hashtable_iterate` which sends the QUIT GAME OVER message to each active player in the player hashtable.

`validateAction` handles the keyPress from a given player to validate its movement. Returns true if the player moved, false for no movement.


The map declares the following structs and utilizes the following functions:

#### Structs (in map.h)
```c
typedef struct position {} position_t;
typedef struct player {} player_t;
typedef struct gold {} gold_t;
typedef struct map {} map_t;
```

Position keeps track of x- and y-coordinates of players/gold
Player keeps track of client data, including address, position, amount of gold held, ability to see the board, identity to other players, and active/inactive status
Gold keeps track of gold pile values and position along with collected/uncollected status
Map keeps track of the map-representing string and its width and height


#### Functions
```c
map_t *map_new(FILE *fp)
map_t *map_buildPlayerMap(map_t *map, player_t *player, hashtable_t *goldData, hashtable_t *players)
void placeGold(void *arg, const char *key, void *item)
void addPlayerITR(void *arg, const char *key, void *item)
int map_calcPosition(map_t *map, position_t *pos)
position_t *map_intToPos(map_t *map, int i)
char *map_buildOutput(map_t *map)
static map_t *map_copy(map_t *map)
char *map_calculateVisibility(map_t *map, player_t *player, hashtable_t *goldData, hashtable_t *players)
void map_calcVisPath(map_t *map, char *vis, position_t *pos1, position_t *pos2)
void map_movePlayer(map_t *map, player_t *player, position_t *nextPos)
staticbool canPlayerCanMoveTo(map_t *map, position_t *pos)
void map_delete(map_t *map)
static bool isObstruct(char c);
static void replaceBlocked(map_t *map, map_t *outMap, player_t *player);
static void map_calcVisPath(map_t *map, char *vis, position_t *pos1, position_t *pos2);
static char *initVisStr(int width, int height);
static void intersectVis(char *vis1, char *vis2);
static void applyVis(map_t *map, char *vis);
void isOnGoldITR(void *arg, const char *key, void *item);
```

`map_new()` loads map struct from passed text file and returns the map

`map_buildPlayerMap()` adapts passed map to account for what the passed player should see and be able to do

`placeGold()`: passed to hashtable_iterate to put gold in output map

`addPlayerITR()`: passed to hashtable_iterate to put player characters in output map

`map_calcPosition()` checks to make sure position is inside map and returns the product of position’s y-coordinate and map’s width plus one greater than position’s x-coordinate

`map_intToPos()` allocates a position struct, decrements i by one, and returns the position with its x-coordinate as the modulus of i over width and its y-coordinate as the quotient of i over width

`map_buildOutput()` builds an output map string, appropriate for text file use/display, from a map

`map_copy()` yields an exact replica of the passed map, ready for alterations, with newly-allocated memory

`map_calculateVisibility()` loops through the border of the map to decide what points the player should be able to see from their current location

`map_calcVisPath()` creates lines to the border of the map and changes the player's visibility string according to what should be seen

`map_movePlayer()` updates player position in response to client input (nextPos) if valid

`canPlayerMoveTo()` checks for allowed player movement (i.e. anywhere but rocks and walls)

`map_delete()` frees map and map string to avoid memory shenanigans

`applyVis()` loops through the map and turns non-visible corresponding characters to spaces in the visibility string

`replaceBlocked()` replaces gold and players that are not currently visible with their basemap characters

`initVisStr()` gets a binary visibility string of appropriate length ready and zeroed

`intersectVis()` figures out whether the intersection of visibility string is visible


## Data Structures

The hashtable struct from lab 3 is vital to this operation, since the (key,item) pairing system allows the server to keep track of players and gold data efficiently and the iterative properties allow for rapid exploration of every value in the table. The counterset is also helpful for keeping track of integer values associated with various other data like the dots representing gold piles. Internally, the map and player structs are necessary to encapsulate position, gold, activity, and display data for passing among functions and modules (and server and clients). Smaller structs, like gold and position, are useful to further abstract and bundle up relevant information.

* Hashtable of (key = player name) (item = Player data struct)
* Hashtable of (key = pile number) (item = Gold data struct)
* Multiple Counters of (key = integer position in map)
* Position data struct
	* `int x`
	* `int y`
* Player data struct
	* Position struct
	* `int goldCt`
	* `char letter`
	* `bool isActive`
	* `char *visibility`
* Gold data struct
	* Position struct
	* `int value`
	* `bool isCollected`
* Map data struct
	* `char *mapStr`
	* `int height`
	* `int width`


## Security & Privacy properties

Buffers and extensive null-checking are our best current efforts to stymie potential malicious use. There’s not much to do privacy-wise at this point because clients are only internal. `map.c`, though, does make use of static internal methods so that only what is necessary for use by the *server* via `map.h` is available outside `map.c`.


## Error Handling & Recovery

Errors in the server will trigger printed messages indicating what went awry and graceful server closeouts. Memory leaks should be impossible given the exit procedures implemented.


## Resource Management & Persistent Storage

`malloc` and `free` are by far the most-used storage management methods. Valgrind will continue to be used in the future to detect leaks and debug allocation goofs.
