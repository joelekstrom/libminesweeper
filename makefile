C_FLAGS = --std=c99 -Wall -pedantic
CXX_FLAGS = --std=c++14 -Wall -pedantic

library = libminesweeper.a

$(library): lib/minesweeper.c
	$(CC) $(C_FLAGS) -c lib/minesweeper.c -Iinclude
	ar rcs $@ minesweeper.o
	rm *.o

.PHONY: run-c-tests, run-cpp-tests, run-all-tests, clean
run-c-tests: tests/c-tests
	tests/c-tests

run-cpp-tests: tests/cpp-tests
	tests/cpp-tests

run-all-tests: run-c-tests run-cpp-tests

tests/c-tests: $(library) tests/*c
	$(CC) $(C_FLAGS) tests/minesweeper_tests.c -Iinclude -Itests -L. -lminesweeper -o $@

tests/cpp-tests: $(library) tests/*cpp include/minesweeper.hpp
	$(CXX) $(CXX_FLAGS) tests/minesweeper_tests.cpp -Iinclude -Itests -L. -lminesweeper -o $@

clean:
	rm -f libminesweeper.a tests/c-tests tests/cpp-tests
