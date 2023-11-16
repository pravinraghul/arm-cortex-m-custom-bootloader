#ifndef PROTO_H_
#define PROTO_H_

#include "main.h"

enum proto_state_t {
STATE_RESET,
STATE_DOWNLOAD_START,
STATE_CONF_PACKET_RECEIVED,
STATE_DATA_PACKET_RECEIVED,
STATE_DOWNLOAD_COMPLETE
};

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
    enum proto_state_t state;
} sbp_handle_t;

/* receives command and data packets and sends response packets */
int proto_receive_packet(sbp_handle_t *handle);

#endif // PROTO_H_
