#include <stddef.h>
#include <stdint.h>

typedef enum {
    KEY_UNDEFINED,

    /* alphabet */
    KEY_A,
    KEY_B,
    KEY_C,
    KEY_D,
    KEY_E,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_I,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_M,
    KEY_N,
    KEY_O,
    KEY_P,
    KEY_Q,
    KEY_R,
    KEY_S,
    KEY_T,
    KEY_U,
    KEY_V,
    KEY_W,
    KEY_X,
    KEY_Y,
    KEY_Z,

    /* other ASCII keys */
    KEY_TAB,
    KEY_ENTER,
    KEY_BACK,
    KEY_SPACE,
    KEY_SUPPR,
    KEY_SQUARE,

    /* Complex keys */
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
    KEY_CIRCONF,
    KEY_DOLLAR,
    KEY_PERCENT,
    KEY_STAR,
    KEY_CHEVR,
    KEY_COMMA,
    KEY_SEMICOL,
    KEY_COL,
    KEY_EXCL,

    /* non ASCII keys */
    KEY_ESC,
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,
    KEY_INSER,
    KEY_IMPEC,
    KEY_UP,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_DOWN,

    /* modifier keys */
    KEY_CAPS,
    KEY_LSHIFT,
    KEY_RSHIFT,
    KEY_CTRL,
    KEY_FN,
    KEY_MAIN,
    KEY_ALT
} keycode;

typedef struct {
    uint8_t mod;
    keycode kc;
} input_t;


void init_kbdriver();
uint8_t kb_scan_to_key(uint8_t scancode);
char kb_key_to_ascii(uint8_t input);
uint8_t kb_scan();
char kb_readc();