#include <minunit.h>
#include <stdio.h>
#include <minesweeper.h>
#include <stdlib.h>

int tests_run = 0;
int width = 120;
int height = 100;

uint8_t *board_buffer = NULL;
struct board *board = NULL;

static char * test_init() {
	board_buffer = malloc(minimum_buffer_size(width, height));
	board = board_init(width, height, 1.0, board_buffer);
    mu_assert("Error: board data must be offset in the buffer.", board->_data - sizeof(struct board) == (uint8_t *)board);
	mu_assert("Error: cursor should be centered after init", board->cursor_x == width / 2  && board->cursor_y == height / 2);
	mu_assert("Error: after init, state must be pending_start", board->_state == BOARD_PENDING_START);
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
	uint8_t *tile = get_tile_at(board, 10, 10);
	mu_assert("Error: the tile at (10, 10) should exist after init.", tile != NULL);

	tile = get_tile_at(board, -1, 10);
	mu_assert("Error: the tile at (-1, 10) shouldn't exist. get_tile_at must return NULL.", tile == NULL);

	tile = get_tile_at(board, 101, 101);
	mu_assert("Error: the tile at (101, 101) shouldn't exist. get_tile_at must return NULL.", tile == NULL);
	return 0;
}

static char * test_get_adjacent_tiles() {
	uint8_t *tile = get_tile_at(board, 0, 0);
	uint8_t *adjacent_tiles[8];
	get_adjacent_tiles(board, tile, adjacent_tiles);
	mu_assert("Error: the tile at (0, 0) should have 3 adjacent tiles.", count_adjacent_tiles(adjacent_tiles) == 3);

	tile = get_tile_at(board, 0, 1);
	get_adjacent_tiles(board, tile, adjacent_tiles);
	mu_assert("Error: the tile at (0, 1) should have 5 adjacent tiles.", count_adjacent_tiles(adjacent_tiles) == 5);

	tile = get_tile_at(board, 10, 10);
	get_adjacent_tiles(board, tile, adjacent_tiles);
	mu_assert("Error: the tile at (10, 10) should 8 adjacent tiles.", count_adjacent_tiles(adjacent_tiles) == 8);
	return 0;
}

static char * test_open_first_tile() {
	open_tile_at_cursor(board);
	mu_assert("Error: after opening the first tile, state should be: playing", board->_state == BOARD_PLAYING);
	mu_assert("Error: there must not be a mine under the first opened tile", !(*get_tile_at(board, board->cursor_x, board->cursor_y) & TILE_MINE));
	return 0;
}

static char * test_open_mine() {
	board->cursor_x = 0;
	board->cursor_y = 0;
	open_tile_at_cursor(board);
	mu_assert("Error: After opening a mine tile, state must be game_over.", board->_state == BOARD_GAME_OVER);
	return 0;
}

extern void place_mine(struct board *board, uint8_t *tile);

static char * test_adjacent_mine_counts() {
	uint8_t *t;
	uint8_t *adj_tiles[8];
	uint8_t mine_count;
	int i;

	board = board_init(width, height, 0.1, board_buffer);

	place_mine(board, get_tile_at(board, 10, 10));
	place_mine(board, get_tile_at(board, 10, 11));

	t = get_tile_at(board, 9, 10);
	mine_count = adjacent_mine_count(t);
	mu_assert("Error: the tile at (9, 10) must have a mine_count of 2 after mines have been placed at (10, 10) and (10, 11).", mine_count == 2);

	get_adjacent_tiles(board, t, adj_tiles);
	for (i = 0; i < 8; i++) {
		place_mine(board, adj_tiles[i]);
	}

	mine_count = adjacent_mine_count(t);
	mu_assert("Error: the tile at (9, 10) must have a mine_count of 8 after mines have been placed at every tile around it.", mine_count == 8);

	return 0;
}

static char * test_win_state() {
	/* Init the board with zero mines */
	board = board_init(width, height, 0.0, board_buffer);
	open_tile_at_cursor(board);
	mu_assert("Error: when 0 mines exist, all tiles should be opened after the first tile is opened", board->_opened_tile_count == width * height);
	mu_assert("Error: when all tiles are opened, state should be WIN", board->_state == BOARD_WIN);
	return 0;
}

static unsigned callback_count = 0;
void callback(struct board *board, uint8_t *tile, int x, int y) {
	callback_count++;
}

static char * test_callbacks() {
	board = board_init(width, height, 0.0, board_buffer);
	toggle_flag_at_cursor(board);
	mu_assert("Error: if no callback function is assigned, no callbacks should fire.", callback_count == 0);
	board->on_tile_updated = &callback;
	toggle_flag_at_cursor(board);
	mu_assert("Error: if a callback function is assigned, a callback should fire when a flag is toggled.", callback_count == 1);
	callback_count = 0;
	open_tile_at_cursor(board);
	mu_assert("Error: when 0 mines exist, a change callback should be sent for every tile.", callback_count == width * height);
	return 0;
}

static char * all_tests() {
	puts("Test: Initialization...");
	mu_run_test(test_init);

	puts("Test: Get tile...");
	mu_run_test(test_get_tile);

	puts("Test: Get adjacent tiles...");
	mu_run_test(test_get_adjacent_tiles);

	puts("Test: Open first tile...");
	mu_run_test(test_open_first_tile);

	puts("Test: Open mine...");
	mu_run_test(test_open_mine);

	puts("Test: Adjacent mine counters...");
	mu_run_test(test_adjacent_mine_counts);

	puts("Test: 0 mines/Win state...");
	mu_run_test(test_win_state);

	puts("Test: Changed tile callbacks...");
	mu_run_test(test_callbacks);
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
