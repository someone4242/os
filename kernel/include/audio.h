#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

void audio_on();
void audio_off();
void audio_set_freq(uint32_t freq);

void audio_tick();
void audio_beep(uint32_t freq, uint32_t duration);


#endif
