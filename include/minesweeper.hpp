#pragma once
#include <vector>
#include <functional>
#include <memory>
#include <iostream>

extern "C" {
	#include <minesweeper.h>
}

namespace Minesweeper {
	class Game;

	class Tile {
		friend class Game;

	public:
		void open();
		void toggleFlag();
		void spaceTile();
		void toggleMine();
		uint8_t adjacentMineCount();
		bool hasFlag();
		bool isOpened();
		bool hasMine();
		std::vector<Tile> adjacentTiles();

		bool operator==(const Tile &rhs) {
			return internal == rhs.internal;
		}

	private:
		Tile(minesweeper_tile *internal, minesweeper_game *game): internal(internal), game(game) {}
		minesweeper_tile *internal;
		minesweeper_game *game;
	};

	class Game {

	public:
		Game(unsigned width, unsigned height, float mineDensity);
		unsigned width();
		unsigned height();
		unsigned mineCount();
		unsigned openedTileCount();
		unsigned flagCount();
		minesweeper_game_state state();
		void setCursor(unsigned x, unsigned y);
		void moveCursor(direction direction, bool should_wrap);
		Tile selectedTile();
		Tile tileAt(unsigned x, unsigned y);
		std::function<void(Game&, Tile&)> tileUpdateCallback;

	private:
		std::unique_ptr<uint8_t[]> buffer;
		minesweeper_game *internal;
	};

	extern "C" void callbackHandler(minesweeper_game *game, struct minesweeper_tile *tile, void *context) {
		Game *gameObject = (Game *)context;
		unsigned x, y; minesweeper_get_tile_location(game, tile, &x, &y);
		Tile tileObject = gameObject->tileAt(x, y);
		if (gameObject->tileUpdateCallback != nullptr) {
			gameObject->tileUpdateCallback(*gameObject, tileObject);
		}
	}

	inline Game::Game(unsigned width, unsigned height, float mineDensity) {
		buffer = std::make_unique<uint8_t[]>(minesweeper_minimum_buffer_size(width, height));
		internal = minesweeper_init(width, height, mineDensity, buffer.get());
		internal->tile_update_callback = &callbackHandler;
		internal->user_info = this;
	}

	inline unsigned Game::width() {
		return internal->width;
	}

	inline unsigned Game::height() {
		return internal->height;
	}

	inline unsigned Game::mineCount() {
		return internal->mine_count;
	}

	inline unsigned Game::openedTileCount() {
		return internal->opened_tile_count;
	}

	inline unsigned Game::flagCount() {
		return internal->flag_count;
	}

	inline minesweeper_game_state Game::state() {
		return internal->state;
	}

	inline void Game::setCursor(unsigned x, unsigned y) {
		if (x >= width() || y >= height())
			throw std::out_of_range("Desired tile is out of bounds for this game.");
		minesweeper_set_cursor(internal, x, y);
	}

	inline void Game::moveCursor(direction direction, bool should_wrap) {
		minesweeper_move_cursor(internal, direction, should_wrap);
	}

	inline Tile Game::selectedTile() {
		minesweeper_tile *tilePtr = internal->selected_tile;
		if (tilePtr == NULL)
			throw std::logic_error("No tile is selected. Call setCursor() first.");
		return Tile(tilePtr, this->internal);
	}

	inline Tile Game::tileAt(unsigned x, unsigned y) {
		minesweeper_tile *tilePtr = minesweeper_get_tile_at(internal, x, y);
		if (tilePtr == NULL)
			throw std::out_of_range("Tile is out of bounds for this game.");
		return Tile(tilePtr, this->internal);
	}

	inline void Tile::open() {
		minesweeper_open_tile(game, internal);
	}

	inline void Tile::toggleFlag() {
		minesweeper_toggle_flag(game, internal);
	}

	inline void Tile::spaceTile() {
		minesweeper_space_tile(game, internal);
	}

	inline uint8_t Tile::adjacentMineCount() {
		return internal->adjacent_mine_count;
	}

	inline void Tile::toggleMine() {
		minesweeper_toggle_mine(game, internal);
	}

	inline bool Tile::hasMine() {
		return internal->has_mine;
	}

	inline bool Tile::hasFlag() {
		return internal->has_flag;
	}

	inline bool Tile::isOpened() {
		return internal->is_opened;
	}

	inline std::vector<Tile> Tile::adjacentTiles() {
		minesweeper_tile *tiles[8];
		minesweeper_get_adjacent_tiles(game, internal, tiles);
		std::vector<Tile> list;

		for (int i = 0; i < 8; i++) {
			minesweeper_tile *tilePtr = tiles[i];
			if (tilePtr != NULL) {
				list.push_back(Tile(tilePtr, this->game));
			}
		}

		return list;
	}
}
