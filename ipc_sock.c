#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <errno.h>

#include "debug.h"
#include "ipc_sock.h"

static int recv_cmd(struct ipc *ipc)
{
    int rc, sfd, count = 0;
    char buf[MAX_IPC_MSG_SIZE] = {0};

    ptun_debugf("entering");
    sfd = accept(ipc->fd, NULL, NULL);
    if (sfd == -1) {
        ptun_errorf("accept() [%s]", strerror(errno));
        goto bail;
    }

    do {
        rc = recv(sfd, &buf[count], MAX_IPC_MSG_SIZE-count, 0);
        if (rc > 0) {
            count += rc;
        } else if (rc < 0) {
            ptun_errorf("recv() [%s]", strerror(errno));
            goto bail;
        }
    } while (count < MAX_IPC_MSG_SIZE && rc > 0);

    ptun_infof("buf: %s", buf);
    //TODO: manipulate remote
    close(sfd);
    return 0;
bail:
    close(sfd);
    return -1;
}

static int get_num_active_fds(struct ipc *ipc)
{
    return 1;
}

static void destroy_ipc_sock(struct ipc *ipc)
{
    ptun_infof("entering");
    if (ipc->fd != -1) {
        close(ipc->fd);
    }
}

/**
 * @brief would get commands and forwards to network connection
 *
 * @param[out] ipc
 *
 * @return
 */
int init_ipc_connection(struct ipc *ipc)
{
    int fd;
    int ret, yes = 1;
    struct sockaddr_un addr;

    ipc->cmd_handler = &recv_cmd;
    ipc->get_num_active_fds = &get_num_active_fds;
    ipc->destroy = &destroy_ipc_sock;

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

    ptun_infof("successfully created ipc");
    ipc->fd = fd;
    return 0;
bail:
    close(fd);
    return -1;
}
