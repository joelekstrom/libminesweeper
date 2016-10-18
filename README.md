[![Build Status](https://travis-ci.org/accatyyc/libminesweeper.svg?branch=master)](https://travis-ci.org/accatyyc/libminesweeper)
# libminesweeper 
A static library for handling Minesweeper game logic, that can run on embedded hardware. 
Its goals are to be as efficient as possible memory- and processing power wise. It's written
in "[SDCC](http://sdcc.sourceforge.net) compatible C99", which pretty much means that it's
C99 without inline variable declarations.

## Building
Clone the repo and run `make` in the folder, and you'll get the binary `libminesweeper.a`. Link the binary and include 
`include/minesweeper.h` in your projects. You can also check out the reference implentations below. Those repos include this repo
as a `git submodule` and automatically builds it as part of their build chain.

## Usage

First, we should include the library headers:
```c
#include <minesweeper.h>
```

Let's set up a game. You can have as many games as you want, if you for example want to implement multiplayer.
First, you need to supply the library with a memory location where it can put game data. The minimum required length
of this buffer can be retrieved from the function `minesweeper_minimum_buffer_size(unsigned width, unsigned height)`.

To initialize a game in this buffer, call `minesweeper_init(unsigned width, unsigned height, float mine_density, uint8_t *buffer)`,
where mine density is a float value between 0 and 1. If 1, all tiles will contain a mine, and if 0, no tiles will
contain a mine. The game starts to get unplayable around a density of 0.5.
	
```c
unsigned width = 20;
unsigned height = 10;
uint8_t *game_buffer = malloc(mineseeper_minimum_buffer_size(width, height));

srand(time(NULL)); // Let's make the game less predictable
struct minesweeper_game *game = minesweeper_init(width, height, 0.1, game_buffer);
```

You don't need to free the pointer returned from minesweeper_init(). It points to somewhere
within the buffer created above, so to invalidate a game you simply free the game buffer.

Next, let's move the cursor around and open a tile.
```c
minesweeper_set_cursor(game, 0, 0);
minesweeper_move_cursor(game, RIGHT, false);
minesweeper_open_tile(game, game->selected_tile);
```

Using the cursor functionality is no requirement. It exists to help handling common
movement logic for keypress-based sweeping. It's also possible to modify tiles directly,
if for example making a mouse based sweeper:
```c
uint8_t *tile = minesweeper_get_tile_at(game, x, y);
minesweeper_open_tile(game, tile);
```

To render tiles correctly in your UI, you can find the state of a tile by doing the following:

```c
uint8_t *tile = minesweeper_get_tile_at(game, x, y);
bool contains_flag = *tile & TILE_FLAG;
bool is_opened = *tile & TILE_OPENED;
bool has_mine = *tile & TILE_MINE;
```

The trick is that each tile is an 8-bit number of flags representing the state of that tile.
It also contains the number of adjacent mines, which is the number shown on opened tiles
in most minesweepers. You can retrieve it like this:
```c
uint8_t mine_count = minesweeper_get_adjacent_mine_count(tile);
```

Check out the reference implementations for more examples on how to render a game.
All available functions are documented in minesweeper.h.

## Testing
Run `make test` to run the unit tests. It might be a good idea to run the tests
with your preferred compiler, to catch anything I might've missed. Please add an
issue if any tests don't pass!

## Reference implementations:
- [Terminal Mines](https://github.com/accatyyc/terminal-mines) An ncurses frontend for running in terminals
- [gbmines](https://github.com/rotmoset/gb-mines) A Gameboy Color frontend
