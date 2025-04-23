#include <minunit.h>
#include <stdio.h>
#include <minesweeper.hpp>

#define assertTrue(MESSAGE, TEST) mu_assert((char *)MESSAGE, TEST)
#define assertFalse(MESSAGE, TEST) mu_assert((char *)MESSAGE, !(TEST))
#define assertException(MESSAGE, CODE) do {try {CODE; assertTrue(MESSAGE, false);} catch (...) {assertTrue("", true);}} while(0)
#define assertNoException(MESSAGE, CODE) do {try {CODE; assertTrue("", true);} catch (...) {assertTrue(MESSAGE, false);}} while(0)

#include <iostream>

int tests_run = 0;
unsigned int width = 120;
unsigned int height = 100;

static void mu_setup() {}

static char * test_init() {
	puts("Test: Initialization...");
	Minesweeper::Game game = Minesweeper::Game(width, height, 1.0);
	assertTrue("Error: after init, state must be pending_start", game.state() == MINESWEEPER_PENDING_START);
	assertTrue("Error: after init, width and height must be returned correctly", game.width() == width && game.height() == height);
	return 0;
}

static char * test_get_tile() {
	puts("Test: Get tile...");
	Minesweeper::Game game = Minesweeper::Game(width, height, 1.0);
	assertNoException("Error: the tile at (10, 10) should exist after init.", game.tileAt(10, 10););
	assertException("Error: the tile at (-1, 10) should throw an exception.", game.tileAt(-1, 10));
	assertException("Error: the tile at (101, 101) should throw an exception.", game.tileAt(101, 101));
	return 0;
}

static char * test_get_adjacent_tiles() {
	puts("Test: Get adjacent tiles...");
	Minesweeper::Game game = Minesweeper::Game(width, height, 1.0);
	Minesweeper::Tile tile = game.tileAt(0, 0);
	auto adjacentTiles = tile.adjacentTiles();
	assertTrue("Error: the tile at (0, 0) should have 3 adjacent tiles.", adjacentTiles.size() == 3);

	tile = game.tileAt(0, 1);
	adjacentTiles = tile.adjacentTiles();
	assertTrue("Error: the tile at (0, 1) should have 5 adjacent tiles.", adjacentTiles.size() == 5);

	tile = game.tileAt(10, 10);
	adjacentTiles = tile.adjacentTiles();
	assertTrue("Error: the tile at (10, 10) should have 8 adjacent tiles.", adjacentTiles.size() == 8);
	return 0;
}

static char * test_open_first_tile() {
	puts("Test: Open first tile...");
	Minesweeper::Game game = Minesweeper::Game(width, height, 1.0);
	game.setCursor(width / 2, height / 2);
	game.selectedTile().open();
	assertTrue("Error: after opening the first tile, state should be: playing", game.state() == MINESWEEPER_PLAYING);
	assertFalse("Error: there must not be a mine under the first opened tile", game.selectedTile().hasMine());
	return 0;
}

static char * test_open_mine() {
	puts("Test: Open mine...");
	Minesweeper::Game game = Minesweeper::Game(width, height, 1.0);
	game.setCursor(0, 0);
	game.selectedTile().open();
	game.setCursor(0, 10);
	if (!game.selectedTile().hasMine()) {
		game.selectedTile().toggleMine();
	}
	game.selectedTile().open();
	assertTrue("Error: After opening a mine tile, state must be game_over.", game.state() == MINESWEEPER_GAME_OVER);
	return 0;
}

static char * test_adjacent_mine_counts() {
	puts("Test: Adjacent mine counters...");
	Minesweeper::Game game = Minesweeper::Game(width, height, 0.0);
	Minesweeper::Tile centerTile = game.tileAt(10, 10);
	Minesweeper::Tile leftTile = game.tileAt(9, 10);
	Minesweeper::Tile rightTile = game.tileAt(11, 10);

	leftTile.toggleMine();
	rightTile.toggleMine();
	
	assertTrue("Error: the tile at (10, 10) must have a mine_count of 2 after mines have been placed at (9, 10) and (11, 10).", centerTile.adjacentMineCount() == 2);

	// Toggle mines on all tiles adjacent to the center tile, except
	// left and right tiles which already have mines
	for (auto &tile: centerTile.adjacentTiles()) {
		if (!tile.hasMine()) {
			tile.toggleMine();
		}
	}
	
	assertTrue("Error: the tile at (10, 10) must have a mine_count of 8 after mines have been placed at every tile around it.", centerTile.adjacentMineCount() == 8);

	leftTile.toggleMine();
	assertTrue("Error: the tile at (10, 10) must have a mine_count of 7 after the mine has been toggled off on the tile to the left of it.", centerTile.adjacentMineCount() == 7);
	return 0;
}

