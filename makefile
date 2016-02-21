C_FLAGS = -std=c99 -Wall

library = libminesweeper.a

$(library): lib/minesweeper.c
	$(CC) $(C_FLAGS) -c lib/minesweeper.c -Ilib
	ar rcs $@ minesweeper.o
	rm *.o

.PHONY: test, clean, clean-tests
test: tests/test
	tests/test

tests/test: $(library)
	$(CC) $(C_FLAGS) tests/minesweeper_tests.c -Ilib -Itests -L. -lminesweeper -o $@

clean:
	-rm libminesweeper.a
	-rm tests/test
