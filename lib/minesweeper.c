#include <minesweeper.h>
#include <stdlib.h>
#include <string.h>

void generate_mines(struct minesweeper_game *game, float density);

struct minesweeper_game *minesweeper_init(unsigned width, unsigned height, float mine_density, uint8_t *buffer) {
	/* Place a game object in the start of the buffer, and
	   treat the rest of the buffer as tile storage. */
	struct minesweeper_game *game = (struct minesweeper_game *)buffer;
	game->tiles = (struct minesweeper_tile *)buffer + sizeof(struct minesweeper_game);
	game->tile_update_callback = NULL;
	game->state = MINESWEEPER_PENDING_START;
	game->width = width;
	game->height = height;
	game->mine_count = 0;
	game->flag_count = 0;
	game->opened_tile_count = 0;
	game->selected_tile = NULL;
	game->user_info = NULL;
	memset(game->tiles, 0, sizeof(struct minesweeper_tile) * width * height);
	generate_mines(game, mine_density);
	return game;
}

size_t minesweeper_minimum_buffer_size(unsigned width, unsigned height) {
	return sizeof(struct minesweeper_game) + sizeof(struct minesweeper_tile) * width * height;
}

bool is_out_of_bounds(struct minesweeper_game *b, unsigned x, unsigned y) {
	return x >= b->width || y >= b->height;
}

/**
 * Get a pointer to a tile. Contains bounds checking and
 * will return NULL if the tile is out of bounds. This
 * is to simplify code that enumerates "adjacent" tiles.
 */
struct minesweeper_tile *minesweeper_get_tile_at(struct minesweeper_game *game, unsigned x, unsigned y) {
	if (is_out_of_bounds(game, x, y))
		return NULL;
	return &game->tiles[game->width * y + x];
}

void minesweeper_get_tile_location(struct minesweeper_game *game, struct minesweeper_tile *tile, unsigned *x, unsigned *y) {
	unsigned tile_index = tile - game->tiles;
	*y = tile_index / game->width;
	*x = tile_index % game->width;
}

