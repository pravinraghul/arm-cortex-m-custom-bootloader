#include "proto.h"
#include "SEGGER_RTT.h"

#include "crc32.h"
#include "usart.h"
// Simple Bootloader Protocol
//
//  Packet Format
//  |  HEADER SECTION  |  DATA SECTION  |
//  | SOF | TYPE | LEN |  DATA    | CRC |
//  |  1  |   1  |  2  | 4 - 1024 |  4  |
//
//  - SOF  | 0x5A
//
//  Type
//  - START  | 0x01
//  - STOP   | 0x23
//  - RESP   | 0x45
//  - CONF   | 0x67
//  - DATA   | 0x89
//
//  START
//  - To know start of the firmware download.
//  - No data field required.
//  STOP
//  - To know end of the firmware download.
//  - No data field required.
//  CONF
//  - To know firmware version, size & crc for the firmware
//  that it to be downloaded.
//  | VERSION | SIZE | CRC |
//  |    4    |   4  |  4  |
//  RESP
//  - Reponse of the bytes received by the target.
//  - No data field required.
//  - ACK  | 0x15
//  - NACK | 0x16
//  DATA
//  - The max size of the data field 1024 in data packet.
//  - Data intergrity is checked by CRC field.

// HEADER SECTION INDEX
#define SBP_HEADER_SOF_OFFSET 0
#define SBP_HEADER_TYPE_OFFSET 1
#define SBP_HEADER_LEN_OFFSET 2
// DATA 
#define SBP_DATA_OFFSET 3

// SOF
#define SBP_HEADER_SOF 0x5A

// TYPE
#define SBP_TYPE_START 0x01
#define SBP_TYPE_STOP 0x23
#define SBP_TYPE_RESP 0x45
#define SBP_TYPE_CONF 0x67
#define SBP_TYPE_DATA 0x89

// CONF TYPE DATA INDEX
#define SBP_CONF_VERSION_OFFSET 0
#define SBP_CONF_SIZE_OFFSET 4
#define SBP_CONF_CRC_OFFSET 8

// RESPONSE TYPE DATA VALUE
#define SBP_RESP_ACK 0x15
#define SBP_RESP_NACK 0x16


static int proto_receive_packet_header(uint8_t *data)
{
    uint16_t ret = 0;
    uint16_t index = 0;

    ret = HAL_UART_Receive(&huart5, &data[index], 1, HAL_MAX_DELAY);
    if (ret != HAL_OK)
        return -1;

    if (data[index] != SBP_HEADER_SOF) {
        SEGGER_RTT_printf(0, "proto_receive_packet_header: SOF failed \r\n");
        return -1;
    }
    index++;

    ret = HAL_UART_Receive(&huart5, &data[index], 1, HAL_MAX_DELAY);
    if (ret != HAL_OK)
        return -1;
    index++;

    ret = HAL_UART_Receive(&huart5, &data[index], 2, HAL_MAX_DELAY);
    if (ret != HAL_OK)
        return -1;

    return 0;
}

static int proto_receive_packet_nodata(void)
{
    uint16_t ret = 0;
    uint8_t data[8] = {0};

    ret = HAL_UART_Receive(&huart5, data, 8, HAL_MAX_DELAY);

    if (ret != HAL_OK)
        return -1;

    return 0;
}

static int proto_receive_packet_config(sbp_config_t *config, uint16_t len)
{
    uint16_t ret = 0;
    uint8_t data[12] = {0};
    uint32_t recv_crc = 0;
    uint32_t calc_crc = 0;

    if (len < 12)
        return -1;

    ret = HAL_UART_Receive(&huart5, data, 12, HAL_MAX_DELAY);
    if (ret != HAL_OK)
        return -1;

    ret = HAL_UART_Receive(&huart5, (uint8_t*)&recv_crc, 4, HAL_MAX_DELAY);
    if (ret != HAL_OK)
        return -1;

    calc_crc = crc32_calculate_from_memory(data, len);
    if (calc_crc != recv_crc) {
        SEGGER_RTT_printf(0, "proto_receive_packet_config: crc failed\r\n");
        SEGGER_RTT_printf(0, "proto_receive_packet_config: calc_crc = %x, recv_crc = %x \r\n", calc_crc, recv_crc);
        return -1;
    }

    memcpy(&config->version, data + 0, 4);
    memcpy(&config->size, data + 4, 4);
    memcpy(&config->crc, data + 8, 4);
    return 0;
}

