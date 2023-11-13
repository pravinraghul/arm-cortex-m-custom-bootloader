#include "boot.h"
#include "partition.h"
#include "SEGGER_RTT.h"

#include "crc32.h"

typedef void (*func_ptr_t) (void);

/* Number of the bootloader slots available */
static uint32_t boot_slots_addr[] = {
SLOT1_FLASH_ADDR,
SLOT2_FLASH_ADDR
};

/* Tracks the application bytes written into slot address */
static uint32_t boot_recv_inc_global = 0;

static bool boot_validate_config_flash_address(void)
{
    if ((CONFIG_FLASH_ADDR >= FLASH_BASE) &&
        (CONFIG_FLASH_ADDR + CONFIG_FLASH_SIZE <= FLASH_END))
        return true;

    return false;
}

static bool boot_validate_appslot_flash_address(void)
{
   if ((APP_FLASH_ADDR >= FLASH_BASE) &&
       (APP_FLASH_ADDR + APP_FLASH_SIZE <= FLASH_END))
        return true;

   return false;
}

static bool boot_validate_tempslot1_flash_address(void)
{
   if ((SLOT1_FLASH_ADDR >= FLASH_BASE) &&
       (SLOT1_FLASH_ADDR + SLOT1_FLASH_SIZE <= FLASH_END))
        return true;

   return false;
}

static bool boot_validate_tempslot2_flash_address(void)
{
   if ((SLOT2_FLASH_ADDR >= FLASH_BASE) &&
       (SLOT2_FLASH_ADDR + SLOT2_FLASH_SIZE <= FLASH_END))
        return true;

   return false;
}

int boot_init(void)
{
    if (!boot_validate_config_flash_address()     ||
        !boot_validate_appslot_flash_address()    ||
        !boot_validate_tempslot1_flash_address()  ||
        !boot_validate_tempslot2_flash_address()) {
        SEGGER_RTT_printf(0, "boot_init: failed \r\n");
        return -1;
    }

    return 0;
}

int boot_validate_appslot_bin(void)
{
    uint32_t app_addr = APP_FLASH_ADDR;
    uint32_t app_size = boot_read_config_size();
    uint32_t app_crc = boot_read_config_crc();
    uint32_t crc = 0;

    if (app_size == 0 || app_size > APP_FLASH_SIZE) {
        SEGGER_RTT_printf(0, "boot_validate_appslot_bin: size = %ld\r\n", app_size);
        return -1;
    }

    // Calculate the CRC
    crc = crc32_calculate_from_flash(app_addr, app_size);
    if (crc != app_crc) {
        SEGGER_RTT_printf(0, "boot_validate_appslot_bin: crc = %x, calc_crc = %x\r\n", app_crc, crc);
        return -1;
    }

    return 0;
}

int boot_validate_tempslot_bin(uint8_t slotno)
{
    uint32_t slot_addr = boot_slots_addr[slotno];
    uint32_t slot_size = boot_read_config_size();
    uint32_t slot_crc = boot_read_config_crc();
    uint32_t crc;

    if (slot_size == 0 || slot_size > SLOT1_FLASH_SIZE) {
        SEGGER_RTT_printf(0, "boot_validate_tempslot_bin: size = %ld\r\n", slot_size);
        return -1;
    }

    // Calculate the CRC
    crc = crc32_calculate_from_flash(slot_addr, slot_size);
    if (crc != slot_crc) {
        SEGGER_RTT_printf(0, "boot_validate_tempslot_bin: crc = %ld, calc_crc = %ld\r\n", slot_crc, crc);
        return -1;
    }

    return 0;
}

void boot_goto_app(void)
{
    func_ptr_t reset_handler;

    SEGGER_RTT_printf(0, "boot_goto_app: Jumping to appslot...! \r\n");

    reset_handler = (void *) *((volatile uint32_t *) (APP_FLASH_ADDR + 4));
    __set_MSP(*((volatile uint32_t *)APP_FLASH_ADDR));
    reset_handler();
}

uint32_t boot_read_config_version(void)
{
    return (uint32_t) *((volatile uint32_t *) CONFIG_FLASH_ADDR);
}

int boot_write_config_version(uint32_t version)
{
    HAL_StatusTypeDef ret;

    HAL_FLASH_Unlock();
    ret = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
                            CONFIG_FLASH_ADDR,
                            version);
    HAL_FLASH_Lock();

    if (ret != HAL_OK) {
        SEGGER_RTT_printf(0, "boot_write_config_version: flash failed \r\n");
        return -1;
    }
    return 0;
}

uint32_t boot_read_config_size(void)
{
    return (uint32_t) *((volatile uint32_t *) (CONFIG_FLASH_ADDR + 4));
}

