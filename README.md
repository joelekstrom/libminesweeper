# libminesweeper 
A library for handling Minesweeper game logic, that can run on embedded hardware.
Its goals are to be as efficient as possible memory- and processing power wise. It's written
in "[SDCC](http://sdcc.sourceforge.net) compatible C99", which pretty much means that it's
C99 without inline variable declarations.

## Building
Clone the repo and run `make` in the folder, and you'll get the binary `libminesweeper.a`. Link the binary and include 
`include/minesweeper.h` in your projects. You can also check out the reference implentations below. Those repos include this repo
as a `git submodule` and automatically builds it as part of their build chain. You can also simply copy the implementation
file and headers you need directly into your project.

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

_Note: due to optimizations, not all tiles will actually have mines with a density of 1.0, since
during generation, if a tile is seleted that already has a mine, it will be ignored. This means
that the actual mine count will usually be a bit less than (width * height * mine_density)_
	
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
struct minesweeper_tile *tile = minesweeper_get_tile_at(game, x, y);
minesweeper_open_tile(game, tile);
```

To render tiles correctly in your UI, you can find the state of a tile by doing the following:

```c
struct minesweeper_tile *tile = minesweeper_get_tile_at(game, x, y);
bool has_mine = tile->has_mine;
```

A tile has the following properties:
- `adjacent_mine_count`: The number of mines in the tiles surrounding this tile. This corresponds to the number that minesweepers normally draw on an opened tile.
- `has_flag`
- `has_mine`
- `is_opened`

### Handling callbacks

You can register a callback handler to be informed by the library whenever a tile updates.
This allows you to update your UI by only redrawing updated tiles. A `void *user_info` is
passed to your update function (same pointer that you set to `game->user_info` which you can use
to pass any data to the callback.

```c
void tile_updated(struct minesweeper_game *game, struct minesweeper_tile *tile, void *user_info) {
	// Redraw tile in your UI
}

game->tile_update_callback = &tile_updated;
```

Check out the reference implementations for more examples on how to render a game.
All available functions are documented in minesweeper.h.

### C++ API

There is a header to use the library from C++, in an object-oriented manner.
Here are the same examples, using `minesweeper.hpp`:

```cpp
#include <minesweeper.hpp>

Minesweeper::Game game = Minesweeper::Game(width, height, 0.3);
game.setCursor(0, 0);
game.moveCursor(RIGHT, false);

auto tile = game.selectedTile();
tile.open();
bool hasMine = tile.hasMine();

game.tileUpdateCallback = [](Minesweeper::Game& game, Minesweeper::Tile& tile) {
	// Redraw tile in your UI
};

```

## Testing
Run `make run-c-tests` to run the unit tests. It might be a good idea to run the tests
with your preferred compiler, to catch anything I might've missed. Please add an
issue if any tests don't pass!

Similarly, use `make run-cpp-tests` for C++, projects, or `make run-all-tests` for both.

## Reference implementations:
- [Terminal Mines](https://github.com/accatyyc/terminal-mines) An ncurses frontend for running in terminals
- [gbmines](https://github.com/rotmoset/gb-mines) A Gameboy Color frontend
