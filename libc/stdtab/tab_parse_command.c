#include <stdtab.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static size_t count_args(char *cmd_line, size_t len) {
    uint8_t state = 0x00; // 0:sniffing 2:in-quotes
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

static void write_arg(char **destptr, char *src, size_t len) {
    char *arg = (char *)malloc((1 + len) * sizeof(char));
    if (arg == NULL) { *destptr = NULL; return; }
    memcpy(arg, src, len);
    arg[len] = '\0';
    *destptr = arg;
}


static parsed_cmd_t parse_command(char *cmd_line, size_t len) {
    size_t arg_count = count_args(cmd_line, len);

    char **tbl = (char **)malloc(arg_count * sizeof(void *));
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

    return (parsed_cmd_t){ c, tbl };
}

parsed_cmd_t tab_parse_command(tab_t *tab) {
    size_t cursor = tab->cursor;
    size_t cmd_start = tab->cmd_start;
    if (cursor <= cmd_start) return NULL_PARSED_CMD;
    return parse_command(tab->data + cmd_start, cursor - cmd_start);
}

