#ifndef CRC32_H_
#define CRC32_H_

#include "main.h"

uint32_t crc32_calculate_from_flash(uint32_t addr, size_t length);
uint32_t crc32_calculate_from_memory(uint8_t *data, size_t length);

#endif // CRC32_H_
