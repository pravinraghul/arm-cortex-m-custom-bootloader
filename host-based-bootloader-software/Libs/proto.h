#ifndef PROTO_H_
#define PROTO_H_

#include "main.h"

typedef struct {
    uint32_t version;
    uint32_t size;
    uint32_t crc;
} sbp_config_t;

typedef struct {
    uint8_t bytes[1024];
    uint16_t size;
} sbp_data_t;

typedef struct {
    sbp_config_t config;
    sbp_data_t data;
    bool config_packet_received;
    bool app_packet_received;
    bool app_packet_complete;
} sbp_handle_t;

/* receives command and data packets and sends response packets */
int proto_receive_packet(sbp_handle_t *handle);

#endif // PROTO_H_
