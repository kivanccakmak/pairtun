#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h> 
#include <sys/time.h>
#include <poll.h>
#include <errno.h>
#include <stdarg.h>

#include "config.h"
#include "debug.h"
#include "commons.h"
#include "pqueue.h"

struct remote {
    struct sockaddr_in addr;
    char ip[16];
    int port;
    int fds[MAX_NUM_NET_FDS];
    int num_actives;
    int (*packet_handler) (int fd, int flag, struct pqueue_t *pq);
    void (*destroy) (struct remote *remote);
};

struct ipc {
    int (*cmd_handler) (struct ipc *ipc, struct remote *remote);
    int fd;
    void (*destroy) (struct ipc *ipc);
};

struct tun {
    int (*packet_handler) (int fd, struct pqueue_t *pq);
    int fd;
    void (*destroy) (struct tun *tun);
};

struct pair_tun {
    struct remote remote;
    struct ipc ipc;
    struct tun tun;
    struct pollfd *pfd;
    struct pqueue_t *pq;
    int poll_msec;
    void (*destroy) (struct pair_tun *ptun);
};

/**
 * @brief 
 *
 * @param[out] tun
 * @param[in] dev
 * @param[in] flags
 *
 * @return 
 */
static int init_tun (struct tun *tun, char *dev, int flags)
{
    struct ifreq ifr;
    int fd, ret;
    char *clonedev = "/dev/net/tun";

    if (dev == NULL) {
        ptun_errorf("device name is null");
        goto bail;
    }

    fd = open(clonedev, O_RDWR);
    if (fd < 0) {
        ptun_errorf("failed to open tun iface [%s]", strerror(errno));
        goto bail;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = flags;
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    ret = ioctl(fd, TUNSETIFF, (void *)&ifr);
    if (ret < 0) {
        ptun_errorf("failed to set tun iface [%s]", strerror(errno));
        goto bail;
    }

    tun->fd = fd;
    tun->packet_handler = NULL;
    return 0;
bail:
    close(fd);
    return -1;
}

/**
 * @brief
 *
 * @param[out] ipc
 *
 * @return
 */
static int init_ipc_socket (struct ipc *ipc)
{
    int fd;
    int ret, yes = 1;
    struct sockaddr_un addr;

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        ptun_errorf("failed to init ipc socket [%s]", strerror(errno));
        goto bail;
    }

    ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if (ret == -1) {
        ptun_errorf("failed to setsockopt [%s]", strerror(errno));
        goto bail;
    }

    addr.sun_family = AF_UNIX;
    memset(addr.sun_path, '\0', 108);
    memcpy(addr.sun_path, IPC_SUN_PATH, IPC_SUN_PATH_LEN);

    ret = bind(fd, (struct sockaddr *) &addr, sizeof(addr));
    if (ret == -1) {
        ptun_errorf("failed to bind [%s]", strerror(errno));
        goto bail;
    }

    ret = listen(fd, 0);
    if (ret == -1) {
        ptun_errorf("failed to listen [%s]", strerror(errno));
        goto bail;
    }

    ipc->fd = fd;
    ipc->cmd_handler = NULL;
    return 0;
bail:
    close(fd);
    return -1;
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
static int init_remote_connection (struct remote *remote, const char *ip, int port)
{
    int ret;

    remote->addr.sin_family = AF_INET;
    remote->addr.sin_addr.s_addr = inet_addr(ip);
    remote->addr.sin_port = htons(port);

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

static int init_pair_tun (struct pair_tun *ptun)
{
    int ret;
    ptun->pfd = NULL;

    ret = init_tun(&ptun->tun, TUN_IFNAME, IFF_TUN);
    if (ret < 0) {
        goto bail;
    }

    ret = init_remote_connection(&ptun->remote, DESTINATION_IP, DESTINATION_PORT);
    if (ret != 0) {
        goto bail;
    }

    ret = init_ipc_socket(&ptun->ipc);
    if (ret != 0) {
        goto bail;
    }

    ptun->pfd = (struct pollfd*) malloc(PFD_NUM_FDS * sizeof(struct pollfd));
    if (ptun->pfd == NULL) {
        ptun_errorf("failed to allocate fds");
        goto bail;
    }

    //TODO: init priority queue

    memset(ptun->pfd, 0, PFD_NUM_FDS * sizeof(struct pollfd));

    ptun->pfd[PFD_IDX_IPC].fd = ptun->ipc.fd;
    ptun->pfd[PFD_IDX_IPC].events = POLLIN;
    
    ptun->pfd[PFD_IDX_TUN].fd = ptun->tun.fd;
    ptun->pfd[PFD_IDX_TUN].events = POLLIN;

    ptun->pfd[PFD_IDX_NET_FIRST].fd = ptun->remote.fds[0];
    ptun->pfd[PFD_IDX_NET_FIRST].events = POLLIN || POLLOUT;

    ptun->poll_msec = POLL_MSEC * 1000;

    return 0;
bail:
    sfree(ptun->pfd);
    return -1;
}

static void usage ()
{
    ptun_infof("./ptun tun_name dest_ip num_conns");
    ptun_infof("./ptun tun0 192.168.2.12 3");
}

int main (int argc, char *argv[])
{
    int ret;
    struct pair_tun ptun;
    memset(&ptun, 0, sizeof(struct pair_tun));

    ret = init_pair_tun(&ptun);
    if (ret != 0) {
        ptun_errorf("initialization failed");
        goto bail;
    }

    //task loop
    while (1) {
        int nfds_ready = poll(ptun.pfd, PFD_NUM_FDS, ptun.poll_msec);
        int i = 0;

        if (nfds_ready < 0) {ptun_errorf("poll() [%s]", strerror(errno)); continue;}
        ptun_debugf("nfds_ready: %d", nfds_ready);

        if (ptun.pfd[PFD_IDX_IPC].revents & POLLIN) {
            if (ptun.ipc.cmd_handler == NULL) {
                ptun_errorf("lack of command handler for ipc packets");
                goto bail;
            }
            ret = ptun.ipc.cmd_handler(&ptun.ipc, &ptun.remote);
        }

        if (ptun.pfd[PFD_IDX_TUN].revents & POLLIN) {
            if (ptun.ipc.cmd_handler == NULL) {
                ptun_errorf("lack of packet_handler for tun iface");
                goto bail;
            }
            ret = ptun.tun.packet_handler(ptun.tun.fd, ptun.pq);
        }

        while (i < ptun.remote.num_actives) {
            int index = i + PFD_IDX_NET_FIRST;
            if (ptun.remote.packet_handler == NULL) {
                ptun_errorf("lack of packet_handler for network iface");
                goto bail;
            }
            if (ptun.pfd[index].revents & POLLIN) {
                ptun_infof("POLLIN");
                ret = ptun.remote.packet_handler(ptun.pfd[index].fd, POLLIN, ptun.pq);
            } else if (ptun.pfd[index].revents & POLLOUT) {
                ptun_infof("POLLOUT");
                ret = ptun.remote.packet_handler(ptun.pfd[index].fd, POLLOUT, ptun.pq);
            }
            i += 1;
        }

        ptun_infof("here");
        if (!nfds_ready) {
            ptun_debugf("woke up due to timeout");
        }
    }

    return 0;
bail:
    //TODO: ptun.destroy(&ptun); (calls remote-ipc-tun destroy, free priority
    //queue and malloced poll pointer)
    return -1;
}
