#include <minesweeper.h>
#include <stdlib.h>
#include <string.h>

uint8_t *get_tile_at(struct board *board, int x, int y);
void open_tile(struct board *board, uint8_t *tile);
void open_adjacent_tiles(struct board *board, uint8_t *tile);

struct board *board_init(unsigned width, unsigned height, float mine_density, uint8_t *buffer) {
	/* Place a board object in the start of the buffer, and
	   treat the rest of the buffer as tile storage. */
	struct board *board = (struct board *)buffer;
	board->_data = buffer + sizeof(struct board);

	board->cursor_x = width / 2;
	board->cursor_y = height / 2;
	board->on_tile_updated = NULL;
	board->_state = BOARD_PENDING_START;
	board->_width = width;
	board->_height = height;
	board->_mine_density = mine_density;
	board->_mine_count = 0;
	board->_flag_count = 0;
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

static inline void get_xy(struct board *board, uint8_t *tile, unsigned *x, unsigned *y) {
	unsigned tile_index = tile - board->_data;
	*y = tile_index / board->_width;
	*x = tile_index % board->_width;
}

void get_adjacent_tiles(struct board *board, uint8_t *tile, uint8_t **adjacent_tiles) {
	unsigned x, y; get_xy(board, tile, &x, &y);
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
		uint8_t i;
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
	unsigned i;
	for (i = 0; i < mine_count; i++) {
		uint8_t *random_tile = &board->_data[rand() % tile_count];
		if (random_tile != safe_tile) {
			place_mine(board, random_tile);
		}
	}
}

void open_tile_at_cursor(struct board *board) {
	uint8_t *tile = get_tile_at(board, board->cursor_x, board->cursor_y);
	if (board->_state == BOARD_PENDING_START) {
		generate_mines(board, tile);
		board->_state = BOARD_PLAYING;
	}
	open_tile(board, tile);
}

void send_update_callback(struct board *board, uint8_t *tile) {
	if (board->on_tile_updated != NULL) {
		unsigned x, y; get_xy(board, tile, &x, &y);
		board->on_tile_updated(board, tile, x, y);
	}
}

void toggle_flag_at_cursor(struct board *board) {
	uint8_t *tile = get_tile_at(board, board->cursor_x, board->cursor_y);
	if (!(*tile & TILE_OPENED)) {
		board->_flag_count += (*tile & TILE_FLAG) ? -1 : 1;
		*tile ^= TILE_FLAG;
		send_update_callback(board, tile);
	}
}

static inline bool all_tiles_opened(struct board *board) {
	return board->_opened_tile_count == board->_width * board->_height - board->_mine_count;
}

void _open_tile(struct board *board, uint8_t *tile, bool cascade) {
	if (*tile & TILE_OPENED) {
		/* If this tile is already opened and has a mine count,
		 * it should open all adjacent tiles instead. This mimics
		 * the behaviour in the original minesweeper where you can
		 * right click opened tiles to open adjacent tiles quickly. */
		uint8_t adj_mine_count = adjacent_mine_count(tile);
		if (adj_mine_count > 0 && adj_mine_count == count_adjacent_flags(board, tile) && cascade)
			open_adjacent_tiles(board, tile);
		return;
	}

	if (*tile & TILE_FLAG) {
		return;
	}

	*tile |= TILE_OPENED;
	board->_opened_tile_count += 1;
	send_update_callback(board, tile);

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

	if (cascade)
		open_adjacent_tiles(board, tile);
}

void open_tile(struct board *board, uint8_t *tile) {
	_open_tile(board, tile, true);
}

void open_line_segments(struct board* board, unsigned x1, unsigned x2, unsigned y) {
	unsigned x;
	uint8_t* tile;
	for (x = x1; x <= x2 && (tile = get_tile_at(board, x, y)); x++) {
		if (!(*tile & TILE_OPENED))
			_open_tile(board, tile, true);
	}
}

void open_adjacent_tiles(struct board *board, uint8_t *tile) {
	unsigned tile_index = tile - board->_data;
	unsigned ty = tile_index / board->_width;
	unsigned tx = tile_index % board->_width;
	unsigned mine_count;
	unsigned lx, rx;
	uint8_t* subtile;

	// Search for left boundary
	for (lx = tx - 1; (subtile = get_tile_at(board, lx, ty)); lx--) {
		mine_count = adjacent_mine_count(subtile);
		if (*subtile & TILE_OPENED && mine_count != 0)
			break;
		_open_tile(board, subtile, false);
		if (mine_count != 0)
			break;
	}

	// Re-adjust value if loop broke out because of out-of-bounds
	if (is_out_of_bounds(board, lx, ty))
		lx++;

	// Search for right boundary
	for (rx = tx + 1; (subtile = get_tile_at(board, rx, ty)); rx++) {
		mine_count = adjacent_mine_count(subtile);
		if (*subtile & TILE_OPENED && mine_count != 0)
			break;
		_open_tile(board, subtile, false);
		if (mine_count != 0)
			break;
	}

	// Re-adjust value if loop broke out because of out-of-bounds
	if (is_out_of_bounds(board, rx, ty))
		rx--;

	// Open line(s) above
	open_line_segments(board, lx, rx, ty - 1);
	// Open line(s) below
	open_line_segments(board,lx, rx, ty + 1);
}

void move_cursor(struct board *board, enum direction direction, bool wrap) {
	switch (direction) {
	case LEFT:
		if (board->cursor_x != 0)
			board->cursor_x--;
		else if (wrap)
			board->cursor_x = board->_width - 1;
		break;
	case RIGHT:
		if (board->cursor_x != board->_width - 1)
			board->cursor_x++;
		else if (wrap)
			board->cursor_x = 0;
		break;
	case UP:
		if (board->cursor_y != 0)
			board->cursor_y--;
		else if (wrap)
			board->cursor_y = board->_height - 1;
		break;
	case DOWN:
		if (board->cursor_y != board->_height - 1)
			board->cursor_y++;
		else if (wrap)
			board->cursor_y = 0;
		break;
	}
}
