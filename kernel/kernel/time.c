#include <time.h>

// #include <x86.h>
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


#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71
#define BCD_TO_BIN(bcd) ((bcd & 0xF0) >> 1) + ((bcd & 0xF0) >> 3) + (bcd & 0x0F)


human_time htime;
uint64_t ticks;
uint64_t second_ticks;


// Code adapted from OSDev CMOS page
static int get_update_in_progress_flag() {
    outb(CMOS_ADDR, 0x0A);
    return (inb(CMOS_DATA) & 0x80);
}
static uint8_t get_rtc_reg(uint8_t reg) {
    outb(CMOS_ADDR, reg);
    return inb(CMOS_DATA);
}
static void poll_rtc(human_time* ht) {
    while (get_update_in_progress_flag()); // Wait out updates
    ht->second   = get_rtc_reg(0x00);
    ht->minute   = get_rtc_reg(0x02);
    ht->hour     = get_rtc_reg(0x04);
    ht->day      = get_rtc_reg(0x07);
    ht->month    = get_rtc_reg(0x08);
    ht->year     = get_rtc_reg(0x09);
    ht->century  = get_rtc_reg(0x32);
}

static human_time read_rtc() {
    human_time ht;
    human_time prev_ht;
    uint8_t registerB;

    poll_rtc(&ht);
    do {
        prev_ht = ht;
        poll_rtc(&ht);
    } while (
            (ht.second  != prev_ht.second )
        ||  (ht.minute  != prev_ht.minute )
        ||  (ht.hour    != prev_ht.hour   )
        ||  (ht.day     != prev_ht.day    )
        ||  (ht.month   != prev_ht.month  )
        ||  (ht.year    != prev_ht.year   )
        ||  (ht.century != prev_ht.century)
    );

    registerB = get_rtc_reg(0x0B);

    // Convertions
    if (!(registerB & 0x04)) {
        ht.second  = BCD_TO_BIN(ht.second );
        ht.minute  = BCD_TO_BIN(ht.minute );
        ht.hour    = (BCD_TO_BIN(ht.hour & 0x7F)) | (ht.hour & 0x80);
        ht.day     = BCD_TO_BIN(ht.day    );
        ht.month   = BCD_TO_BIN(ht.month  );
        ht.year    = BCD_TO_BIN(ht.year   );
        ht.century = BCD_TO_BIN(ht.century);
    }
    if (!(registerB & 0x02) && (ht.hour & 0x80)) {
        ht.hour = ((ht.hour & 0x7F) + 12) % 24;
    }

    return ht;
}

static void human_time_incr(human_time* ht) {
    if (++ht->second < 60) return;
    ht->second = 0;
    if (++ht->minute < 60) return;
    ht->minute = 0;
    if (++ht->hour < 24) return;
    ht->hour = 0;
    if (++ht->day < 31) return;
    ht->day = 0;
    if (++ht->month < 12) return;
    ht->month = 0;
    if (++ht->year < 100) return;
    ht->year = 0;
    ht->century++;
}

void init_time() {
    htime = read_rtc();
    ticks = 0;
}

int time_tick() {
    ticks++;
    if (++second_ticks >= TICK_FREQ) {
        second_ticks = 0;
        human_time_incr(&htime);
        return 1;
    }
    return 0;
}

human_time get_human_time() { return htime; }


