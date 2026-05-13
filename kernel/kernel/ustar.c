#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <macros.h>
#include <kernel/ustar.h>

void outl(uint16_t p_port,uint32_t p_data) {
    asm volatile ("outl %1, %0" : : "dN" (p_port), "a" (p_data));
}
uint32_t inl( uint16_t p_port) {
    uint32_t l_ret;
    asm volatile ("inl %1, %0" : "=a" (l_ret) : "dN" (p_port));
    return l_ret;
}

void outb(int port, uint8_t data) {
  asm volatile("outb %0,%w1" : : "a"(data), "d"(port));
}
uint8_t inb(int port) {
  uint8_t data;
  asm volatile("inb %w1,%0" : "=a"(data) : "d"(port));
  return data;
}

uint16_t inw(uint16_t p_port)
{
    uint16_t l_ret;
    asm volatile ("inw %1, %0" : "=a" (l_ret) : "dN" (p_port));
    return l_ret;
}

void outw (uint16_t p_port,uint16_t p_data)
{
    asm volatile ("outw %1, %0" : : "dN" (p_port), "a" (p_data));
}

//jsp quoi faire de ça ------------------------------------
tar_record* tar_fs_start_address;

void init_fs_first_adress(tar_record* addr) {
    tar_fs_start_address = addr;
}

size_t octascii_to_dec(char *number, int size) {
    size_t res = 0;
    for (int i = 0; i < size; i++) {
        res = res*8 + (number[i] - '0');
    }
    return res;
}


bool is_zeroed(tar_record* current_record) {
    char* parcours = (char*)current_record;
    for (uint i = 0; i < sizeof(tar_record); i++) {
        if (parcours[i] != '\0') {
            return false;
        }
    }
    return true;
}
// --------------------------------------------------

void wait_for_ready_hard_drive(void) {
    uint8_t read_status;

    while (67) {
        read_status = inb(0X1F7);
        
        // Vérifier BSY (busy) (bit 7)
        if (read_status & 0x80) {
            continue;
        }
        
        // Vérifier DRDY (bit 6)
        if (read_status & 0x40) {
            break;
        }
        
        // pause
        for (int i = 0; i < 100; i++) {
            asm volatile("nop");
        }
    }

}

void wait_for_data_request(void) {
    uint8_t read_status;

    while (67) {
        read_status = inb(0X1F7);
        
        // Vérifier DRQ (bit 3)
        if (read_status & 0x08) {
            break;
        }
        
        // pause
        for (int i = 0; i < 100; i++) {
            asm volatile("nop");
        }
    }

}


void read_sector_pio(uint32_t lba, uint8_t* buffer) {
    
    wait_for_ready_hard_drive();
    
    // Registres de lecture
    outb(ATA_PRIMARY_BASE + 2, 1);
    outb(ATA_PRIMARY_BASE + 3, lba & 0xFF);         // LBA Low
    outb(ATA_PRIMARY_BASE + 4, (lba >> 8) & 0xFF);  // LBA Mid
    outb(ATA_PRIMARY_BASE + 5, (lba >> 16) & 0xFF); // LBA High
    outb(ATA_PRIMARY_BASE + 6, ATA_MASTER | ((lba >> 24) & 0x0F));
    
    //commande READ SECTORS
    outb(ATA_PRIMARY_BASE + 7, 0x20);
    wait_for_data_request();
    
    // 5. Lire les 256 mots (512 octets) du secteur
    for (int i = 0; i < 256; i++) {
        uint16_t word = inw(ATA_PRIMARY_BASE);
        buffer[i * 2] = word & 0xFF;            
        buffer[i * 2 + 1] = (word >> 8) & 0xFF; 
    }
    
    return;
}