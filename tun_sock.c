#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>
#include <linux/if_tun.h>
#include <sys/types.h>
#include <fcntl.h>

#include "debug.h"
#include "tun_sock.h"

static void destroy_tun_sock(struct tun *tun)
{
    ptun_infof("entering");
    if (tun->fd != -1) {
        close(tun->fd);
    }
}

/**
 * @brief 
 *
 * @param[out] tun
 * @param[in] dev
 *
 * @return 
 */
int init_tun_sock(struct tun *tun, char *dev)
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
    ifr.ifr_flags = IFF_TUN;
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    ret = ioctl(fd, TUNSETIFF, (void *)&ifr);
    if (ret < 0) {
        ptun_errorf("failed to set tun iface [%s]", strerror(errno));
        goto bail;
    }

    tun->fd = fd;
    tun->packet_handler = NULL;
    tun->destroy = &destroy_tun_sock;
    return 0;
bail:
    close(fd);
    return -1;
}