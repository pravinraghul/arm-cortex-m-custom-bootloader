#ifndef PARTITION_H_
#define PARTITION_H_


#define CONFIG_FLASH_ADDR (0x08010000) // sector 4
#define CONFIG_FLASH_SIZE (16)

#define APP_FLASH_ADDR (0x08020000) // sector 5, 6
#define APP_FLASH_SIZE (256 * 1024)

#define SLOT1_FLASH_ADDR (0x08120000) // sector 17, 18
#define SLOT1_FLASH_SIZE (256 * 1024)

#define SLOT2_FLASH_ADDR (0x08160000) // sector 19, 20
#define SLOT2_FLASH_SIZE (256 * 1024)

#endif // PARTITION_H_
