CC=gcc
CFLAGS= -ggdb -std=c99 -Wall -Wextra -Wpedantic -Wundef -Wimplicit-fallthrough

build: out/ src/jj.c
	$(CC) $(CFLAGS) -o out/jj src/jj.c

out/:
	mkdir -p out

run: build
	./out/jj

test: src/jj.c
	$(CC) $(CFLAGS) -ggdb -o out/tests src/tests.c
	out/tests

debug: out/ src/jj.c
	$(CC) $(CFLAGS) -g -o out/jj src/jj.c
	gf2 out/jj &

debug/test:
	$(CC) $(CFLAGS) -ggdb -o out/tests src/tests.c
	gf2 out/tests &
