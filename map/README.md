# NUGGETS: Bash Boys
## Gus Emmett, Will Dinauer, and J.W. Dell

### map

This directory is the home of the *map* program of the __Nuggets__ project's `map` module.
The __map__ is a critical part of the *Nuggets* game in that *maps* form the playing surface. Usage of this module (code in `map.c`; declarations in `map.h`) happens largely in `../server/server.c`, where player interactions and client-server communication is handled.

`map.c` concerns building *maps* from text files, placing *players* and *gold* on appropriate random gridpoints, and handling *player* movement.

See `../IMPLEMENTATION.md` for detailed information regarding `map.c` and its relationship with the `server` module.

Compile with `make`. Test with `make test`. See `../TESTING.md` for documentation and `maptest.c` for test cases.
