#include <minunit.h>
#include <stdio.h>
#include <minesweeper.h>

int tests_run = 0;
int width = 120;
int height = 100;
struct board board;

static char * test_init() {
	board.data = NULL;
	board_init(&board, width, height, 0.1);
    mu_assert("Error: init must allocate board data.", board.data != NULL);
	mu_assert("Error: mines must not be placed until the first tile is opened.", board.mines_placed == false);
	mu_assert("Error: cursor should be centered after init", board.cursor_x == width / 2  && board.cursor_y == height / 2);
   	return 0;
}

static int count_adjacent_tiles(uint8_t **adjacent_tiles) {
	int count =  0;
	for (int i = 0; i < 8; i++) {
		if (adjacent_tiles[i]) {
			count++;
		}
	}
	return count;
}

static char * test_get_tile() {
	uint8_t *tile = get_tile_at(&board, 10, 10);
	mu_assert("Error: the tile at (10, 10) should exist after init.", tile != NULL);

	tile = get_tile_at(&board, -1, 10);
	mu_assert("Error: the tile at (-1, 10) shouldn't exist. get_tile_at must return NULL.", tile == NULL);

	tile = get_tile_at(&board, 101, 101);
	mu_assert("Error: the tile at (101, 101) shouldn't exist. get_tile_at must return NULL.", tile == NULL);
	return 0;
}

static char * test_get_adjacent_tiles() {
	uint8_t *tile = get_tile_at(&board, 0, 0);
	uint8_t *adjacent_tiles[8];
	get_adjacent_tiles(&board, tile, adjacent_tiles);
	mu_assert("Error: the tile at (0, 0) should have 3 adjacent tiles.", count_adjacent_tiles(adjacent_tiles) == 3);

	tile = get_tile_at(&board, 0, 1);
	get_adjacent_tiles(&board, tile, adjacent_tiles);
	mu_assert("Error: the tile at (0, 1) should have 5 adjacent tiles.", count_adjacent_tiles(adjacent_tiles) == 5);

	tile = get_tile_at(&board, 10, 10);
	get_adjacent_tiles(&board, tile, adjacent_tiles);
	mu_assert("Error: the tile at (10, 10) should 8 adjacent tiles.", count_adjacent_tiles(adjacent_tiles) == 8);
	return 0;
}

static char * test_open_first_tile() {
	open_tile_at_cursor(&board);
	mu_assert("Error: mines must be placed after first tile is opened", board.mines_placed == true);
	mu_assert("Error: there must not be a mine under the first opened tile", board.game_over == false);
	return 0;
}

static char * test_open_mine() {
	// Let's find a tile with a mine and open it...
	for (int x = 0; x < board.width; x++) {
		for (int y = 0; y < board.height; y++) {
			uint8_t *tile = get_tile_at(&board, x, y);
			if (*tile & TILE_MINE) {
				board.cursor_x = x;
				board.cursor_y = y;
				open_tile_at_cursor(&board);
				mu_assert("Error: After opening a mine tile, game_over must be true.", board.game_over == true);
				break;
			}
		}
	}
	return 0;
}

extern void place_mine(struct board *board, uint8_t *tile);

static char * test_adjacent_mine_counts() {
	board_deinit(&board);
	board_init(&board, width, height, 0.1);

	place_mine(&board, get_tile_at(&board, 10, 10));
	place_mine(&board, get_tile_at(&board, 10, 11));

	uint8_t *t = get_tile_at(&board, 9, 10);
	uint8_t mine_count = adjacent_mine_count(t);
	mu_assert("Error: the tile at (9, 10) must have a mine_count of 2 after mines have been placed at (10, 10) and (10, 11).", mine_count == 2);

	uint8_t *adj_tiles[8];
	get_adjacent_tiles(&board, t, adj_tiles);
	for (int i = 0; i < 8; i++) {
		place_mine(&board, adj_tiles[i]);
	}

	mine_count = adjacent_mine_count(t);
	mu_assert("Error: the tile at (9, 10) must have a mine_count of 8 after mines have been placed at every tile around it.", mine_count == 8);

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
