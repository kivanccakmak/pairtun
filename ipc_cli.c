#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#include "debug.h"
#include "config.h"

static int init_cli_fd()
{
    int yes=1, rc, ret, fd;
    struct sockaddr_un addr;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;

    memset(addr.sun_path, '\0', 108);
    memcpy(addr.sun_path, IPC_SUN_PATH, IPC_SUN_PATH_LEN);

    ptun_debugf("socket()");
    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        ptun_errorf("socket() [%s]", strerror(errno));
        goto bail;
    }

    ptun_debugf("setsockopt()");
    ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if (ret == -1) {
        ptun_errorf("setsockopt() [%s]", strerror(errno));
        goto bail;
    }

    ptun_debugf("connect()");
    ret = connect(fd, (struct sockaddr*) &addr, sizeof(addr));
    if (ret == -1) {
        ptun_errorf("connect() [%s]", strerror(errno));
        goto bail;
    }

    ptun_debugf("connected");
    return fd;
bail:
    close(fd);
    return -1;
}

static void usage ()
{
    ptun_errorf("invalid usage");
    ptun_errorf("./cli message");
    ptun_errorf("example: ./cli add");
    ptun_errorf("example: ./cli remove");
}

int main(int argc, char *argv[])
{
    int count = 0, rc, fd;
    size_t msg_size;
    char buf[MAX_IPC_MSG_SIZE + 1] = {0}, *pack_ptr = NULL;

    if (argc != 2) {
        usage(); 
        goto bail;
    }

    msg_size = strlen(argv[1]) + 1;
    if (msg_size > MAX_IPC_MSG_SIZE) {
        ptun_errorf("long message"); 
        goto bail;
    }

    fd = init_cli_fd();
    if (fd == -1) {
        goto bail;
    }

    rc = snprintf(buf, msg_size, "%s", argv[1]);
    if (rc == -1) {
        ptun_errorf("snprintf() [%s]", strerror(errno));
        goto bail;
    }

    pack_ptr = &buf[0];
    ptun_infof("trying to send \"%s\"", buf);
    do {
        rc = write(fd, pack_ptr, msg_size-count);
        if (rc > 0) {
            count += rc;
            pack_ptr += rc;
        } else if (rc == -1) {
            ptun_errorf("write() [%s]", strerror(errno));
            goto bail;
        }
    } while (msg_size > count && rc > 0);
    ptun_infof("sent %d bytes", count);

    close(fd);
    return 0;
bail:
    ptun_errorf("failed to send \"%s\"", argv[1]);
    close(fd);
    return -1;
}
