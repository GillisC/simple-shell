#pragma once

#include <stdlib.h>

#include "command.h"

typedef enum {
    START,
    OUTSIDE,

    NUM_PARSING_STATES
} ParsingState;

typedef struct {
    ParsingState curr_state;
    char curr_char;
    char buffer[512];
    size_t curr_buffer_index;
} ParsingContext;

void init_parser(ParsingContext *ctx);
void parse_input(Command *cmd, const char *input);
