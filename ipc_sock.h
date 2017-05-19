#ifndef _IPC_H
#define _IPC_H

#include "net_sock.h"

struct ipc {
    int fd;
    struct remote *remote;
    int (*cmd_handler) (struct ipc *ipc);
    void (*destroy) (struct ipc *ipc);
};

int init_ipc_connection(struct ipc *ipc);

#endif
