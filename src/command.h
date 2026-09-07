#pragma once

#include <stdlib.h>

typedef struct {
    char **tokens;
    size_t token_count;
    size_t tokens_allocation_size;
    char *in_dst;
    char *out_dst;
} Command;

Command *init_command();
void append_token(Command *cmd, const char *str);
void clear_command(Command *command);
void free_command(Command *command);
