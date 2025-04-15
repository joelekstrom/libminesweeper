#ifndef MINESWEEPER_H
#define MINESWEEPER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

enum direction {
	LEFT,
	RIGHT,
	UP,
	DOWN
};

enum minesweeper_game_state {
	MINESWEEPER_PENDING_START,
	MINESWEEPER_PLAYING,
	MINESWEEPER_WIN,
	MINESWEEPER_GAME_OVER
};

struct minesweeper_tile {
	uint8_t adjacent_mine_count : 4;
	bool has_flag : 1;
	bool has_mine : 1;
	bool is_opened : 1;
};

struct minesweeper_game;
typedef void (*minesweeper_callback) (struct minesweeper_game *game, struct minesweeper_tile *tile, void *user_info);

/**
 * Contains data for a single minesweeper game.
 *
 * Do not modify fields directly - use the functions below instead. The only
 * exception to this is tile_update_callback, which can be set at any time.
 *
 * Created automatically by minesweeper_init().
 */
struct minesweeper_game {
	unsigned width;
	unsigned height;
	unsigned mine_count;
	unsigned opened_tile_count;
	unsigned flag_count;
	struct minesweeper_tile *selected_tile; /* Pointer to the tile under the cursor */
	struct minesweeper_tile *tiles;
	enum minesweeper_game_state state;
	minesweeper_callback tile_update_callback; /* Optional function pointer to receive tile state updates */
	void *user_info; /* Can be used for anything, will be passed as a parameter to tile_update_callback */
};

/**
 * Initialize a new game
 *
 * width, height: Adjusts the size of the game area
 * mine_density: A value between 0 and 1. When 0, no tiles will have mines. When 1, all tiles will have mines
 * buffer: A memory location to store the game at. Must be at least the size returned from minesweeper_minimum_buffer_size() for the given height and width
 *
 * Returns a pointer to somewhere within buffer. To delete a game, invalidate entire buffer.
 */
struct minesweeper_game *minesweeper_init(unsigned width, unsigned height, float mine_density, uint8_t *buffer);
size_t minesweeper_minimum_buffer_size(unsigned width, unsigned height);

/**
 * Set the location of the cursor. "The cursor"
 * is another name for game->selected_tile.
 */
void minesweeper_set_cursor(struct minesweeper_game *game, unsigned x, unsigned y);

/**
 * Move the cursor in a specified direction.
 *
 * If should_wrap is true, attempting to move outside the game bounds will
 * result in the cursor wrapping to the opposite side of the game area.
 *
 * This will update game->selected_tile.
 */
void minesweeper_move_cursor(struct minesweeper_game *game, enum direction direction, bool should_wrap);

/**
 * Opens an unflagged tile.
 *
 * Will recursively open all adjacent tiles that have zero
 * adjacent mines.
 *
 * If tile is already opened, all adjacent unflagged tiles will
 * be opened instead, to imitate the quick-open functionality of most
 * minesweeper games.
 */
void minesweeper_open_tile(struct minesweeper_game *game, struct minesweeper_tile *tile);

/**
 * Open souraounding tiles if tile is opened, else a flag is placed
 */
void minesweeper_space_tile(struct minesweeper_game *game, struct minesweeper_tile *tile);

/**
 * Toggles a flag on an unopened tile.
 */
void minesweeper_toggle_flag(struct minesweeper_game *game, struct minesweeper_tile *tile);

/**
 * Get pointer to tile at location. Returns NULL if location if out of bounds.
 */
struct minesweeper_tile *minesweeper_get_tile_at(struct minesweeper_game *game, unsigned x, unsigned y);

/**
 * Get location of a tile.
 *
 * x/y: Pointers to integers which the result will be written to.
 */
void minesweeper_get_tile_location(struct minesweeper_game *game, struct minesweeper_tile *tile, unsigned *x, unsigned *y);

/**
 * Get all tiles adjacent to tile. A tile can have at most 8 adjacent tiles,
 * but tiles adjacent to edges of the game area will have fewer.
 *
 * adjacent_tiles: A pointer to an array of 8 tile pointers. Pointers to the resulting tiles will
 * be written to this array. Some tiles may be NULL, if tile is adjacent to an edge.
 */
void minesweeper_get_adjacent_tiles(struct minesweeper_game *game, struct minesweeper_tile *tile, struct minesweeper_tile *adjacent_tiles[8]);

/**
 * Toggles a mine on a tile, and adjusts the adjacent mine counts for all
 * adjacent tiles.
 */
void minesweeper_toggle_mine(struct minesweeper_game *game, struct minesweeper_tile *tile);

#endif
