#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "debug.h"
#include "net_sock.h"

static int handle_net_packet(int fd, int flag, struct pqueue_t *pq)
{
    //TODO: if (pollin) {recv, insert to queue}
    //TODO: if (pollout) {get from queue, send}
    ptun_infof("entering");
    return 0;
}

static int add_net_sock(struct remote *remote)
{
    int ret, idx;
    ptun_infof("entering");
    if (remote->num_actives == MAX_NUM_NET_FDS) {
        ptun_infof("no more active socket allowed");
        goto bail;
    }

    ptun_infof("adding connection %d", remote->num_actives);
    idx = remote->num_actives;

    remote->fds[idx] = socket(AF_INET, SOCK_STREAM, 0);
    if (remote->fds[idx] < 0) {
        ptun_errorf("socket() [%s]", strerror(errno));
        goto bail;
    }

    ret = connect(remote->fds[idx], (struct sockaddr*) &remote->addr,
            sizeof(remote->addr));
    if (ret < 0) {
        ptun_errorf("failed to connect [%s]", strerror(errno));
        goto bail;
    }

    remote->num_actives += 1;
    ptun_infof("added succesfully");
    return 0;
bail:
    return -1;
}

static int close_net_sock(struct remote *remote)
{
    int ret, idx;
    ptun_infof("entering");
    if (remote->num_actives == 0) {
        ptun_errorf("no active socket exists");
        goto bail;
    }

    idx = remote->num_actives - 1;
    ret = close(remote->fds[idx]);
    if (ret != 0) {
        ptun_errorf("close() [%s]", strerror(errno));
        goto bail;
    }
    return 0;
bail:
    return -1;
}

static int get_num_active_fds(struct remote *remote)
{
    return remote->num_actives;
}

static void destroy_net_sock(struct remote *remote)
{
    int i;
    ptun_infof("entering");
    for (i = 0; i < remote->num_actives; i++) {
        ptun_debugf("close network connection %d", i);
        close(remote->fds[i]);
    }
}

/**
 * @brief connect to remote device
 *
 * @param[out] remote
 * @param[in] ip
 * @param[in] port
 *
 * @return 
 */
int init_remote_sock(struct remote *remote, const char *ip, int port)
{
    int ret;

    remote->addr.sin_family = AF_INET;
    remote->addr.sin_addr.s_addr = inet_addr(ip);
    remote->addr.sin_port = htons(port);
    remote->num_actives = 0;

    remote->packet_handler = &handle_net_packet;
    remote->add_connection = &add_net_sock;
    remote->close_connection = &close_net_sock;
    remote->destroy = &destroy_net_sock;
    remote->get_num_active_fds = &get_num_active_fds;

    remote->fds[0] = socket(AF_INET, SOCK_STREAM, 0);
    if (remote->fds[0] < 0) {
        ptun_errorf("socket() [%s]", strerror(errno));
        goto bail;
    }

    ret = connect(remote->fds[0], (struct sockaddr*) &remote->addr,
            sizeof(remote->addr));
    if (ret < 0) {
        ptun_errorf("failed to connect [%s]", strerror(errno));
        goto bail;
    }

    remote->num_actives = 1;
    ptun_debugf("succesfully connected to %s", ip);
    return 0;
bail:
    close(remote->fds[0]);
    return -1;
}
