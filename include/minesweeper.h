#ifndef BOARD_H
#define BOARD_H

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

enum board_state {
	BOARD_PENDING_START,
	BOARD_PLAYING,
	BOARD_WIN,
	BOARD_GAME_OVER
};

struct board;
typedef void (*mswp_callback) (struct board *board, uint8_t *tile, int x, int y);

struct board {
	unsigned cursor_x;
	unsigned cursor_y;
	mswp_callback on_tile_updated;

	/* "Private" variables. Do not change from outside library,
	 * or undefined things will happen */
	enum board_state _state;
	unsigned _width;
	unsigned _height;
	float _mine_density;
	unsigned _mine_count;
	unsigned _opened_tile_count;
	uint8_t *_data;
};

/**
 * Create a new game board. Your frontends can use as many boards as it wants,
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
struct board *board_init(unsigned width, unsigned height, float mine_density, uint8_t *buffer);
size_t minimum_buffer_size(unsigned width, unsigned height);

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
void move_cursor(struct board *board, enum direction direction, bool wrap);
void open_tile_at_cursor(struct board *board);
void toggle_flag_at_cursor(struct board *board);
uint8_t *get_tile_at(struct board *board, int x, int y);
void get_adjacent_tiles(struct board *board, uint8_t *tile, uint8_t **adjacent_tiles);
uint8_t adjacent_mine_count(uint8_t *tile);

#endif
