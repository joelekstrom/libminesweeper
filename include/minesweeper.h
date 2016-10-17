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
	unsigned cursor_x;
	unsigned cursor_y;
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
 * Use the move_cursor function to move the cursor around, one step at a time.
 * If you for example are implementing a mouse based UI, you can instead set the
 * cursor position directly in the board struct. If 'wrap' is true, the cursor
 * will wrap around the board, meaning if that you move it out of bounds, it
 * will jump to the opposite side. This can make the game more efficient to
 * play with key movement.
 *
 * Note that there's no requirement for your UI to display the cursor, it's
 * just there to be the basis for the "open_tile_at_cursor"-function.
 *
 * If you do want to display a cursor, the library takes care of all movement
 * and out of bounds-handling for you, so you should use the built-in way.
 */
void minesweeper_move_cursor(struct minesweeper_game *game, enum direction direction, bool should_wrap);
void minesweeper_open_tile(struct minesweeper_game *game, uint8_t *tile);
void minesweeper_toggle_flag(struct minesweeper_game *game, uint8_t *tile);
uint8_t *minesweeper_get_tile_at(struct minesweeper_game *game, int x, int y);
void minesweeper_get_adjacent_tiles(struct minesweeper_game *game, uint8_t *tile, uint8_t **adjacent_tiles);
uint8_t minesweeper_get_adjacent_mine_count(uint8_t *tile);

#endif
