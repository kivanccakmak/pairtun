CC=gcc
TARGET=ptun
DEP=debug.c
SRC=ptun.c

default: all

all: $(SRC) $(DEP)
	$(CC) $(SRC) $(DEP) -o $(TARGET)

clean:
	rm -rf $(TARGET)
