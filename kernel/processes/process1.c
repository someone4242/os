#include <stdint.h>
#include <stdbool.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdtab.h>
#include <string.h>

#define NB_BUILTINS 5
static cmd_t builtins[NB_BUILTINS] = {};

#define STATE_NQDMJW 0x01
static uint8_t state;

#define NB_NOTE_KEYS 17
typedef uint8_t note;
uint8_t nqdmjw_state;
unsigned char visual_note = 219;


void init_kellp();
void kellp_feedinp(/*input_t inp*/);
void print_parsed(parsed_cmd_t pcmd);
void execute_parsed_cmd(parsed_cmd_t pcmd);

void builtin_help(parsed_cmd_t cmd);
void builtin_echo(parsed_cmd_t cmd);
void builtin_beep(parsed_cmd_t cmd);
void builtin_nqdmjw(parsed_cmd_t cmd);
void nqdmjw_feedinp(/*input_t inp*/);
void builtin_date(parsed_cmd_t cmd);


static char tab_data[DISPLAY_SIZE];
static tab_t tab_struct;
static tab_t *tab;



int main() {
    // Init Tab
    tab = &tab_struct;
    tab->width = DISPLAY_WIDTH;
    tab->height = DISPLAY_HEIGHT;
    tab->size = DISPLAY_SIZE;
    tab->cursor = 0;
    tab->cmd_start = 0;
    tab->is_terminal = true;
    tab->data = tab_data;

    for (size_t i = 0; i < DISPLAY_SIZE; i++) tab->data[i] = 0x00;

    tab_load(tab);
    while(1);
    return 0;
}

void init_kellp() {
    // Load Buitlins
    builtins[1] = (cmd_t){ "echo", &builtin_echo };
    builtins[2] = (cmd_t){ "beep", &builtin_beep };
    builtins[3] = (cmd_t){ "nqdmjw", &builtin_nqdmjw };
    builtins[4] = (cmd_t){ "date", &builtin_date };


    // Initial state
    tab_writestring(tab, ">> ");
    tab->cmd_start = 3;

    tab_load(tab);
}

void kellp_feedinp(/*input_t inp*/) {
    /*
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

    tab_load(tab);
    */
}

void print_parsed(parsed_cmd_t pcmd) {
    for (int i = 0; i < pcmd.count; i++) {
        tab_cursor_drop_down(tab);
        tab_writestring(tab, pcmd.tbl[i]);
    }
}

void execute_parsed_cmd(parsed_cmd_t pcmd) {
    // look for builtin
    size_t namelen = strlen(pcmd.tbl[0]);
    for (int i = 0; i < NB_BUILTINS; i++) {
        if (builtins[i].name != NULL &&
            memcmp(pcmd.tbl[0], builtins[i].name, namelen) == 0) {
            (*builtins[i].f)(pcmd);
            return;
        }
    }

    tab_cursor_drop_down(tab);
    tab_writestring(tab, "Command does not exist, here it is parsed :");
    print_parsed(pcmd);
}


/* Buitlins */
void builtin_help(parsed_cmd_t cmd) {
    (void)(cmd);
    tab_cursor_drop_down(tab);
    for (int i = 0; i < NB_BUILTINS; i++) {
        tab_writestring(tab, builtins[i].name);
        tab_putchar(tab, ' ');
    }
}

void builtin_echo(parsed_cmd_t cmd) {
    tab_cursor_drop_down(tab);
    for (int i = 1; i < cmd.count; i++) {
        tab_writestring(tab, cmd.tbl[i]);
        tab_putchar(tab, ' ');
    }
}

void builtin_beep(parsed_cmd_t cmd) {
    uint32_t freq = 440;
    uint32_t duration = 3000;
    if (cmd.count >= 2) freq = str_to_uint32(cmd.tbl[1]);
    if (cmd.count >= 3) duration = str_to_uint32(cmd.tbl[2]);
    if (cmd.count >= 4) {
        tab_cursor_drop_down(tab);
        tab_writestring(tab, "Too many arguments, expected 2 : freq, duration");
        return;
    }
    // audio_beep(freq, duration);
}

void builtin_nqdmjw(parsed_cmd_t cmd) {
    if (cmd.count >= 2) {
        tab_cursor_drop_down(tab);
        tab_writestring(tab, "No arguments expected");
        return;
    }

    tab_cursor_drop_down(tab);
    if (tab->cursor / tab->width + 2 >= tab->height) tab_scroll(tab, 3);
    tab_writestring(tab,
"=============================== NQDM'S JAZZWORLD ==============================="
    );
    tab_writestring(tab,
"================================= TIME TO JAZZ ================================="
    );
    tab->data[13 + tab->cursor] = -56;
    tab->data[13 + tab->cursor + (NB_NOTE_KEYS * 3) + 1] = -68;
    state |= STATE_NQDMJW;

    nqdmjw_state = 0;
}



/* NQDMJW */
uint32_t note_freq[] = { 0,
    26163, 27718, 29366, 31113, 32961, 34923, 36999, 39200, 41530, 44000,
    46616, 49388, 52325, 55437, 58733, 62225, 65926
};

/*
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
*/

void nqdmjw_feedinp(/*input_t inp*/) {
    /*
    if (inp.kc == KEY_ESC) {
        audio_off();
        state &= ~STATE_NQDMJW;
        tab_cursor_drop_down();
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
    */
}


void builtin_date(parsed_cmd_t cmd) {
    tab_cursor_drop_down(tab);
    if (cmd.count >= 2) {
        tab_cursor_drop_down(tab);
        tab_writestring(tab, "No arguments expected.");
        return;
    }
    /*
    human_time ht = get_human_time();
    // Display using ISO 8601
    tab_writedecimal(tab, ht.century);
    tab_writedecimal(tab, ht.year);
    tab_putchar(tab, '-');
    if (ht.month < 10) tab_putchar(tab, '0');
    tab_writedecimal(tab, ht.month);
    tab_putchar(tab, '-');
    if (ht.day < 10) tab_putchar(tab, '0');
    tab_writedecimal(tab, ht.day);
    tab_putchar(tab, 'T');
    if (ht.hour < 10) tab_putchar(tab, '0');
    tab_writedecimal(tab, ht.hour);
    tab_putchar(tab, ':');
    if (ht.minute < 10) tab_putchar(tab, '0');
    tab_writedecimal(tab, ht.minute);
    tab_putchar(tab, ':');
    if (ht.second < 10) tab_putchar(tab, '0');
    tab_writedecimal(tab, ht.second);
    tab_putchar(tab, 'Z');
    */
}