void minesweeper_get_adjacent_tiles(struct minesweeper_game *game, struct minesweeper_tile *tile, struct minesweeper_tile *adjacent_tiles[8]) {
	unsigned x, y; minesweeper_get_tile_location(game, tile, &x, &y);
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
 * When attempting to open a tile that's already opened, the game
 * can "auto open" adjacent tiles if it's surrounded by the correct
 * amount of flagged tiles. This function counts the surrounding
 * flagged tiles.
 */
uint8_t count_adjacent_flags(struct minesweeper_game *game, struct minesweeper_tile *tile) {
	uint8_t count = 0;
	struct minesweeper_tile *adjacent_tiles[8];
	uint8_t i;
	minesweeper_get_adjacent_tiles(game, tile, adjacent_tiles);
	for (i = 0; i < 8; i++) {
		struct minesweeper_tile *adj_tile = adjacent_tiles[i];
		if (adj_tile && !(adj_tile->is_opened) && adj_tile->has_flag) {
			count++;
		}
	}
	return count;
}

void minesweeper_toggle_mine(struct minesweeper_game *game, struct minesweeper_tile *tile) {
	uint8_t i;
	struct minesweeper_tile *adjacent_tiles[8];
	int8_t count_modifier = -1;

	if (!tile) {
		return;
	}
	
	tile->has_mine = !tile->has_mine;
	if (tile->has_mine) {
		count_modifier = 1;
	}
	game->mine_count += count_modifier;

	/* Increase or decrease the mine counts on all adjacent tiles */
	minesweeper_get_adjacent_tiles(game, tile, adjacent_tiles);
	for (i = 0; i < 8; i++) {
		if (adjacent_tiles[i]) {
			adjacent_tiles[i]->adjacent_mine_count += count_modifier;
		}
	}
}

void generate_mines(struct minesweeper_game *game, float density) {
	unsigned tile_count = game->width * game->height;
	unsigned mine_count = tile_count * density;
	unsigned i;
	for (i = 0; i < mine_count; i++) {
		struct minesweeper_tile *random_tile = &game->tiles[rand() % tile_count];
		if (!random_tile->has_mine) {
			minesweeper_toggle_mine(game, random_tile);
		}
	}
}

void send_update_callback(struct minesweeper_game *game, struct minesweeper_tile *tile) {
	if (game->tile_update_callback != NULL) {
		game->tile_update_callback(game, tile, game->user_info);
	}
}

void minesweeper_toggle_flag(struct minesweeper_game *game, struct minesweeper_tile *tile) {
	if (tile && !tile->is_opened) {
		game->flag_count += tile->has_flag ? -1 : 1;
		tile->has_flag = !tile->has_flag;
		send_update_callback(game, tile);
	}
}

static inline bool all_tiles_opened(struct minesweeper_game *game) {
	return game->opened_tile_count == game->width * game->height - game->mine_count;
}

void open_adjacent_tiles(struct minesweeper_game *game, struct minesweeper_tile *tile);

void _open_tile(struct minesweeper_game *game, struct minesweeper_tile *tile, bool cascade) {
	if (tile->is_opened) {
		/* If this tile is already opened and has a mine count,
		 * it should open all adjacent tiles instead. This mimics
		 * the behaviour in the original minesweeper where you can
		 * right click opened tiles to open adjacent tiles quickly. */
		if (tile->adjacent_mine_count > 0 && tile->adjacent_mine_count == count_adjacent_flags(game, tile) && cascade)
			open_adjacent_tiles(game, tile);
		return;
	}

	if (tile->has_flag) {
		return;
	}

	tile->is_opened = true;
	game->opened_tile_count += 1;
	send_update_callback(game, tile);

	if (tile->has_mine) {
		game->state = MINESWEEPER_GAME_OVER;
		return;
	}

	if (all_tiles_opened(game)) {
		game->state = MINESWEEPER_WIN;
		return;
	}

	if (tile->adjacent_mine_count != 0) {
		return;
	}

	if (cascade)
		open_adjacent_tiles(game, tile);
}

void minesweeper_open_tile(struct minesweeper_game *game, struct minesweeper_tile *tile) {
	if (game->state == MINESWEEPER_PENDING_START) {
		game->state = MINESWEEPER_PLAYING;

		// Delete any potential mine on the first opened tile
		if (tile->has_mine) {
			minesweeper_toggle_mine(game, tile);
		}
	}
	_open_tile(game, tile, true);
}

void open_line_segments(struct minesweeper_game *game, unsigned x1, unsigned x2, unsigned y) {
	unsigned x;
	struct minesweeper_tile *tile;
	for (x = x1; x <= x2 && (tile = minesweeper_get_tile_at(game, x, y)); x++) {
		if (!tile->is_opened)
			_open_tile(game, tile, true);
	}
}

void open_adjacent_tiles(struct minesweeper_game *game, struct minesweeper_tile *tile) {
	unsigned tile_index = tile - game->tiles;
	unsigned ty = tile_index / game->width;
	unsigned tx = tile_index % game->width;
	unsigned lx, rx;
	struct minesweeper_tile *subtile;

	// Search for left boundary
	for (lx = tx - 1; (subtile = minesweeper_get_tile_at(game, lx, ty)); lx--) {
		if (subtile->is_opened && subtile->adjacent_mine_count != 0)
			break;
		_open_tile(game, subtile, false);
		if (subtile->adjacent_mine_count != 0)
			break;
	}

	// Re-adjust value if loop broke out because of out-of-bounds
	if (is_out_of_bounds(game, lx, ty))
		lx++;

	// Search for right boundary
	for (rx = tx + 1; (subtile = minesweeper_get_tile_at(game, rx, ty)); rx++) {
		if (subtile->is_opened && subtile->adjacent_mine_count != 0)
			break;
		_open_tile(game, subtile, false);
		if (subtile->adjacent_mine_count != 0)
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

void minesweeper_set_cursor(struct minesweeper_game *game, unsigned x, unsigned y) {
	if (is_out_of_bounds(game, x, y)) {
		game->selected_tile = NULL;
	} else {	
		game->selected_tile = minesweeper_get_tile_at(game, x, y);
	}
}

void minesweeper_move_cursor(struct minesweeper_game *game, enum direction direction, bool should_wrap) {
	unsigned x, y;
	if (game->selected_tile == NULL) {
		return;
	}

	minesweeper_get_tile_location(game, game->selected_tile, &x, &y);
	switch (direction) {
	case LEFT:
		if (x != 0)
			x--;
		else if (should_wrap)
			x = game->width - 1;
		break;
	case RIGHT:
		if (x != game->width - 1)
			x++;
		else if (should_wrap)
			x = 0;
		break;
	case UP:
		if (y != 0)
			y--;
		else if (should_wrap)
			y = game->height - 1;
		break;
	case DOWN:
		if (y != game->height - 1)
			y++;
		else if (should_wrap)
			y = 0;
		break;
	}
	minesweeper_set_cursor(game, x, y);
}
