#include <minesweeper.h>
#include <stdlib.h>
#include <string.h>

uint8_t *get_tile_at(struct board *board, int x, int y);
void open_tile(struct board *board, uint8_t *tile);

struct board *board_init(unsigned width, unsigned height, float mine_density, uint8_t *buffer) {
	/* Place a board object in the start of the buffer, and
	   treat the rest of the buffer as tile storage. */
	struct board *board = (struct board *)buffer;
	board->_data = buffer + sizeof(struct board);

	board->cursor_x = width / 2;
	board->cursor_y = height / 2;
	board->_state = BOARD_PENDING_START;
	board->_width = width;
	board->_height = height;
	board->_mine_density = mine_density;
	board->_mine_count = 0;
	board->_opened_tile_count = 0;
	memset(board->_data, 0, width * height);
	return board;
}

size_t minimum_buffer_size(unsigned width, unsigned height) {
	return sizeof(struct board) + width * height;
}

bool is_out_of_bounds(struct board *b, int x, int y) {
	return x < 0 || x >= b->_width || y < 0 || y >= b->_height;
}

/**
 * Get a pointer to a tile. Contains bounds checking and
 * will return NULL if the tile is out of bounds. This
 * is to simplify code that enumerates "adjacent" tiles.
 */
uint8_t *get_tile_at(struct board *board, int x, int y) {
	if (is_out_of_bounds(board, x, y))
		return NULL;
	return &board->_data[board->_width * y + x];
}

void get_adjacent_tiles(struct board *board, uint8_t *tile, uint8_t **adjacent_tiles) {
	unsigned tile_index = tile - board->_data;
	unsigned y = tile_index / board->_width;
	unsigned x = tile_index % board->_width;
	adjacent_tiles[0] = get_tile_at(board, x - 1, y - 1);
	adjacent_tiles[1] = get_tile_at(board, x - 1, y);
	adjacent_tiles[2] = get_tile_at(board, x - 1, y + 1);
	adjacent_tiles[3] = get_tile_at(board, x, y - 1);
	adjacent_tiles[4] = get_tile_at(board, x, y + 1);
	adjacent_tiles[5] = get_tile_at(board, x + 1, y - 1);
	adjacent_tiles[6] = get_tile_at(board, x + 1, y);
	adjacent_tiles[7] = get_tile_at(board, x + 1, y + 1);
}

/**
 * We use the last 4 bits of a tile for tile data such as
 * opened, mine, flag etc. the first 4 stores a count of
 * adjacent mines. This function gets that value.
 */
uint8_t adjacent_mine_count(uint8_t *tile) {
	return (*tile & 0xF0) >> 4;
}

/**
 * When attempting to open a tile that's already opened, the game
 * can "auto open" adjacent tiles if it's surrounded by the correct
 * amount of flagged tiles. This function counts the surrounding
 * flagged tiles.
 */
uint8_t count_adjacent_flags(struct board *board, uint8_t *tile) {
	uint8_t count = 0;
	uint8_t *adjacent_tiles[8];
	uint8_t i;
	get_adjacent_tiles(board, tile, adjacent_tiles);
	for (i = 0; i < 8; i++) {
		uint8_t *adj_tile = adjacent_tiles[i];
		if (adj_tile && !(*adj_tile & TILE_OPENED) && *adj_tile & TILE_FLAG) {
			count++;
		}
	}
	return count;
}

void increment_adjacent_mine_count(uint8_t *tile) {
	uint8_t value = adjacent_mine_count(tile) + 1;
	uint8_t shifted_value = value << 4;
	*tile = shifted_value | (*tile & 0x0F);
}

void place_mine(struct board *board, uint8_t *tile) {
	bool has_mine = *tile & TILE_MINE;
	if (!has_mine) {
		int i;
		uint8_t *adjacent_tiles[8];
		*tile |= TILE_MINE;
		board->_mine_count++;

		/* Increase the mine counts on all adjacent tiles */
		get_adjacent_tiles(board, tile, adjacent_tiles);
		for (i = 0; i < 8; i++) {
			if (adjacent_tiles[i]) {
				increment_adjacent_mine_count(adjacent_tiles[i]);
			}
		}
	}
}

void generate_mines(struct board *board, uint8_t *safe_tile) {
	unsigned tile_count = board->_width * board->_height;
	unsigned mine_count = tile_count * board->_mine_density;
	int i;
	for (i = 0; i < mine_count; i++) {
		uint8_t *random_tile = &board->_data[rand() % tile_count];
		if (random_tile != safe_tile) {
			place_mine(board, random_tile);
		}
	}
}

bool all_tiles_opened(struct board *board) {
	return board->_opened_tile_count == board->_width * board->_height - board->_mine_count;
}

void open_tile_at_cursor(struct board *board) {
	uint8_t *tile = get_tile_at(board, board->cursor_x, board->cursor_y);
	if (board->_state == BOARD_PENDING_START) {
		generate_mines(board, tile);
		board->_state = BOARD_PLAYING;
	}
	open_tile(board, tile);
}

void toggle_flag_at_cursor(struct board *board) {
	uint8_t *tile = get_tile_at(board, board->cursor_x, board->cursor_y);
	*tile ^= TILE_FLAG;
}

void open_tile(struct board *board, uint8_t *tile) {
	if (*tile & TILE_OPENED) {
		/* If this tile is already opened and has a mine count,
		 * it should open all adjacent tiles instead. This mimics
		 * the behaviour in the original minesweeper where you can
		 * right click opened tiles to open adjacent tiles quickly. */
		uint8_t adj_mine_count = adjacent_mine_count(tile);
		if (adj_mine_count > 0 && adj_mine_count == count_adjacent_flags(board, tile))
			goto open_adjacent_tiles;
		return;
	}

	if (*tile & TILE_FLAG) {
		return;
	}

	*tile |= TILE_OPENED;
	board->_opened_tile_count += 1;

	if (*tile & TILE_MINE) {
		board->_state = BOARD_GAME_OVER;
		return;
	}

	if (all_tiles_opened(board)) {
		board->_state = BOARD_WIN;
		return;
	}

	if (adjacent_mine_count(tile) != 0) {
		return;
	}

 open_adjacent_tiles:
	{
		int i;
		uint8_t *adjacent_tiles[8];
		get_adjacent_tiles(board, tile, adjacent_tiles);
		for (i = 0; i < 8; i++) {
			uint8_t *adjacent_tile = adjacent_tiles[i];
			if (adjacent_tile && !(*adjacent_tile & TILE_OPENED) && !(*adjacent_tile & TILE_FLAG)) {
				open_tile(board, adjacent_tile);
			}
		}
	}
}

int max(int a, int b) {
	return a > b ? a : b;
}

int min(int a, int b) {
	return a < b ? a : b;
}

void move_cursor(struct board *board, enum direction direction) {
	switch (direction) {
	case LEFT:
		board->cursor_x = max(board->cursor_x - 1, 0);
		break;
	case RIGHT:
		board->cursor_x = min(board->cursor_x + 1, board->_width - 1);
		break;
	case UP:
		board->cursor_y = max(board->cursor_y - 1, 0);
		break;
	case DOWN:
		board->cursor_y = min(board->cursor_y + 1, board->_height - 1);
		break;
	}
}
