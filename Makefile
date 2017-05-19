CC=gcc
TARGET=ptun
SRC=ptun.c
DEP=net_sock.c \
	ipc_sock.c \
	tun_sock.c \
	debug.c \
	pqueue.c \

default: all

all: $(SRC) $(DEP)
	$(CC) $(SRC) $(DEP) -o $(TARGET)

clean:
	rm -rf $(TARGET)
