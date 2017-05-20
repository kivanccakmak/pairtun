#ifndef _CONFIG_H
#define _CONFIG_H

#define TUN_IFNAME "tun0"

#define IPC_SUN_PATH "\0ptun"
#define IPC_SUN_PATH_LEN 5

#define DESTINATION_PORT 9090
#define DESTINATION_IP "127.0.0.1"

#define PFD_IDX_IPC 0
#define PFD_IDX_TUN 1

#define PFD_IDX_NET_FIRST 2
#define MAX_NUM_NET_FDS 8
#define PFD_NUM_FDS PFD_IDX_NET_FIRST + MAX_NUM_NET_FDS

#define POLL_MSEC 1000

#define MAX_IPC_MSG_SIZE 1024

#endif
