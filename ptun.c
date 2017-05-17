#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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

struct remote {
    struct sockaddr_in addr;
    char ip[16];
    int port;
    int fds[MAX_NUM_NET_FDS];
    int num_actives;
};

struct pair_tun {
    struct remote remote;
    int tun_fd;
    int ipc_fd;
    struct pollfd *pfd;
};

/**
 * @brief 
 *
 * @param[in] dev
 * @param[in] flags
 *
 * @return 
 */
static int tun_alloc(char *dev, int flags) 
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
    return fd;
bail:
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
    if (remote->fds[0] != -1) {close(remote->fds[0]);}
    return -1;
}

static int init_pair_tun (struct pair_tun *ptun)
{
    int ret;
    int i;

    ptun->tun_fd = tun_alloc(TUN_IFNAME, IFF_TUN);
    if (ptun->tun_fd < 0) {
        goto bail;
    }

    ret = init_remote_connection(&ptun->remote, DESTINATION_IP, DESTINATION_PORT);
    if (ret != 0) {
        goto bail;
    }

    ptun->pfd = (struct pollfd*) malloc(PFD_NUM_FDS * sizeof(struct pollfd));
    memset(ptun->pfd, 0, PFD_NUM_FDS * sizeof(struct pollfd));

    //TODO: initialize ipc_fd
    ptun->pfd[PFD_IDX_IPC].fd = ptun->ipc_fd;
    ptun->pfd[PFD_IDX_IPC].events = POLLIN;
    
    ptun->pfd[PFD_IDX_TUN].fd = ptun->tun_fd;
    ptun->pfd[PFD_IDX_TUN].events = POLLIN;

    ptun->pfd[PFD_IDX_NET_FIRST].fd = ptun->remote.fds[0];
    ptun->pfd[PFD_IDX_NET_FIRST].events = POLLOUT;

    return 0;
bail:
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

    return 0;
bail:
    if (ptun.tun_fd != -1) {close(ptun.tun_fd);}
    return -1;
}
