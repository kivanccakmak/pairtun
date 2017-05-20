#ifndef _CONFIG_H
#define _CONFIG_H

#define TUN_IFNAME "tun0"

#define IPC_SUN_PATH "\0ptun"
#define IPC_SUN_PATH_LEN 5

#define DESTINATION_PORT 9191
#define DESTINATION_IP "192.168.1.107"

#define PFD_IDX_IPC 0
#define PFD_IDX_TUN 1
#define PFD_IDX_NET_FIRST 2

#define INITIAL_NUM_FDS 3

#define MAX_NUM_NET_FDS 8
#define PFD_NUM_FDS PFD_IDX_NET_FIRST + MAX_NUM_NET_FDS

#define POLL_DURATION 1000 // milliseconds

#define MAX_IPC_MSG_SIZE 1024

#endif
