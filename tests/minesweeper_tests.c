#include <minunit.h>
#include <stdio.h>
#include <minesweeper.h>
#include <stdlib.h>

int tests_run = 0;
int width = 120;
int height = 100;

uint8_t *game_buffer = NULL;
struct minesweeper_game *game = NULL;

static char * test_init() {
	puts("Test: Initialization...");
	game_buffer = malloc(minesweeper_minimum_buffer_size(width, height));
	game = minesweeper_init(width, height, 1.0, game_buffer);
    mu_assert("Error: game data must be offset in the buffer.", game->data - sizeof(struct minesweeper_game) == (uint8_t *)game);
	mu_assert("Error: after init, state must be pending_start", game->state == MINESWEEPER_PENDING_START);
   	return 0;
}

static int count_adjacent_tiles(uint8_t **adjacent_tiles) {
	int i;
	int count = 0;
	for (i = 0; i < 8; i++) {
		if (adjacent_tiles[i]) {
			count++;
		}
	}
	return count;
}

static char * test_get_tile() {
	uint8_t *tile = minesweeper_get_tile_at(game, 10, 10);
	puts("Test: Get tile...");
	mu_assert("Error: the tile at (10, 10) should exist after init.", tile != NULL);

	tile = minesweeper_get_tile_at(game, -1, 10);
	mu_assert("Error: the tile at (-1, 10) shouldn't exist. minesweeper_get_tile_at must return NULL.", tile == NULL);

	tile = minesweeper_get_tile_at(game, 101, 101);
	mu_assert("Error: the tile at (101, 101) shouldn't exist. minesweeper_get_tile_at must return NULL.", tile == NULL);
	return 0;
}

static char * test_get_adjacent_tiles() {
	uint8_t *tile = minesweeper_get_tile_at(game, 0, 0);
	uint8_t *adjacent_tiles[8];
	puts("Test: Get adjacent tiles...");
	minesweeper_get_adjacent_tiles(game, tile, adjacent_tiles);
	mu_assert("Error: the tile at (0, 0) should have 3 adjacent tiles.", count_adjacent_tiles(adjacent_tiles) == 3);

	tile = minesweeper_get_tile_at(game, 0, 1);
	minesweeper_get_adjacent_tiles(game, tile, adjacent_tiles);
	mu_assert("Error: the tile at (0, 1) should have 5 adjacent tiles.", count_adjacent_tiles(adjacent_tiles) == 5);

	tile = minesweeper_get_tile_at(game, 10, 10);
	minesweeper_get_adjacent_tiles(game, tile, adjacent_tiles);
	mu_assert("Error: the tile at (10, 10) should 8 adjacent tiles.", count_adjacent_tiles(adjacent_tiles) == 8);
	return 0;
}

static char * test_open_first_tile() {
	puts("Test: Open first tile...");
	minesweeper_set_cursor(game, width / 2, height / 2);
	minesweeper_open_tile(game, game->selected_tile);
	mu_assert("Error: after opening the first tile, state should be: playing", game->state == MINESWEEPER_PLAYING);
	mu_assert("Error: there must not be a mine under the first opened tile", !(*game->selected_tile & TILE_MINE));
	return 0;
}

static char * test_open_mine() {
	puts("Test: Open mine...");
	minesweeper_set_cursor(game, 0, 0);
	minesweeper_open_tile(game, game->selected_tile);
	mu_assert("Error: After opening a mine tile, state must be game_over.", game->state == MINESWEEPER_GAME_OVER);
	return 0;
}

static char * test_adjacent_mine_counts() {
	uint8_t *center_tile;
	uint8_t *left_tile;
	uint8_t *right_tile;
	uint8_t *adj_tiles[8];
	uint8_t mine_count;
	int i;

	puts("Test: Adjacent mine counters...");
	game = minesweeper_init(width, height, 0.0, game_buffer);
	center_tile = minesweeper_get_tile_at(game, 10, 10);
	left_tile = minesweeper_get_tile_at(game, 9, 10);
	right_tile = minesweeper_get_tile_at(game, 11, 10);

	minesweeper_toggle_mine(game, left_tile);
	minesweeper_toggle_mine(game, right_tile);

	mine_count = minesweeper_get_adjacent_mine_count(center_tile);
	mu_assert("Error: the tile at (10, 10) must have a mine_count of 2 after mines have been placed at (9, 10) and (11, 10).", mine_count == 2);

	// Toggle mines on all tiles adjacent to the center tile, except
	// left and right tiles which already have mines
	minesweeper_get_adjacent_tiles(game, center_tile, adj_tiles);
	for (i = 0; i < 8; i++) {
		if (adj_tiles[i] != left_tile && adj_tiles[i] != right_tile) {
			minesweeper_toggle_mine(game, adj_tiles[i]);
		}
	}

	mine_count = minesweeper_get_adjacent_mine_count(center_tile);
	mu_assert("Error: the tile at (10, 10) must have a mine_count of 8 after mines have been placed at every tile around it.", mine_count == 8);

	minesweeper_toggle_mine(game, left_tile);
	mine_count = minesweeper_get_adjacent_mine_count(center_tile);
	mu_assert("Error: the tile at (10, 10) must have a mine_count of 7 after the mine has been toggled off on the tile to the left of it.", mine_count == 7);

	return 0;
}

