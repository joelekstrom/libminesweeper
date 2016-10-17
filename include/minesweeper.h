#ifndef MINESWEEPER_H
#define MINESWEEPER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

enum {
	TILE_OPENED = 1,
	TILE_MINE = 1 << 1,
	TILE_FLAG = 1 << 2
};

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

struct minesweeper_game;
typedef void (*minesweeper_callback) (struct minesweeper_game *board, uint8_t *tile, int x, int y);

struct minesweeper_game {
	minesweeper_callback on_tile_updated;

	/* "Readonly" variables. Do not change from outside library,
	 * or undefined things will happen */
	enum minesweeper_game_state _state;
	unsigned _width;
	unsigned _height;
	float _mine_density;
	unsigned _mine_count;
	unsigned _opened_tile_count;
	unsigned _flag_count;
	uint8_t *_selected_tile;
	uint8_t *_data;
};

/**
 * Create a new game. Your frontends can use as many games as it wants simulatenously,
 * for example to implement multiplayer.
 *
 * Mine density is a value between 0.0 and 1.0. At 1.0, every tile will contain
 * a mine, and at 0.0, no tiles will contain mines.
 *
 * Buffer is the memory location where you want to keep your board data.
 * The buffer must be at least the size returned from minimum_buffer_size
 * for the specified height and width values.
 *
 * The returned pointer will point to somewhere within buffer, so to remove
 * a board, you can invalidate the whole buffer.
 */
struct minesweeper_game *minesweeper_init(unsigned width, unsigned height, float mine_density, uint8_t *buffer);
size_t minesweeper_minimum_buffer_size(unsigned width, unsigned height);

/**
 * Use the minesweeper_move_cursor() function to move the cursor around, one step at a time.
 * If you for example are implementing a mouse based UI, you can instead set the
 * cursor position directly using minesweeper_set_cursor(). If 'wrap' is true, the cursor
 * will wrap around the board, meaning if that you move it out of bounds, it
 * will jump to the opposite side. This can make the game more efficient to
 * play with key movement.
 *
 * Note that there's no requirement for your UI to display the cursor. It exists
 * to make common game patterns simpler to implement.
 *
 * Setting/moving the cursor will update the _selected_tile variable in the game struct.
 *
 * If you do want to display a cursor, the library takes care of all movement
 * and out of bounds-handling for you, so you should use the built-in way.
 */
void minesweeper_move_cursor(struct minesweeper_game *game, enum direction direction, bool should_wrap);
void minesweeper_set_cursor(struct minesweeper_game *game, int x, int y);

/**
 * Open a tile if it isn't flagged. All adjacent tiles that do not have
 * an adjacent mine count will also be opened. This will recursively
 * open tiles until adjacent mine counts are encountered.
 *
 * If minesweeper_open_tile() is called with an already open tile, it will
 * open all adjacent non-flagged tiles if the correct number of flags has
 * been placed around it.
 */
void minesweeper_open_tile(struct minesweeper_game *game, uint8_t *tile);
void minesweeper_toggle_flag(struct minesweeper_game *game, uint8_t *tile);
uint8_t *minesweeper_get_tile_at(struct minesweeper_game *game, int x, int y);
void minesweeper_get_adjacent_tiles(struct minesweeper_game *game, uint8_t *tile, uint8_t **adjacent_tiles);

/**
 * Returns the number of adjacent mines for a tile. This is
 * the colored number that is shown in most minesweeper
 * implementations.
 */
uint8_t minesweeper_get_adjacent_mine_count(uint8_t *tile);

/**
 * Toggles a mine on a tile, and adjusts the adjacent mine counts for all
 * adjacent tiles.
 *
 * This function can be used to add/remove mines to alter the game.
 */
void minesweeper_toggle_mine(struct minesweeper_game *game, uint8_t *tile);

#endif
