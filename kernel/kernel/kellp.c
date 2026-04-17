/* Kernel Shell Prototype - C */

#include <kellp.h>

#include <string.h>
#include <kernel/tty.h>
#include <kernel/allocator.h>

#define TAB_WIDTH 80
#define TAB_HEIGHT 25
#define TAB_SIZE TAB_WIDTH * TAB_HEIGHT
static char tab[TAB_SIZE];
static size_t cursor;
static size_t cmd_start;


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

static void drop_cursor() {
    while (cmd_start < cursor && tab[cursor - 1] == 0x00) cursor--;
}


/* Inspired from TTY */

void kellp_putchar(char ch) {
    tab[cursor] = ch;
    if (cursor++ == TAB_SIZE) {
        tab_scroll(1);
        cursor -= TAB_WIDTH;
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

size_t count_args(char *cmd_line, size_t len) {
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
size_t count_until(char *str, size_t len, char ch) {
    for (size_t i = 0; i < len; i++) {
        if (str[i] == ch) return i;
    }
    return len;
}

typedef struct {
    int count;
    char **tbl;
} parsed_cmd;

#define NULL_CMD (parsed_cmd){ 0, NULL }

static void write_arg(char **destptr, char *src, size_t len) {
    char *arg = (char *)kmalloc((1 + len) * sizeof(char));
    if (arg == NULL) { *destptr = NULL; return; }
    memcpy(arg, src, len);
    arg[len] = '\0';
    *destptr = arg;
}

// Parseur de ligne de commange
// A ajouter : l'échappement des chars
parsed_cmd parse_command(char *cmd_line, size_t len) {
    size_t arg_count = count_args(cmd_line, len);

    char **tbl = (char **)kmalloc(arg_count * sizeof(void *));
    if (tbl == NULL) { return NULL_CMD; }

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
                    if (tbl[c] == NULL) { return NULL_CMD; }
                    c++;
                    state &= ~0x02; // stop reading
                }
                state |= 0x01; // start sniffing
                break;

            case '"':
                if ((state & 0x04) == 0) {
                    if (i == len - 1) { return NULL_CMD; }
                    word_start = i + 1;
                    state = 0x04; // in quotes
                } else {
                    write_arg(tbl + c, cmd_line + word_start, i - word_start);
                    if (tbl[c] == NULL) { return NULL_CMD; }
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

    if (state & 0x04) { return NULL_CMD; }

    if (state & 0x02) {
        write_arg(tbl + c, cmd_line + word_start, len - word_start);
        if (tbl[c] == NULL) { return NULL_CMD; }
        c++;
    }

    return (parsed_cmd){ c, tbl };
}

// temp
void print_parsed(parsed_cmd cmd) {
    for (int i = 0; i < cmd.count; i++) {
        kellp_feedinp('\n');
        kellp_writestring(cmd.tbl[i]);
    }
}





/* Interface */

void init_kellp() {
    for (size_t i = 0; i < TAB_SIZE; i++) {
        tab[i] = 0x00;
    }

    tab[0] = '>'; tab[1] = '>'; tab [2] = ' ';
    cmd_start = 3;
    cursor = 3;

    terminal_update(tab, cursor);
}

void kellp_feedinp(char ch) {
    switch (ch)
    {
        case '\n': // A changer : correspondrait au Shift + Enter
            cursor = TAB_WIDTH * (cursor / TAB_WIDTH + 1);
            if (cursor >= TAB_SIZE) {
                tab_scroll(1);
                cursor -= TAB_WIDTH;
            }
            break;

        case 'N': // temporaire
            print_parsed(parse_command(tab + cmd_start, cursor - cmd_start));
            cursor = TAB_WIDTH * (cursor / TAB_WIDTH + 1);
            if (cursor >= TAB_SIZE) {
                tab_scroll(1);
                cursor -= TAB_WIDTH;
            }
            kellp_writestring(">> ");
            cmd_start = cursor;
            break;

        case '\r': // input possible même ?
            cursor = TAB_WIDTH * (cursor / TAB_WIDTH);
            break;

        case '\b':
            if (cursor == cmd_start) break;
            cursor--;
            tab[cursor] = 0x00;
            drop_cursor();
            break;

        default:
            kellp_putchar(ch);
            break;
    }

    terminal_update(tab, cursor);
}

