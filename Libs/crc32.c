#include "crc32.h"

// MPEG-2 CRC32 polynomial
#define MPEG2_CRC32_POLYNOMIAL 0x04C11DB7

// Function to calculate CRC32 from the flash address
uint32_t crc32_calculate_from_flash(uint32_t addr, size_t length) {
    uint32_t crc = 0xFFFFFFFF;

    for (size_t i = 0; i < length; i++) {
        crc ^= *((volatile uint32_t *)(addr + i)) << 24;

        for (int j = 0; j < 8; j++) {
            crc = (crc << 1) ^ ((crc & 0x80000000) ? MPEG2_CRC32_POLYNOMIAL : 0);
        }
    }

    return crc;
}

// Function to calculate CRC32 from the memory
uint32_t crc32_calculate_from_memory(uint8_t *data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;

    for (size_t i = 0; i < length; i++) {
        crc ^= data[i] << 24;

        for (int j = 0; j < 8; j++) {
            crc = (crc << 1) ^ ((crc & 0x80000000) ? MPEG2_CRC32_POLYNOMIAL : 0);
        }
    }

    return crc;
}
