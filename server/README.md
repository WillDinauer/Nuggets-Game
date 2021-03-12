# NUGGETS: Bash Boys
## Gus Emmett, Will Dinauer, and J.W. Dell

### server

This directory is the home of the *server* program and `serverUtils` library of the __Nuggets__ project's `server` module.
The __server__ is the central "brain" of the *Nuggets* game in that all communication among *players* goes through here. *maps* form the playing surface. After compilation, the usage of this module is `./server 2>server.log ../maps/*.txt`, where any properly-formatted file in `../maps` may stand in for `*`. Error and status messages print to the *logfile*. The bulk of the code is in `server.c`, though the module relies on `serverUtils.h` and `../map.h`.

`server.c` concerns initiating a game and keeping *players* up to date with one another, handling messages and sending gameplay information. `serverUtils.c` provides necessary functionality to the __server__ module.

See `../IMPLEMENTATION.md` for detailed information regarding `server.c` and its relationship with the `map` module.

Compile with `make`. Test with `make test`. See `../TESTING.md` for documentation.
