CC     ?= cc
CFLAGS  = -std=gnu11 -Wall -Wextra -Wshadow -Iinclude
LIB     = src/html_string.c src/html_render.c src/html_validate.c

all: html_demo

html_demo: src/main.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $^

# not part of "all"; run with: make test
test: tests/test.c $(LIB)
	$(CC) $(CFLAGS) -o html_test $^
	./html_test

clean:
	rm -f html_demo html_test

.PHONY: all test clean
