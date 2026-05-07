#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <macros.h>
#include <kernel/ustar.h>

//https://wiki.osdev.org/ATA_PIO_Mode
//https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ata/ns-ata-_identify_device_data

void outl(uint16_t p_port,uint32_t p_data) {
    asm volatile ("outl %1, %0" : : "dN" (p_port), "a" (p_data));
}
uint32_t inl( uint16_t p_port) {
    uint32_t l_ret;
    asm volatile ("inl %1, %0" : "=a" (l_ret) : "dN" (p_port));
    return l_ret;
}

void checkFunction(uint8_t bus, uint8_t device) {
    printf("%d, %d\n", bus, device);
}

uint16_t getVendorID(uint8_t bus, uint8_t device, uint8_t function) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)device;
    uint32_t lfunc = (uint32_t)function;
    uint32_t offset = 0;
    uint16_t tmp = 0;
  
    // Create configuration address as per Figure 1
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));

    outl(0xCF8, address);
    // Read in the data
    // (offset & 2) * 8) = 0 will choose the first word of the 32-bit register
    tmp = (uint16_t)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xFFFF);
    return tmp;
}

void checkDevice(uint8_t bus, uint8_t device) {
    uint8_t function = 0;

    uint16_t vendorID = getVendorID(bus, device, function);

    if (vendorID == 0xFFFF) return; // Device doesn't exist
    checkFunction(bus, device);
}

void checkAllBuses(void) {
    uint16_t bus;
    uint8_t device;

    for (bus = 0; bus < 256; bus++) {
        for (device = 0; device < 32; device++) {
            checkDevice(bus, device);
        }
    }
}

void init_file_system(void) {

    /*
    int nb_blocks = 2;
    tar_record* addr = malloc(sizeof(tar_record)*nb_blocks);

    //filling with zeroes
    for (int i = 0; i < sizeof(tar_record)*nb_blocks; i++) {
        ((char*)&addr)[i] = '\0';
    }*/

    //init_fs_first_adress(addr);
    checkAllBuses();


    
}

/*
void header_creation(char* file_name, char file_type, tar_record* addr) {
    size_t name_length = strlen(file_name);

    if (name_length > FILE_NAME_MAX_SIZE + FILE_NAME_PREFIX_MAX_SIZE) {
        return;
    }
    addr->magic[0] = 'u'; addr->magic[1] = 's'; addr->magic[2] = 't';
    addr->magic[3] = 'a'; addr->magic[4] = 'r';
    addr->version[0] = '0'; addr->version[0] = '1';
    addr->typeflag = '0';

    memcpy(addr->file_name, file_name, min(FILE_NAME_MAX_SIZE, name_length));
    if (name_length > FILE_NAME_MAX_SIZE) {
        memcpy(addr->file_name_prefix, file_name + FILE_NAME_MAX_SIZE, 
               name_length - FILE_NAME_MAX_SIZE);
    }

    
}
*/