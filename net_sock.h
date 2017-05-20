#ifndef _REMOTE_H
#define _REMOTE_H

#include <net/if.h>
#include <arpa/inet.h> 
#include "config.h"
#include "pqueue.h"

struct remote {
    struct sockaddr_in addr;
    char ip[16];
    int port;
    int fds[MAX_NUM_NET_FDS];
    int num_actives;
    int (*packet_handler) (int fd, int flag, struct pqueue_t *pq);
    int (*add_connection) (struct remote *remote);
    int (*get_num_active_fds) (struct remote *remote);
    int (*close_connection) (struct remote *remote);
    void (*destroy) (struct remote *remote);
};

int init_remote_sock(struct remote *remote, const char *ip, int port);

#endif
