#include <minesweeper.h>
#include <stdlib.h>
#include <string.h>

void generate_mines(struct minesweeper_game *game, float density);

struct minesweeper_game *minesweeper_init(unsigned width, unsigned height, float mine_density, uint8_t *buffer) {
	/* Place a game object in the start of the buffer, and
	   treat the rest of the buffer as tile storage. */
	struct minesweeper_game *game = (struct minesweeper_game *)buffer;
	game->_data = buffer + sizeof(struct minesweeper_game);
	game->on_tile_updated = NULL;
	game->_state = MINESWEEPER_PENDING_START;
	game->_width = width;
	game->_height = height;
	game->_mine_count = 0;
	game->_flag_count = 0;
	game->_opened_tile_count = 0;
	game->_selected_tile = NULL;
	memset(game->_data, 0, width * height);
	generate_mines(game, mine_density);
	return game;
}

size_t minesweeper_minimum_buffer_size(unsigned width, unsigned height) {
	return sizeof(struct minesweeper_game) + width * height;
}

bool is_out_of_bounds(struct minesweeper_game *b, int x, int y) {
	return x < 0 || x >= b->_width || y < 0 || y >= b->_height;
}

/**
 * Get a pointer to a tile. Contains bounds checking and
 * will return NULL if the tile is out of bounds. This
 * is to simplify code that enumerates "adjacent" tiles.
 */
uint8_t *minesweeper_get_tile_at(struct minesweeper_game *game, int x, int y) {
	if (is_out_of_bounds(game, x, y))
		return NULL;
	return &game->_data[game->_width * y + x];
}

static inline void get_xy(struct minesweeper_game *game, uint8_t *tile, unsigned *x, unsigned *y) {
	unsigned tile_index = tile - game->_data;
	*y = tile_index / game->_width;
	*x = tile_index % game->_width;
}

void minesweeper_get_adjacent_tiles(struct minesweeper_game *game, uint8_t *tile, uint8_t **adjacent_tiles) {
	unsigned x, y; get_xy(game, tile, &x, &y);
	adjacent_tiles[0] = minesweeper_get_tile_at(game, x - 1, y - 1);
	adjacent_tiles[1] = minesweeper_get_tile_at(game, x - 1, y);
	adjacent_tiles[2] = minesweeper_get_tile_at(game, x - 1, y + 1);
	adjacent_tiles[3] = minesweeper_get_tile_at(game, x, y - 1);
	adjacent_tiles[4] = minesweeper_get_tile_at(game, x, y + 1);
	adjacent_tiles[5] = minesweeper_get_tile_at(game, x + 1, y - 1);
	adjacent_tiles[6] = minesweeper_get_tile_at(game, x + 1, y);
	adjacent_tiles[7] = minesweeper_get_tile_at(game, x + 1, y + 1);
}

/**
 * We use the last 4 bits of a tile for tile data such as
 * opened, mine, flag etc. the first 4 stores a count of
 * adjacent mines. This function gets that value.
 */
uint8_t minesweeper_get_adjacent_mine_count(uint8_t *tile) {
	return (*tile & 0xF0) >> 4;
}

/**
 * When attempting to open a tile that's already opened, the game
 * can "auto open" adjacent tiles if it's surrounded by the correct
 * amount of flagged tiles. This function counts the surrounding
 * flagged tiles.
 */
uint8_t count_adjacent_flags(struct minesweeper_game *game, uint8_t *tile) {
	uint8_t count = 0;
	uint8_t *adjacent_tiles[8];
	uint8_t i;
	minesweeper_get_adjacent_tiles(game, tile, adjacent_tiles);
	for (i = 0; i < 8; i++) {
		uint8_t *adj_tile = adjacent_tiles[i];
		if (adj_tile && !(*adj_tile & TILE_OPENED) && *adj_tile & TILE_FLAG) {
			count++;
		}
	}
	return count;
}

void adjust_adjacent_mine_count(uint8_t *tile, int8_t value) {
	uint8_t new_value = minesweeper_get_adjacent_mine_count(tile) + value;
	uint8_t shifted_value = new_value << 4;
	*tile = shifted_value | (*tile & 0x0F);
}

void minesweeper_toggle_mine(struct minesweeper_game *game, uint8_t *tile) {
	uint8_t i;
	uint8_t *adjacent_tiles[8];
	int8_t count_modifier = -1;

	if (!tile) {
		return;
	}
	
	*tile ^= TILE_MINE;
	if (*tile & TILE_MINE) {
		count_modifier = 1;
	}
	game->_mine_count += count_modifier;

	/* Increase the mine counts on all adjacent tiles */
	minesweeper_get_adjacent_tiles(game, tile, adjacent_tiles);
	for (i = 0; i < 8; i++) {
		if (adjacent_tiles[i]) {
			adjust_adjacent_mine_count(adjacent_tiles[i], count_modifier);
		}
	}
}

void generate_mines(struct minesweeper_game *game, float density) {
	unsigned tile_count = game->_width * game->_height;
	unsigned mine_count = tile_count * density;
	unsigned i;
	for (i = 0; i < mine_count; i++) {
		uint8_t *random_tile = &game->_data[rand() % tile_count];
		if (!(*random_tile & TILE_MINE)) {
			minesweeper_toggle_mine(game, random_tile);
		}
	}
}

