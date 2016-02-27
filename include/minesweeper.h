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

struct board {
	int cursor_x;
	int cursor_y;

	/* "Private" variables. Do not change from outside library,
	 * or undefined things will happen */
	enum board_state _state;
	int _width;
	int _height;
	float _mine_density;
	int _mine_count;
	int _opened_tile_count;
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
struct board *board_init(int width, int height, float mine_density, uint8_t *buffer);
size_t minimum_buffer_size(int width, int height);

void move_cursor(struct board* board, enum direction direction);
void open_tile_at_cursor(struct board* board);
void toggle_flag_at_cursor(struct board* board);
uint8_t* get_tile_at(struct board *board, int x, int y);
void get_adjacent_tiles(struct board *board, uint8_t *tile, uint8_t **adjacent_tiles);
uint8_t adjacent_mine_count(uint8_t* tile);

#endif
