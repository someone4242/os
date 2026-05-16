#ifndef _USTAR_H 
#define _USTAR_H 

#include <macros.h>
#include <stdbool.h>

#define FILE_NAME_MAX_SIZE 100
#define FILE_MODE_SIZE 8
#define USER_ID_SIZE 8
#define FILE_SIZE_SIZE 12
#define LAST_MODIFICATION_DATE_SIZE 12
#define CHECKSUM_SIZE 8
#define USERNAME_MAX_SIZE 32
#define DEVICE_MAJMIN_NUMBER_SIZE 8
#define FILE_NAME_PREFIX_MAX_SIZE 167
#define MAGIC_USTAR_NUMBER_SIZE 6
#define USTAR_VERSION_SIZE 2
#define ATA_PRIMARY_BASE 0x1F0
#define ATA_MASTER 0xA0

#define IS_A_REGULAR_FILE '0'
#define IS_A_DIRECTORY '5'

typedef struct {
    char file_name[FILE_NAME_MAX_SIZE];
    char file_mode[FILE_MODE_SIZE];
    char owner_id[USER_ID_SIZE];
    char group_id[USER_ID_SIZE];
    char file_size[FILE_SIZE_SIZE];
    char last_modification[LAST_MODIFICATION_DATE_SIZE];
    char checksum[CHECKSUM_SIZE];
    char typeflag;
    char linked_file_name[FILE_NAME_MAX_SIZE];
    char magic[MAGIC_USTAR_NUMBER_SIZE];
    char version[USTAR_VERSION_SIZE];
    char owner_name[USERNAME_MAX_SIZE];
    char group_name[USERNAME_MAX_SIZE];
    char device_major[DEVICE_MAJMIN_NUMBER_SIZE];
    char device_minor[DEVICE_MAJMIN_NUMBER_SIZE];
    char file_name_prefix[FILE_NAME_PREFIX_MAX_SIZE];
} tar_record;

void outb(int port, uint8_t data);
uint8_t inb(int port);

void outw (uint16_t p_port,uint16_t p_data);
uint16_t inw(uint16_t p_port);

void outl(uint16_t p_port,uint32_t p_data);
uint32_t inl( uint16_t p_port);

void wait_for_ready_hard_drive(void);
void read_sector_pio(uint32_t lba, uint8_t* buffer);

bool is_zeroed(tar_record* current_record);
size_t octascii_to_dec(char *number, int size);

#endif