static char * test_win_state() {
	puts("Test: 0 mines/Win state...");
	/* Init the game with zero mines */
	Minesweeper::Game game = Minesweeper::Game(width, height, 0.0);
	game.setCursor(0, 0);
	game.selectedTile().open();
	assertTrue("Error: when 0 mines exist, all tiles should be opened after the first tile is opened", game.openedTileCount() == width * height);
	assertTrue("Error: when all tiles are opened, state should be WIN", game.state() == MINESWEEPER_WIN);
	return 0;
}

static char * test_callbacks() {
	puts("Test: Changed tile callbacks...");
	Minesweeper::Game game = Minesweeper::Game(width, height, 0.0);
	unsigned int callbackCount = 0;
	game.tileUpdateCallback = [&callbackCount](Minesweeper::Game&, Minesweeper::Tile&) { callbackCount++; };
	game.setCursor(width / 2, height / 2);
	game.selectedTile().toggleFlag();
	assertTrue("Error: if a callback function is assigned, a callback should fire when a flag is toggled.", callbackCount == 1);

	game.selectedTile().toggleFlag();
	callbackCount = 0;
	game.selectedTile().open();
	assertTrue("Error: when 0 mines exist, a change callback should be sent for every tile.", callbackCount == width * height);
	return 0;
}

static char * test_flag_counts() {
	puts("Test: Flag counts...");
	Minesweeper::Game game = Minesweeper::Game(width, height, 1.0);
	game.setCursor(width / 2, height / 2);
	game.selectedTile().toggleFlag();
	assertTrue("Error: after placing a flag, _flag_count should increase.", game.flagCount() == 1);
	game.selectedTile().toggleFlag();
	assertTrue("Error: after toggling flag at the same tile, _flag_count should decrease.", game.flagCount() == 0);
	game.selectedTile().open();
	game.selectedTile().toggleFlag();
	assertTrue("Error: when attempting to toggle flag at an already opened tile, nothing should happen, and flag count should stay at 0.", game.flagCount() == 0);
	return 0;
}

static char * test_selected_tile() {
	puts("Test: Selected tile...");
	Minesweeper::Game game = Minesweeper::Game(width, height, 0.0);
	assertException("Error: Selected tile raise an exception if cursor position isn't set", game.selectedTile());
	game.setCursor(10, 10);
	assertTrue("Error: Selected tile should be at (10, 10) after a call to set_cursor", game.selectedTile() == game.tileAt(10, 10));
	return 0;
}

static char * test_cursor_movement() {
	puts("Test: Cursor movement...");
	Minesweeper::Game game = Minesweeper::Game(width, height, 0.0);
	game.setCursor(10, 10);
	game.moveCursor(RIGHT, false);
	game.moveCursor(RIGHT, false);
	game.moveCursor(DOWN, false);
	assertTrue("Error: Selected tile should be at (12, 11) after cursor moving", game.selectedTile() == game.tileAt(12, 11));
	game.setCursor(0, 0);
	game.moveCursor(LEFT, false);
	assertTrue("Error: Cursor should not move outside of bounds if should_wrap is false", game.selectedTile() == game.tileAt(0, 0));
	game.moveCursor(LEFT, true);
	game.moveCursor(UP, true);
	assertTrue("Error: Cursor should have wrapped to other side of the board if shouldWrap is true",  game.selectedTile() == game.tileAt(width - 1, height - 1));
	return 0;
}

static char * test_space_flag_tile() {
	puts("Test: space flagged tile..");
	Minesweeper::Game game = Minesweeper::Game(width, height, 0.0);
	game.setCursor(width / 2, height / 2);
	game.selectedTile().spaceTile();

	assertTrue("Error: unopened tile should get flagged by space.", game.selectedTile().hasFlag());
	return 0;
}

static char * test_space_open_tile() {
	puts("Test: space opened tile..");
	Minesweeper::Game game = Minesweeper::Game(width, height, 0.0);


	Minesweeper::Tile zero_tile = game.tileAt(0, 0);

	zero_tile.toggleMine();
	game.setCursor(0,1);
	game.selectedTile().open();
	game.setCursor(0,0);
	game.selectedTile().toggleFlag();
	game.setCursor(0,1);
	game.selectedTile().spaceTile();
	game.setCursor(game.width() -1, game.height() -1 );
	assertTrue("Error: all tiles should get opened by space.", game.selectedTile().isOpened());

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
	mu_run_test(test_space_flag_tile);
	mu_run_test(test_space_open_tile);
	return 0;
}
 
int main() {
	puts("Running C++ tests...");
	const char *result = all_tests();
	if (result != 0) {
		printf("%s\n", result);
	}
	else {
		puts("ALL TESTS PASSED");
	}
	printf("Tests run: %d\n", tests_run);
	return result == 0 ? 0 : 1;
}
