CC=g++
CFLAGS= -std=c++17 -O3 -Wall -Wextra -Wshadow -Wpedantic
CLIBS= -lX11
PROG=ytclip

all: build

clean:
	@rm -f ytclip

build:
	$(CC) -o $(PROG) $(CFLAGS) $(CLIBS) ytclip.cpp