static char * test_win_state() {
	puts("Test: 0 mines/Win state...");
	/* Init the game with zero mines */
	game = minesweeper_init(width, height, 0.0, game_buffer);
	minesweeper_set_cursor(game, 0, 0);
	minesweeper_open_tile(game, game->selected_tile);
	mu_assert("Error: when 0 mines exist, all tiles should be opened after the first tile is opened", game->opened_tile_count == width * height);
	mu_assert("Error: when all tiles are opened, state should be WIN", game->state == MINESWEEPER_WIN);
	return 0;
}

static unsigned callback_count = 0;
void callback(struct minesweeper_game *game, uint8_t *tile, int x, int y) {
	callback_count++;
}

static char * test_callbacks() {
	puts("Test: Changed tile callbacks...");
	game = minesweeper_init(width, height, 0.0, game_buffer);
	minesweeper_set_cursor(game, width / 2, height / 2);
	minesweeper_toggle_flag(game, game->selected_tile);
	mu_assert("Error: if no callback function is assigned, no callbacks should fire.", callback_count == 0);
	game->tile_update_callback = &callback;
	minesweeper_toggle_flag(game, game->selected_tile);
	mu_assert("Error: if a callback function is assigned, a callback should fire when a flag is toggled.", callback_count == 1);
	callback_count = 0;
	minesweeper_open_tile(game, game->selected_tile);
	mu_assert("Error: when 0 mines exist, a change callback should be sent for every tile.", callback_count == width * height);
	return 0;
}

static char * test_flag_counts() {
	puts("Test: Flag counts...");
	game = minesweeper_init(width, height, 0.0, game_buffer);
	minesweeper_set_cursor(game, width / 2, height / 2);
	minesweeper_toggle_flag(game, game->selected_tile);
	mu_assert("Error: after placing a flag, _flag_count should increase.", game->flag_count == 1);
	minesweeper_toggle_flag(game, game->selected_tile);
	mu_assert("Error: after toggling flag at the same tile, _flag_count should decrease.", game->flag_count == 0);
	minesweeper_open_tile(game, game->selected_tile);
	minesweeper_toggle_flag(game, game->selected_tile);
	mu_assert("Error: when attempting to toggle flag at an already opened tile, nothing should happen, and flag count should stay at 0.", game->flag_count == 0);
	return 0;
}

static char * test_selected_tile() {
	puts("Test: Selected tile...");
	game = minesweeper_init(width, height, 0.0, game_buffer);
	mu_assert("Error: Selected tile should be NULL after game init", game->selected_tile == NULL);
	minesweeper_toggle_flag(game, game->selected_tile);
	mu_assert("Error: Flag count should not change when toggling flag on empty cursor tile", game->flag_count == 0);
	minesweeper_set_cursor(game, 10, 10);
	mu_assert("Error: Selected tile should be at (10, 10) after a call to set_cursor", game->selected_tile == minesweeper_get_tile_at(game, 10, 10));
	minesweeper_set_cursor(game, 9999, 9999);
	mu_assert("Error: Selected tile should be set to NULL when attempting to set cursor out of bounds", game->selected_tile == NULL);
	return 0;
}

static char * test_cursor_movement() {
	puts("Test: Cursor movement...");
	game = minesweeper_init(width, height, 0.0, game_buffer);
	minesweeper_set_cursor(game, 10, 10);
	minesweeper_move_cursor(game, RIGHT, false);
	minesweeper_move_cursor(game, RIGHT, false);
	minesweeper_move_cursor(game, DOWN, false);
	mu_assert("Error: Selected tile should be at (12, 11) after cursor moving", game->selected_tile == minesweeper_get_tile_at(game, 12, 11));
	minesweeper_set_cursor(game, 0, 0);
	minesweeper_move_cursor(game, LEFT, false);
	mu_assert("Error: Cursor should not move outside of bounds if should_wrap is false", game->selected_tile == minesweeper_get_tile_at(game, 0, 0));
	minesweeper_move_cursor(game, LEFT, true);
	minesweeper_move_cursor(game, UP, true);
	mu_assert("Error: Cursor should have wrapped to other side of the board if should_wrap is true", game->selected_tile == minesweeper_get_tile_at(game, width - 1, height - 1));
	return 0;
}

static char * all_tests() {
	mu_run_test(test_init);
	mu_run_test(test_get_tile);
	mu_run_test(test_get_adjacent_tiles);
	mu_run_test(test_open_first_tile);
	mu_run_test(test_open_mine);
	mu_run_test(test_adjacent_mine_counts);
	mu_run_test(test_win_state);
	mu_run_test(test_callbacks);
	mu_run_test(test_flag_counts);
	mu_run_test(test_selected_tile);
	mu_run_test(test_cursor_movement);
	return 0;
}
 
int main(int argc, char **argv) {
	char *result = all_tests();
	if (result != 0) {
		printf("%s\n", result);
	}
	else {
		printf("ALL TESTS PASSED\n");
	}
	printf("Tests run: %d\n", tests_run);
	return 0;
}
