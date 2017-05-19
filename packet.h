#ifndef _PACKET_H
#define _PACKET_H

#include <stdint.h>

#define MAX_PAYLOAD_SIZE 65536

typedef enum {
    PTUN_V001 = 1,
} ptun_version_t;

typedef enum {
    CONTROL = 1,
    MANAGEMENT,
    DATA,
} ptun_packet_type_t;

typedef struct {
    uint8_t version;
    uint8_t type;
    uint16_t seq;
    uint16_t size;
} __attribute__ ((__packed__)) ptun_header_t;

typedef struct {
    ptun_header_t header;
    char payload[0];
} __attribute__((ptun_packet_t)) 

#endif
