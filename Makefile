CC=g++
CFLAGS= -std=c++17 -O3 -Wall -Wextra -Wshadow -Wpedantic
CLIBS= -lX11
SRC=ytclip.cpp
PROG=${SRC:.cpp=}

all: build

clean:
	@rm -f $(PROG)

build:
	$(CC) -o $(PROG) $(SRC) $(CFLAGS) $(CLIBS)

