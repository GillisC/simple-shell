#pragma once

#include <stdlib.h>

typedef struct c {
    char **tokens;
    size_t token_count;
    size_t tokens_allocation_size;
    char *in_dst;
    char *out_dst;
    int background;
    int pid; // the process id, -1 if command was not executed as a separate process
    struct c *next_command;
} Command;

Command *init_command();
Command *next_command(Command *cmd);
void append_token(Command *cmd, const char *str);
void print_cmd(Command *cmd);
void clear_commands(Command *cmd);
void reap_processes(Command *cmd);
void free_command(Command *cmd);