int boot_write_config_size(uint32_t size)
{
    HAL_StatusTypeDef ret;

    HAL_FLASH_Unlock();
    ret = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
                            CONFIG_FLASH_ADDR + 4,
                            size);
    HAL_FLASH_Lock();

    if (ret != HAL_OK) {
        SEGGER_RTT_printf(0, "boot_write_config_size: flash failed \r\n");
        return -1;
    }
    return 0;
}

uint32_t boot_read_config_crc(void)
{
    return (uint32_t) *((volatile uint32_t *) (CONFIG_FLASH_ADDR + 8));
}

int boot_write_config_crc(uint32_t crc)
{
    HAL_StatusTypeDef ret;

    HAL_FLASH_Unlock();
    ret = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
                            CONFIG_FLASH_ADDR + 8,
                            crc);
    HAL_FLASH_Lock();

    if (ret != HAL_OK) {
        SEGGER_RTT_printf(0, "boot_write_config_crc: flash failed \r\n");
        return -1;
    }
    return 0;
}

uint32_t boot_read_config_slotno(void)
{
    return (uint32_t) *((volatile uint32_t *) (CONFIG_FLASH_ADDR + 12));
}

int boot_write_config_slotno(uint32_t slotno)
{
    HAL_StatusTypeDef ret;

    HAL_FLASH_Unlock();
    ret = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
                            CONFIG_FLASH_ADDR + 12,
                            slotno + 1);
    HAL_FLASH_Lock();

    if (ret != HAL_OK) {
        SEGGER_RTT_printf(0, "boot_write_config_slotno: flash failed \r\n");
        return -1;
    }
    return 0;
}

static int boot_erase(uint32_t sectors, uint8_t no_of_sectors)
{
    FLASH_EraseInitTypeDef erase_struct = {0};
    uint32_t erase_status;
    uint16_t ret;

    erase_struct.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase_struct.Sector = sectors;
    erase_struct.NbSectors = no_of_sectors;
    erase_struct.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    HAL_FLASH_Unlock();

    ret = HAL_FLASHEx_Erase(&erase_struct, &erase_status);

    HAL_FLASH_Lock();

    if (ret != HAL_OK) {
        SEGGER_RTT_printf(0, "boot_erase : erase failed \r\n");
        return -1;
    }
    return 0;
}

int boot_erase_tempslot(uint8_t slotno)
{
    uint32_t sector = 0;

    boot_recv_inc_global = 0;

    if (slotno != BOOT_TEMPSLOT1 && slotno != BOOT_TEMPSLOT2) {
        SEGGER_RTT_printf(0, "boot_erase_tempslot : wrong slotno \r\n");
        return -1;
    }

    if (slotno == BOOT_TEMPSLOT1)
        sector = FLASH_SECTOR_17;
    else if (slotno == BOOT_TEMPSLOT2)
        sector = FLASH_SECTOR_19;

    return boot_erase(sector, 2);
}

int boot_write_bin_to_tempslot(uint8_t slotno, uint8_t *data, uint16_t size)
{
    HAL_StatusTypeDef ret;
    uint32_t slot_addr = boot_slots_addr[slotno];

    HAL_FLASH_Unlock();

    for (int i = 0; i < size; i++) {

        if (boot_recv_inc_global > SLOT1_FLASH_SIZE) {
            SEGGER_RTT_printf(0, "boot_write_bin_to_tempslot : beyond the flash size\r\n");
            return -1;
        }

        ret = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE,
                                slot_addr + boot_recv_inc_global,
                                data[i]);
        if (ret != HAL_OK) {
            HAL_FLASH_Lock();
            SEGGER_RTT_printf(0, "boot_write_bin_to_tempslot : flash failed\r\n");
            return -1;
        }
        boot_recv_inc_global += 1;
    }

    HAL_FLASH_Lock();
    return 0;
}

int boot_load_bin_to_appslot(uint8_t slotno)
{
    HAL_StatusTypeDef ret;
    uint32_t app_addr = APP_FLASH_ADDR;
    uint32_t slot_addr = boot_slots_addr[slotno];
    uint32_t size = boot_read_config_size();

    if (boot_erase(FLASH_SECTOR_5, 2) == -1) {
        SEGGER_RTT_printf(0, "boot_load_bin_to_appslot : erase failed \r\n");
        return -1;
    }

    HAL_FLASH_Unlock();

    for (int i = 0; i < size; i++) {
        ret = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE,
                                app_addr + i,
                                *((volatile uint32_t *)(slot_addr+i)));
        if (ret != HAL_OK) {
            HAL_FLASH_Lock();
            SEGGER_RTT_printf(0, "boot_load_bin_to_appslot : flash failed \r\n");
            return -1;
        }
    }

    HAL_FLASH_Lock();

    return 0;
}
