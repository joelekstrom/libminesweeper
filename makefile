C_FLAGS = -std=c89 -Wall -pedantic

library = libminesweeper.a

$(library): lib/minesweeper.c
	$(CC) $(C_FLAGS) -c lib/minesweeper.c -Iinclude
	ar rcs $@ minesweeper.o
	rm *.o

.PHONY: test, clean, clean-tests
test: tests/test
	tests/test

tests/test: $(library) tests/*c
	$(CC) $(C_FLAGS) tests/minesweeper_tests.c -Iinclude -Itests -L. -lminesweeper -o $@

clean:
	-rm libminesweeper.a
	-rm tests/test
