#ifndef _TUN_SOCK_H
#define _TUN_SOCK_H

#include "pqueue.h"

struct tun {
    int fd;
    int (*packet_handler)(int fd, struct pqueue_t *pq);
    int (*get_num_active_fds) (struct tun *tun);
    void (*destroy)(struct tun *tun);
};

int init_tun_sock(struct tun *tun, char *dev);

#endif
