#include <audio.h>

#include <kernel/interrupt.h>

// #include <x86.h>
// à retirer plus tard
// -----------------------------------------
static inline void outb(int port, uint8_t data) {
  asm volatile("outb %0,%w1" : : "a"(data), "d"(port));
}
static inline uint8_t inb(int port) {
  uint8_t data;
  asm volatile("inb %w1,%0" : "=a"(data) : "d"(port));
  return data;
}
// -----------------------------------------



static uint32_t audio_countdown;


void audio_on() {
    uint8_t tmp = inb(0x61);
    if (tmp != (tmp | 3)) outb(0x61, tmp | 3);
}

void audio_off() {
    uint8_t tmp = inb(0x61) & 0xFC;
    outb(0x61, tmp);
}

void audio_set_freq(uint32_t freq) {
    uint32_t div = INTERN_FREQ / freq;

    outb(0x43, 0xB6); // Channel 2 : square wave lo/hi
    outb(0x42, (uint8_t)(div & 0xFF));
    outb(0x42, (uint8_t)(div >> 8 & 0xFF));
}


void audio_tick() {
    if (audio_countdown == 0) audio_off();
    else if (audio_countdown > 0) audio_countdown--;
}

// duration in ms
void audio_beep(uint32_t freq, uint32_t duration) {
    audio_countdown = (TICK_FREQ * duration) / 1000;
    audio_set_freq(freq);
    audio_on();
}



