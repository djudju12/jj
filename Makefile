CC=gcc
CFLAGS+= -std=c99 -Wall -Wextra -Wpedantic -Wundef -Wimplicit-fallthrough

# -O3 -march=native -mtune=native

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