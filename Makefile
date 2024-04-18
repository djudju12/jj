CC=gcc
CFLAGS+= -std=c99 -Wall -Wextra -Wpedantic -Wundef -Wimplicit-fallthrough

# -O3 -march=native -mtune=native

out/:
	mkdir -p out

build: out/ src/main.c
	$(CC) $(CFLAGS) -o out/jj src/main.c

run: build
	./out/jj

debug: out/ src/main.c
	$(CC) $(CFLAGS) -g -o out/jj src/main.c
	gf2 out/jj &