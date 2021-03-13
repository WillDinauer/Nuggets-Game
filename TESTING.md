# TESTING
## Nuggets: Bash Boys
### `make test` to run server with map specified in `Makefile`

The primary *unit testing* occurs in the __map__ module with `mapTest.c` (usage after compiling: ./mapTest). This places a __player__ on a small, new map and checks random movements to see if the __map__ was built properly and __player__ movement works.

`gdb ./server core` was a primary debugging method for the __server__ module, allowing us to step through the *client*-*server* communication paradigm and `server.c`'s use of the __map__ module and find programming errors. `make clean` gets rid of any backup collateral files.

As specified in the `server/Makefile`, __Valgrind__ was useful to find memory leaks (`valgrind ./server 2>server.log ../maps/*.txt`, where `*` represents a map name of the user's choosing).

Most importantly, we integration-tested game functionality with `tmux`, which allows one user to be both *host* and as many *clients* as is useful. This enabled `valgrind` testing as well as __player__ collision (which found further application when group members joined the same server, made possible by use of the command `localhost` since we were all on `plank`).
