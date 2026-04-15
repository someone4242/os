#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "macros.h"
#include "kbdriver.h"


// include <x86.h>


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


static uint8_t mod; // modifiers : 0-shift, 1-caps, 2-alt, 3-ctrl

static const keycode keycode_table[KEYCODE_TABLE_LENGTH] = {
    /* 0x00 */
    KEY_UNDEFINED,
    KEY_ESC,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_0,
    KEY_DEGRE,
    KEY_PLUS,
    KEY_BACK,
    KEY_TAB,

    /* 0x10 */
    KEY_A,
    KEY_Z,
    KEY_E,
    KEY_R,
    KEY_T,
    KEY_Y,
    KEY_U,
    KEY_I,
    KEY_O,
    KEY_P,
    KEY_CIRCONF,
    KEY_DOLLAR,
    KEY_ENTER,
    KEY_CTRL,
    KEY_Q,
    KEY_S,

    /* 0x20 */
    KEY_D,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_M,
    KEY_PERCENT,
    KEY_SQUARE,
    KEY_LSHIFT,
    KEY_STAR,
    KEY_W,
    KEY_X,
    KEY_C,
    KEY_V,


    /* 0x30 */
    KEY_B,
    KEY_N,
    KEY_COMMA,
    KEY_SEMICOL,
    KEY_COL,
    KEY_EXCL,
    KEY_RSHIFT,
    KEY_UNDEFINED, // (keypad) *
    KEY_ALT,
    KEY_SPACE,
    KEY_CAPS,
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,

    /* 0x40 */
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_UNDEFINED,
    KEY_UNDEFINED,
    KEY_UNDEFINED,
    KEY_UNDEFINED,
    KEY_UNDEFINED,
    KEY_UNDEFINED,
    KEY_UNDEFINED,
    KEY_UNDEFINED,
    KEY_UNDEFINED,
    KEY_UNDEFINED,
    KEY_UNDEFINED,

    /* 0x50 */
    KEY_UNDEFINED,
    KEY_UNDEFINED,
    KEY_UNDEFINED,
    KEY_UNDEFINED,
    KEY_UNDEFINED,
    KEY_UNDEFINED,
    KEY_CHEVR,
    KEY_F11,
    KEY_F12,
};

static const struct complex_map {
    uint8_t map[4];
} complex_table[] = {
    {{'&', '1', '&', '&'}},
    {{/*'é'*/ 0xE9, '2', /*'É'*/ 0xC9, '~'}},
    {{'"', '3', '"', '#'}},
    {{'\'', '4', '\'', '{'}},
    {{'(', '5', '(', '['}},
    {{'-', '6', '-', '|'}},
    {{/*'è'*/ 0xE8, '7', /*'È'*/ 0xC8, '`'}},
    {{'_', '8', '_', '\\'}},
    {{/*'ç'*/ 0xE7, '9', /*'Ç'*/ 0xC7, '^'}},
    {{/*'à'*/ 0xE0, '0', /*'À'*/ 0xC0, '@'}},
    {{')', /*'°'*/ 0xB0, ')', ']'}},
    {{'=', '+', '=', '}'}},
    {{'^', /*'¨'*/ 0xA8, '^', '^'}},
    {{'$', /*'£'*/ 0xA3, '$', /*'¤'*/ 0xA4}},
    {{/*'ù'*/ 0xF9, '%', /*'Ù'*/ 0xD9, /*'ù'*/ 0xF9}},
    {{'*', /*'µ'*/ 0xB5, '*', '*'}},
    {{'<', '>', '<', '|'}},
    {{',', '?', ',', ','}},
    {{';', '.', ';', ';'}},
    {{':', '/', ':', ':'}},
    {{'!', /*'§'*/ 0xA7, '!', '!'}}
};



void init_kbdriver() {
    mod = 0;
}

uint8_t kb_scan_to_key(uint8_t scancode) {
    if ((scancode & 0x7F) > KEYCODE_TABLE_LENGTH)
        return KEY_UNDEFINED;

    return (scancode & 0x80) + (keycode_table[scancode & 0x7F]);
}

char kb_key_to_ascii(uint8_t input) {
    keycode kc = input & 0x7F;

    if (input & 0x80) {                        // key released
        switch (kc)
        {
            case KEY_LSHIFT:
            case KEY_RSHIFT:
                mod &= ~(1 << 0);
                break;
            case KEY_ALT:
                mod &= ~(1 << 2);
                break;
            case KEY_CTRL:
                mod &= ~(1 << 3);
                break;
           default:
                break;
        }
        return 0;
    }

    else if ( KEY_A <= kc && kc <= KEY_Z ) {     // alphabet
        return ((mod & 0x01) ? 'A' : 'a') + (kc - KEY_A);

    } else if ( kc <= KEY_SPACE ) {         // other ASCII
        switch (kc)
        {
            case KEY_TAB:   return '\t';
            case KEY_ENTER: return '\n';
            case KEY_BACK:  return '\b';
            case KEY_SPACE: return ' ';
            case KEY_SUPPR: return 0x7F;
            case KEY_SQUARE:return /*'²'*/ 0xB2;
            default:        return 0;
        }

    } else if ( kc <= KEY_EXCL ) {          // Complex keys
        int i = (mod & 0x04) ? 3 : (mod & 0x01) ? 1 : (mod & 0x02) ? 2 : 0;
        return complex_table[kc - KEY_1].map[i];

    } else if ( kc <= KEY_DOWN ) {          // non ASCII keys
        return 0;

    } else {                                // modifier keys
        switch (kc)
        {
            case KEY_CAPS:
                mod ^= 1 << 1;
                break;
            case KEY_LSHIFT:
            case KEY_RSHIFT:
                mod |= 1 << 0;
                break;
            case KEY_ALT:
                mod |= 1 << 2;
                break;
            case KEY_CTRL:
                mod |= 1 << 3;
                break;
            default:
                break;
        }
        return 0;
    }
}


uint8_t kb_scan() {
    return kb_scan_to_key(inb(0x60));
}

char kb_readc() {
    return kb_key_to_ascii(kb_scan());
}


