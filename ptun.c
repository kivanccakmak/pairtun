#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <poll.h>
#include <errno.h>

#include "net_sock.h"
#include "ipc_sock.h"
#include "tun_sock.h"
#include "config.h"
#include "pqueue.h"
#include "commons.h"
#include "debug.h"

struct pair_tun {
    struct remote remote;
    struct ipc ipc;
    struct tun tun;
    struct pollfd *pfd;
    struct pqueue_t *pq;
    int poll_msec;
    void (*destroy) (struct pair_tun *ptun);
};

static void destroy_pair_tun(struct pair_tun *ptun)
{
    ptun_infof("entering");
    ptun->remote.destroy(&ptun->remote);
    ptun->ipc.destroy(&ptun->ipc);
    ptun->tun.destroy(&ptun->tun);
    if (ptun->pq != NULL) {
        pqueue_free(ptun->pq);
    }
    sfree(ptun->pfd);
}

static int init_pair_tun(struct pair_tun *ptun)
{
    int ret;
    ptun->pfd = NULL;
    ptun->pq = NULL;

    ptun->pfd = (struct pollfd*) malloc(PFD_NUM_FDS * sizeof(struct pollfd));
    if (ptun->pfd == NULL) {
        ptun_errorf("failed to allocate fds");
        goto bail;
    }
    memset(ptun->pfd, 0, PFD_NUM_FDS * sizeof(struct pollfd));
    ptun->destroy = &destroy_pair_tun;

    // to get intercepted packets
    ret = init_tun_sock(&ptun->tun, TUN_IFNAME);
    if (ret < 0) {goto bail;}

    // to forward intercepted packets through network
    ret = init_remote_sock(&ptun->remote, DESTINATION_IP, DESTINATION_PORT);
    if (ret != 0) {goto bail;}

    // to get cli commands
    ret = init_ipc_connection(&ptun->ipc);
    if (ret != 0) {goto bail;}

    ptun->pfd[PFD_IDX_IPC].fd = ptun->ipc.fd;
    ptun->pfd[PFD_IDX_IPC].events = POLLIN;

    ptun->pfd[PFD_IDX_TUN].fd = ptun->tun.fd;
    ptun->pfd[PFD_IDX_TUN].events = POLLIN;

    ptun->pfd[PFD_IDX_NET_FIRST].fd = ptun->remote.fds[0];
    ptun->pfd[PFD_IDX_NET_FIRST].events = POLLIN || POLLOUT;

    ptun->poll_msec = POLL_MSEC * 1000;

    //TODO: init priority queue
    return 0;
bail:
    return -1;
}

static void usage()
{
    ptun_infof("./ptun tun_name dest_ip num_conns");
    ptun_infof("./ptun tun0 192.168.2.12 3");
}

static int check_handlers(struct pair_tun *ptun)
{
    if (ptun->ipc.cmd_handler == NULL) {
        ptun_errorf("no command handler for ipc iface");
        goto bail;
    }

    if (ptun->tun.packet_handler == NULL) {
        ptun_errorf("no packet handler for tun iface");
        goto bail;
    }

    if (ptun->remote.packet_handler == NULL) {
        ptun_errorf("no packet handler for net iface");
        goto bail;
    }

    return 0;
bail:
    return -1;
}

static int task_loop(struct pair_tun *ptun)
{
    int ret;
    ret = check_handlers(ptun);
    if (ret != 0) {goto bail;}

    while (1) {
        int nfds_ready, i = 0;
        nfds_ready = poll(ptun->pfd, PFD_NUM_FDS, ptun->poll_msec);
        i = 0;

        if (nfds_ready < 0) {
            ptun_errorf("poll() [%s]", strerror(errno));
            continue;
        }

        if (ptun->pfd[PFD_IDX_IPC].revents & POLLIN) {
            ret = ptun->ipc.cmd_handler(&ptun->ipc);
        }

        if (ptun->pfd[PFD_IDX_TUN].revents & POLLIN) {
            ret = ptun->tun.packet_handler(ptun->tun.fd, ptun->pq);
        }

        while (i < ptun->remote.num_actives) {
            int index = i + PFD_IDX_NET_FIRST;
            if (ptun->pfd[index].revents & POLLIN) {
                ret = ptun->remote.packet_handler(ptun->pfd[index].fd, POLLIN, ptun->pq);
            } else if (ptun->pfd[index].revents & POLLOUT) {
                ret = ptun->remote.packet_handler(ptun->pfd[index].fd, POLLOUT, ptun->pq);
            }
            i += 1;
        }

        if (!nfds_ready) {
            ptun_debugf("woke up due to timeout");
        }
    }
    return 0;
bail:
    return -1;
}

int main(int argc, char *argv[])
{
    int ret;
    struct pair_tun ptun;
    memset(&ptun, 0, sizeof(struct pair_tun));

    ret = init_pair_tun(&ptun);
    if (ret != 0) {
        ptun_errorf("initialization failed");
        goto bail;
    }

    ret = task_loop(&ptun);
    if (ret != 0) {
        ptun_errorf("task_loop failed");
        goto bail;
    }

    return 0;
bail:
    ptun.destroy(&ptun);
    return -1;
}
