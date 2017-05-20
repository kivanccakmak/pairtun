CC=gcc

PTUN_TARGET=ptun
PTUN_SRC=ptun.c
PTUN_DEP=net_sock.c \
	ipc_sock.c \
	tun_sock.c \
	debug.c \
	pqueue.c \

CLI_TARGET=cli
CLI_SRC=ipc_cli.c
CLI_DEP=debug.c

default: all

all: ptun cli

ptun: $(PTUN_SRC) $(PTUN_DEP)
	$(CC) $(PTUN_SRC) $(PTUN_DEP) -o $(PTUN_TARGET)

cli: $(CLI_SRC) $(CLI_DEP)
	$(CC) $(CLI_SRC) $(CLI_DEP) -o $(CLI_TARGET)

clean:
	rm -rf $(PTUN_TARGET) $(CLI_TARGET)
