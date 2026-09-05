CXX=gcc

CFLAGS = -Wall -Wextra -g -O0
BIN = shell.exe

SRC = src

all:
	$(CXX) $(CFLAGS) $(SRC)/main.c $(SRC)/command.c $(SRC)/parser.c -o $(BIN)
