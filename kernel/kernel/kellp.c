/* Kernel Shell Prototype - C */

#include <kellp.h>

#include <string.h>
#include <kernel/tty.h>
#include <kernel/allocator.h>
#include <audio.h>

#define TAB_WIDTH 80
#define TAB_HEIGHT 25
#define TAB_SIZE TAB_WIDTH * TAB_HEIGHT
static char tab[TAB_SIZE];
static size_t cursor;
static size_t cmd_start;

#define STATE_NQDMJW 0x01
static uint8_t state; // 0:NQDM JW


#define NULL_PARSED_CMD (parsed_cmd){ 0, NULL }
#define NULL_CMD        (cmd){ NULL, NULL }

typedef struct {
    int count;
    char **tbl;
} parsed_cmd;

typedef struct {
    char *name;
    void (*f)(parsed_cmd);
} builtin_cmd;

#define NB_BUILTINS 4
static builtin_cmd builtins[NB_BUILTINS] = {};

/* Builtins */
void builtin_help(parsed_cmd cmd);

void builtin_echo(parsed_cmd cmd);

void builtin_beep(parsed_cmd cmd);

void builtin_nqdmjw(parsed_cmd cmd);
void nqdmjw_feedinp(input_t inp);


#define NB_NOTE_KEYS 17
typedef uint8_t note;
uint8_t nqdmjw_state;
unsigned char visual_note = 219;



/* Utilitaries */

static void tab_scroll(const size_t amount) {
    size_t i;
    for (i = 0; i < (TAB_HEIGHT - amount) * TAB_WIDTH; i++) {
        const uint16_t c = tab[amount * TAB_WIDTH + i];
        tab[i] = c;
    }
    for (i = (TAB_HEIGHT-amount)*TAB_WIDTH; i < TAB_HEIGHT * TAB_WIDTH; i++) {
        tab[i] = ' ';
    }
}

static void scroll(const size_t amount) {
    if (cursor < amount * TAB_WIDTH || cmd_start < amount * TAB_WIDTH) {
        init_kellp();
    }
    cursor -= amount * TAB_WIDTH;
    cmd_start -= amount * TAB_WIDTH;
    tab_scroll(amount);
}

static void cursor_drop_left() {
    while (cmd_start < cursor && tab[cursor - 1] == 0x00) cursor--;
}

static void cursor_drop_down() {
    cursor = TAB_WIDTH * (cursor / TAB_WIDTH + 1);
    if (cursor >= TAB_SIZE) scroll(1);
}


/* Inspired from TTY */

void kellp_putchar(char ch) {
    tab[cursor] = ch;
    if (cursor++ == TAB_SIZE) {
        scroll(1);
    }
}

void kellp_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++)
        kellp_putchar(data[i]);
}

void kellp_writestring(const char* data) {
    kellp_write(data, strlen(data));
}


/* Parsing (need for a dedicated standard library later ?) */

static size_t count_args(char *cmd_line, size_t len) {
    uint8_t state = 0x00; // 0:sniffing
    int c = 0;

    for (size_t i = 0; i < len; i++) {
        switch (cmd_line[i])
        {
            case ' ':
                state |= 0x01; // start sniffing
                break;
            default:
                if (state & 0x01) {
                    c++;
                    state &= ~0x01; // stop sniffing
                }
                break;
        }
    }

    return c;
}

// compte le nombre de chars jusqu'à tomber sur `ch` (exclus)
static size_t count_until(char *str, size_t len, char ch) {
    for (size_t i = 0; i < len; i++) {
        if (str[i] == ch) return i;
    }
    return len;
}
static void write_arg(char **destptr, char *src, size_t len) {
    char *arg = (char *)kmalloc((1 + len) * sizeof(char));
    if (arg == NULL) { *destptr = NULL; return; }
    memcpy(arg, src, len);
    arg[len] = '\0';
    *destptr = arg;
}

