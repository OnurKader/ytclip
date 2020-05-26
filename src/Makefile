CC=g++
CFLAGS= -std=c++2a -O3 -Wall -Wextra -Wshadow -Wpedantic -I../include
CLIBS= -lX11 -lfmt -lpthread
SRC= ytclip.cpp
PROG=${SRC:.cpp=}

all: build

clean:
	@rm -f $(PROG)

build:
	$(CC) -o $(PROG) $(SRC) $(CFLAGS) $(CLIBS)