static int proto_receive_packet_data(sbp_data_t *data, uint16_t len)
{
    uint16_t ret = 0;
    uint32_t calc_crc = 0;
    uint32_t recv_crc = 0;

    ret = HAL_UART_Receive(&huart5, data->bytes, len, HAL_MAX_DELAY);
    if (ret != HAL_OK)
        return -1;

    ret = HAL_UART_Receive(&huart5, (uint8_t*)&recv_crc, 4, HAL_MAX_DELAY);
    if (ret != HAL_OK)
        return -1;

    calc_crc = crc32_calculate_from_memory(data->bytes, len);
    if (calc_crc != recv_crc) {
        SEGGER_RTT_printf(0, "proto_receive_packet_data: crc failed \r\n");
        return -1;
    }

    return 0;
}

static void proto_transmit_packet_resp(uint8_t resp)
{
    uint8_t resp_pkt[12] = {0};
    uint32_t crc = 0;

    resp_pkt[SBP_HEADER_SOF_OFFSET] = SBP_HEADER_SOF;
    resp_pkt[SBP_HEADER_TYPE_OFFSET] = SBP_TYPE_RESP;
    resp_pkt[SBP_HEADER_LEN_OFFSET] = 4;
    resp_pkt[SBP_DATA_OFFSET] = resp;

    crc = crc32_calculate_from_memory(&resp, 1);

    memcpy(resp_pkt + SBP_DATA_OFFSET + 4, (uint8_t*)&crc, 4);

    HAL_UART_Transmit(&huart5, resp_pkt, sizeof(resp_pkt), HAL_MAX_DELAY);
}

int proto_receive_packet(sbp_handle_t *handle)
{
    uint8_t header[4] = {0};

    // clear all the config
    handle->config.version = 0;
    handle->config.size = 0;
    handle->config.crc = 0;

    // clear all the data
    memset(handle->data.bytes, 0, 1024);
    handle->data.size = 0;

    // clear all the flags
    handle->app_packet_received = false;
    handle->app_packet_complete = false;
    handle->config_packet_received = false;

    if (proto_receive_packet_header(header) != 0) {
        SEGGER_RTT_printf(0, "proto_receive_packet: header receive failed \r\n");
        proto_transmit_packet_resp(SBP_RESP_NACK);
        return -1;
    }

    memcpy(&handle->data.size, header + SBP_HEADER_LEN_OFFSET, 2);

    switch (header[SBP_HEADER_TYPE_OFFSET]) {
        case SBP_TYPE_START:
            SEGGER_RTT_printf(0, "proto_receive_packet: start packet type received \r\n");
            if (proto_receive_packet_nodata() != 0) {
                SEGGER_RTT_printf(0, "proto_receive_packet: start receive failed \r\n");
                proto_transmit_packet_resp(SBP_RESP_NACK);
                return -1;
            }
            break;
        case SBP_TYPE_STOP:
            SEGGER_RTT_printf(0, "proto_receive_packet: stop packet type received \r\n");
            if (proto_receive_packet_nodata() != 0) {
                SEGGER_RTT_printf(0, "proto_receive_packet: stop receive failed \r\n");
                proto_transmit_packet_resp(SBP_RESP_NACK);
                return -1;
            }
            handle->app_packet_complete = true;
            break;
        case SBP_TYPE_CONF:
            SEGGER_RTT_printf(0, "proto_receive_packet: config packet type received \r\n");
            if (proto_receive_packet_config(&handle->config, handle->data.size) != 0) {
                SEGGER_RTT_printf(0, "proto_receive_packet: config receive failed \r\n");
                proto_transmit_packet_resp(SBP_RESP_NACK);
                return -1;
            }
            handle->config_packet_received = true;
            break;
        case SBP_TYPE_DATA:
            SEGGER_RTT_printf(0, "proto_receive_packet: data packet type received \r\n");
            if (proto_receive_packet_data(&handle->data, handle->data.size) != 0) {
                SEGGER_RTT_printf(0, "proto_receive_packet: data receive failed \r\n");
                proto_transmit_packet_resp(SBP_RESP_NACK);
                return -1;
            }
            handle->app_packet_received = true;
            break;
    }

    proto_transmit_packet_resp(SBP_RESP_ACK);

    return 0;
}