void send_update_callback(struct minesweeper_game *game, uint8_t *tile) {
	if (game->on_tile_updated != NULL) {
		unsigned x, y; get_xy(game, tile, &x, &y);
		game->on_tile_updated(game, tile, x, y);
	}
}

void minesweeper_toggle_flag(struct minesweeper_game *game, uint8_t *tile) {
	if (tile && !(*tile & TILE_OPENED)) {
		game->_flag_count += (*tile & TILE_FLAG) ? -1 : 1;
		*tile ^= TILE_FLAG;
		send_update_callback(game, tile);
	}
}

static inline bool all_tiles_opened(struct minesweeper_game *game) {
	return game->_opened_tile_count == game->_width * game->_height - game->_mine_count;
}

void open_adjacent_tiles(struct minesweeper_game *game, uint8_t *tile);

void _open_tile(struct minesweeper_game *game, uint8_t *tile, bool cascade) {
	if (*tile & TILE_OPENED) {
		/* If this tile is already opened and has a mine count,
		 * it should open all adjacent tiles instead. This mimics
		 * the behaviour in the original minesweeper where you can
		 * right click opened tiles to open adjacent tiles quickly. */
		uint8_t adj_mine_count = minesweeper_get_adjacent_mine_count(tile);
		if (adj_mine_count > 0 && adj_mine_count == count_adjacent_flags(game, tile) && cascade)
			open_adjacent_tiles(game, tile);
		return;
	}

	if (*tile & TILE_FLAG) {
		return;
	}

	*tile |= TILE_OPENED;
	game->_opened_tile_count += 1;
	send_update_callback(game, tile);

	if (*tile & TILE_MINE) {
		game->_state = MINESWEEPER_GAME_OVER;
		return;
	}

	if (all_tiles_opened(game)) {
		game->_state = MINESWEEPER_WIN;
		return;
	}

	if (minesweeper_get_adjacent_mine_count(tile) != 0) {
		return;
	}

	if (cascade)
		open_adjacent_tiles(game, tile);
}

void minesweeper_open_tile(struct minesweeper_game *game, uint8_t *tile) {
	if (game->_state == MINESWEEPER_PENDING_START) {
		game->_state = MINESWEEPER_PLAYING;

		// Delete any potential mine on the first opened tile
		if (*tile & TILE_MINE) {
			minesweeper_toggle_mine(game, tile);
		}
	}
	_open_tile(game, tile, true);
}

void open_line_segments(struct minesweeper_game *game, unsigned x1, unsigned x2, unsigned y) {
	unsigned x;
	uint8_t* tile;
	for (x = x1; x <= x2 && (tile = minesweeper_get_tile_at(game, x, y)); x++) {
		if (!(*tile & TILE_OPENED))
			_open_tile(game, tile, true);
	}
}

void open_adjacent_tiles(struct minesweeper_game *game, uint8_t *tile) {
	unsigned tile_index = tile - game->_data;
	unsigned ty = tile_index / game->_width;
	unsigned tx = tile_index % game->_width;
	unsigned mine_count;
	unsigned lx, rx;
	uint8_t* subtile;

	// Search for left boundary
	for (lx = tx - 1; (subtile = minesweeper_get_tile_at(game, lx, ty)); lx--) {
		mine_count = minesweeper_get_adjacent_mine_count(subtile);
		if (*subtile & TILE_OPENED && mine_count != 0)
			break;
		_open_tile(game, subtile, false);
		if (mine_count != 0)
			break;
	}

	// Re-adjust value if loop broke out because of out-of-bounds
	if (is_out_of_bounds(game, lx, ty))
		lx++;

	// Search for right boundary
	for (rx = tx + 1; (subtile = minesweeper_get_tile_at(game, rx, ty)); rx++) {
		mine_count = minesweeper_get_adjacent_mine_count(subtile);
		if (*subtile & TILE_OPENED && mine_count != 0)
			break;
		_open_tile(game, subtile, false);
		if (mine_count != 0)
			break;
	}

	// Re-adjust value if loop broke out because of out-of-bounds
	if (is_out_of_bounds(game, rx, ty))
		rx--;

	// Open line(s) above
	open_line_segments(game, lx, rx, ty - 1);
	// Open line(s) below
	open_line_segments(game,lx, rx, ty + 1);
}

void minesweeper_set_cursor(struct minesweeper_game *game, int x, int y) {
	if (is_out_of_bounds(game, x, y)) {
		game->_selected_tile = NULL;
	} else {	
		game->_selected_tile = minesweeper_get_tile_at(game, x, y);
	}
}

void minesweeper_move_cursor(struct minesweeper_game *game, enum direction direction, bool should_wrap) {
	unsigned x, y;
	if (game->_selected_tile == NULL) {
		return;
	}

	get_xy(game, game->_selected_tile, &x, &y);
	switch (direction) {
	case LEFT:
		if (x != 0)
			x--;
		else if (should_wrap)
			x = game->_width - 1;
		break;
	case RIGHT:
		if (x != game->_width - 1)
			x++;
		else if (should_wrap)
			x = 0;
		break;
	case UP:
		if (y != 0)
			y--;
		else if (should_wrap)
			y = game->_height - 1;
		break;
	case DOWN:
		if (y != game->_height - 1)
			y++;
		else if (should_wrap)
			y = 0;
		break;
	}
	minesweeper_set_cursor(game, x, y);
}
