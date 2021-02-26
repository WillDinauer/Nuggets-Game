# Nuggets - Design Spec

### User: “player” of Nuggets game, modeled as a client making requests from the server
### Roles of User: provide a name and navigate through the game appropriately
### User interface:
Execute server from command line ./server map.txt [seed]

* `map.txt` is the pathname for a *map* file, assumed to be valid
* `seed` is an optional parameter to control the randomness in the game

The server prints status updates throughout the game to indicate what it is doing and how it is communicating with the clients. It also prints a message indicating the end of the game along with the final score for each player. 

### Inputs:
1. Map file: `map.txt` (we assume the `map` is *valid*, but infer NR, NC (rows and columns))
2. Optional `seed` to dictate random behavior; must be a positive integer

### Outputs:
1. __Logfile__ that collects all errors and log outputs
2. Messages printed to ***stdout***, which help mediate gameplay

### Modules: 
We anticipate the use of the following modules/functions:
* `main` which parses arguments and initializes other modules
* `server` which manages the stored player data, *map*, and gold information (including NR, NC)
* `mapReader` which reads the map file to construct the one-line *map* `string`
* `messageHandler` which waits for messages from clients and calls the appropriate function relevant to their action
* `mapBroadcaster` which sends an updated version of the map to all clients

### Dataflow through modules:
1. `main` parses arguments and sends them to the __server__
2. `server` stores the *map* obtained from `mapReader` and initializes the *player data* __hashtable__. The *map*, player, and gold data are passed to the `messageHandler`
3. `mapReader` constructs the *map* `string` and passes it to the __server__, updating NR, NC
4. `messageHandler` receives messages from the *clients* (ex. __Players__ joining, moving, leaving), updates the gold and player data passed from the __server__, and passes the updated information to the `mapBroadcaster`
5. `mapBroadcaster` constructs and sends appropriate updated *maps* to each *client* using information from `messageHandler` 

### Functions:
__init__
```c
startServer()
loadMap()
placeGold()
```
__Gameplay__
```c
addPlayer()
updatePlayerPosition()
removePlayer()
gameOverScreen()
```
__Client-Server Communication__
```c
parsePlayerMessage()
updateClientMaps()
```
__Misc__ 
```c
logError()
```

### Pseudo code:
1. Execute from the command line as shown in the [*user interface*](#user-interface)
2. Parse command line, validate arguments, initialize [*modules*](#modules)
3. Read from the map file to infer and store the number of columns and rows, NR x NC
4. Write the map file to a one-line string
5. Create an array of gold `structs` using the seed or a default form of randomization
6. Open a *port* to allow users to connect
7. For each *player* that connects,
     * Validate the player (acceptable name length, not max players already connected)
     * Create a *player* `struct` for them, place them in an unoccupied space on the __map__, and try to store their data in the *player* __hashtable__
     * Send the display with the new *player* to all users
8. For each *spectator* that connects,
      * If there is an existing *spectator*, send them a message to quit
      * Send the display to the latest *spectator*
9. Whenever a *player* makes a move,
      * Validate the action (ex. Not bumping into a wall)
      * Handle the action accordingly and update *player*/gold data
      * Send the new display to all connected users
10. Whenever a *player* disconnects,
      * Remove them from the *map* (but keep them in the __hashtable__ to prevent rejoining)
      * If there are no players left, end the game and close the server
      * Send the new display to all connected users
11. Once all gold has been collected, broadcast the end of the game to the clients. Send a report of the final scores to each player

### Major data structures:
* __Hashtable__ of (key = player name) (item = *Player* data `struct`)
* *Player* data `struct`
     * *Position* `struct`
     * Gold collected
     * Name
     * isActive
     * Visibility binary string (0 for invisible; 1 for visible)
* Gold data `struct`
     * Value
     * isCollected
     * *Position* `struct`
* Position data `struct`
     * X
     * Y


### Testing Plan:

__Unit Testing__ - test each module individually
* Do map load and place gold
* Successful parse of a message from the user
* Update player positions 
     * Test over multiple clients
     * Player get gold
* Threaded test
* Add and delete player test

__Integration Testing__ - assemble server and test complete functionality as well as client interactions
* Test basic functionality:
     * Adding a player
     * Adding a spectator
     * Player can move around the map, collect gold, stopped at barriers
     * Visibility updated properly
* Test player interactions - collision of two players
* Test adding a spectator while a spectator already exists
* Testing boundary cases: 
     * long player names (50 characters)
     * adding player when game is full (26 players)
     * players making keystroke requests at the same time (server collisions)

System Testing - needs client to be complete

Regression testing - be able to run unit and integration tests from a simpler (eg .sh) program

Usability testing - normal users with standard relevant biases and habits

Acceptance testing - will the graders be happy?
