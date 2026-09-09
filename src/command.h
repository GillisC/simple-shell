#pragma once

#include <stdlib.h>

typedef struct c {
    char **tokens;
    size_t token_count;
    size_t tokens_allocation_size;
    char *in_dst;
    char *out_dst;
    int background;
    struct c *next_command;
} Command;

Command *init_command();
void append_token(Command *cmd, const char *str);
void print_cmd(Command *cmd);
void clear_commands(Command *command);
void free_command(Command *command);
