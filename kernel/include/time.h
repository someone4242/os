#ifndef TIME_H
#define TIME_H

#include <stdint.h>

#define INTERN_FREQ 1193182 // Hz
#define TICK_FREQ 100       // Hz

typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint8_t year;
    uint8_t century;
} human_time;

void init_time();
int time_tick();
human_time get_human_time();


#endif
