CXX=gcc

CFLAGS = -Wall -Wextra -g -O0
BIN = shell.exe

SRC = src

all:
	$(CXX) $(CFLAGS) $(SRC)/main.c -o $(BIN)
