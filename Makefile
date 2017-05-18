CC=gcc
TARGET=ptun
SRC=ptun.c
DEP=debug.c \
	pqueue.c \

default: all

all: $(SRC) $(DEP)
	$(CC) $(SRC) $(DEP) -o $(TARGET)

clean:
	rm -rf $(TARGET)
