#ifndef BOOT_H_
#define BOOT_H_

#include "main.h"

#define BOOT_TEMPSLOT1 0
#define BOOT_TEMPSLOT2 1

/* initialize and validates binary in specific flash address */
int boot_init(void);
void boot_goto_app(void);
int boot_validate_appslot_bin(void);
int boot_validate_tempslot_bin(uint8_t slotno);

/* read application header sections */
uint32_t boot_read_config_version(void);
uint32_t boot_read_config_size(void);
uint32_t boot_read_config_crc(void);
uint32_t boot_read_config_slotno(void);

/* erase the slot sections */
int boot_erase_tempslot(uint8_t slotno);

/* write application header sections */
int boot_write_config_version(uint32_t version);
int boot_write_config_size(uint32_t size);
int boot_write_config_crc(uint32_t crc);
int boot_write_config_slotno(uint32_t slotno);

/* write the application binary to respective temp slots */
int boot_write_bin_to_tempslot(uint8_t slotno, uint8_t *data, uint16_t size);

/* copy the application binary from the given temp slots to the application flash address */
int boot_load_bin_to_appslot(uint8_t slotno);

#endif // BOOT_H_