// Parseur de ligne de commange
// A ajouter : l'échappement des chars
static parsed_cmd parse_command(char *cmd_line, size_t len) {
    size_t arg_count = count_args(cmd_line, len);

    char **tbl = (char **)kmalloc(arg_count * sizeof(void *));
    if (tbl == NULL) { return NULL_PARSED_CMD; }

    uint8_t state = 0x00; // 0:sniffing 1:reading 2:in-quotes
    if (len > 0 && cmd_line[0] != ' ') state |= 0x02;

    int c = 0;
    size_t word_start = 0;

    for (size_t i = 0; i < len; i++) {
        switch (cmd_line[i])
        {
            case ' ':
                if (state & 0x04) break;
                if (state & 0x02) {
                    write_arg(tbl + c, cmd_line + word_start, i - word_start);
                    if (tbl[c] == NULL) { return NULL_PARSED_CMD; }
                    c++;
                    state &= ~0x02; // stop reading
                }
                state |= 0x01; // start sniffing
                break;

            case '"':
                if ((state & 0x04) == 0) {
                    if (i == len - 1) { return NULL_PARSED_CMD; }
                    word_start = i + 1;
                    state = 0x04; // in quotes
                } else {
                    write_arg(tbl + c, cmd_line + word_start, i - word_start);
                    if (tbl[c] == NULL) { return NULL_PARSED_CMD; }
                    c++;
                    state = 0x01; // start sniffing
                }
                break;

            default:
                if (state & 0x01) {
                    word_start = i;
                    state |= 0x02; //start reading
                    state &= ~0x01; // stop sniffing
                }
                break;
        }
    }

    if (state & 0x04) { return NULL_PARSED_CMD; }

    if (state & 0x02) {
        write_arg(tbl + c, cmd_line + word_start, len - word_start);
        if (tbl[c] == NULL) { return NULL_PARSED_CMD; }
        c++;
    }

    return (parsed_cmd){ c, tbl };
}


// When useless, just print the parsed command
static void print_parsed(parsed_cmd cmd) {
    for (int i = 0; i < cmd.count; i++) {
        cursor_drop_down();
        kellp_writestring(cmd.tbl[i]);
    }
}



/* Command execution */
static void execute_parsed_cmd(parsed_cmd cmd) {
    // look for builtin
    size_t namelen = strlen(cmd.tbl[0]);
    for (int i = 0; i < NB_BUILTINS; i++) {
        if (builtins[i].name != NULL &&
                memcmp(cmd.tbl[0], builtins[i].name, namelen) == 0) {
            (*builtins[i].f)(cmd); return;
        }
    }

    cursor_drop_down();
    kellp_writestring("Command does not exist, here it is parsed :");
    print_parsed(cmd);
}



/* Interface */

void init_kellp() {
    // Builtins
    builtins[0] = (builtin_cmd){ "help", &builtin_help };
    builtins[1] = (builtin_cmd){ "echo", &builtin_echo };
    builtins[2] = (builtin_cmd){ "beep", &builtin_beep };
    builtins[3] = (builtin_cmd){ "nqdmjw", &builtin_nqdmjw };


    for (size_t i = 0; i < TAB_SIZE; i++) {
        tab[i] = 0x00;
    }

    tab[0] = '>'; tab[1] = '>'; tab [2] = ' ';
    cmd_start = 3;
    cursor = 3;

    terminal_update(tab, cursor);
}

void kellp_feedinp(input_t inp) {
    parsed_cmd cmd;

    if (state & STATE_NQDMJW) {
        nqdmjw_feedinp(inp);
        return;
    }

    switch (inp.kc)
    {
        case KEY_ENTER:
            cmd = parse_command(tab + cmd_start, cursor - cmd_start);
            execute_parsed_cmd(cmd);
            if (state & STATE_NQDMJW) break;
            cursor_drop_down();
            kellp_writestring(">> ");
            cmd_start = cursor;
            break;

        case KEY_BACK:
            if (cursor == cmd_start) break;
            cursor--;
            tab[cursor] = 0x00;
            cursor_drop_left();
            break;

        default:
            if (inp.ch != 0) kellp_putchar(inp.ch);
            break;
    }

    terminal_update(tab, cursor);
}


//temp
uint32_t str_to_uint32(char *s) {
    size_t i = 0;
    uint32_t k = 0;
    while (s[i]) {
        if ('0' <= s[i] && s[i] <= '9') {
            k *= 10;
            k += (uint32_t)(s[i] - '0');
        }
        i++;
    }
    return k;
}


/* Buitlins */
void builtin_help(parsed_cmd cmd) {
    (void)(cmd);
    cursor_drop_down();
    for (int i = 0; i < NB_BUILTINS; i++) {
        kellp_writestring(builtins[i].name);
        kellp_putchar(' ');
    }
}

void builtin_echo(parsed_cmd cmd) {
    cursor_drop_down();
    for (int i = 1; i < cmd.count; i++) {
        kellp_writestring(cmd.tbl[i]);
        kellp_putchar(' ');
    }
}

void builtin_beep(parsed_cmd cmd) {
    uint32_t freq = 440;
    uint32_t duration = 3000;
    if (cmd.count >= 2) freq = str_to_uint32(cmd.tbl[1]);
    if (cmd.count >= 3) duration = str_to_uint32(cmd.tbl[2]);
    if (cmd.count >= 4) {
        cursor_drop_down();
        kellp_writestring("Too many arguments, expected 2 : freq, duration");
        return;
    }
    audio_beep(freq, duration);
}

void builtin_nqdmjw(parsed_cmd cmd) {
    if (cmd.count >= 2) {
        cursor_drop_down();
        kellp_writestring("No arguments expected");
        return;
    }

    cursor_drop_down();
    if (cursor / TAB_WIDTH + 2 >= TAB_HEIGHT) scroll(3);
    kellp_writestring(
"=============================== NQDM'S JAZZWORLD ==============================="
    );
    kellp_writestring(
"================================= TIME TO JAZZ ================================="
    );
    tab[13 + cursor] = 200;
    tab[13 + cursor + (NB_NOTE_KEYS * 3) + 1] = 188;
    state |= STATE_NQDMJW;

    nqdmjw_state = 0;
}



/* NQDMJW */
uint32_t note_freq[] = { 0,
    26163, 27718, 29366, 31113, 32961, 34923, 36999, 39200, 41530, 44000,
    46616, 49388, 52325, 55437, 58733, 62225, 65926
};

note kc_to_note(keycode kc) {
    switch(kc)
    {
        case KEY_Q: return 1;
        case KEY_Z: return 2;
        case KEY_S: return 3;
        case KEY_E: return 4;
        case KEY_D: return 5;
        case KEY_F: return 6;
        case KEY_T: return 7;
        case KEY_G: return 8;
        case KEY_Y: return 9;
        case KEY_H: return 10;
        case KEY_U: return 11;
        case KEY_J: return 12;
        case KEY_K: return 13;
        case KEY_O: return 14;
        case KEY_L: return 15;
        case KEY_P: return 16;
        case KEY_M: return 17;
        default:    return 0;
    }
}

void nqdmjw_feedinp(input_t inp) {
    if (inp.kc == KEY_ESC) {
        audio_off();
        state &= ~STATE_NQDMJW;
        cursor_drop_down();
        kellp_writestring(">> ");
        cmd_start = cursor;
        return;
    }

    if (inp.kc == KEY_SPACE) {
        nqdmjw_state |= 0x80;
        return;
    } else if (inp.kc == 0x80 + KEY_SPACE) {
        nqdmjw_state &= ~0x80;
    }

    note n = kc_to_note(inp.kc & 0x7F);
    if (n == 0) return;

    if (inp.kc & 0x80) {
        tab[13 + cursor + ((n-1) * 3) + 1] = ' ';
        tab[13 + cursor + ((n-1) * 3) + 2] = ' ';
        tab[13 + cursor + ((n-1) * 3) + 3] = ' ';
        if (n == (nqdmjw_state & 0x1F)) {
            nqdmjw_state &= ~0x1F;
            audio_off();
        }
    } else {
        nqdmjw_state = (nqdmjw_state & ~0x1F) + n;
        uint32_t freq = note_freq[n] / ((nqdmjw_state & 0x80) ? 2 : 1);
        audio_set_freq_prec(freq);
        audio_on();
        tab[13 + cursor + ((n-1) * 3) + 1] = -78;
        tab[13 + cursor + ((n-1) * 3) + 2] = -78;
        tab[13 + cursor + ((n-1) * 3) + 3] = -78;
    }

    terminal_update(tab, cursor + TAB_WIDTH);
}



