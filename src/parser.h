#pragma once

#include <stdlib.h>

#include "command.h"

typedef enum {
    START,
    OUTSIDE,
    REDIRECT_ADJACENT,
    REDIRECT_SEEKING,
    REDIRECT_INSIDE,

    NUM_PARSING_STATES
} ParsingState;

typedef enum {
    SPACE,
    CHAR,
    NEWLINE,
    REDIRECT_OUT,
    REDIRECT_IN,
    PIPE,

    NUM_CHARACTER_TYPES 
} CharacterType;

typedef struct {
    ParsingState curr_state;
    char buffer[512];
    char curr_char;
    size_t curr_buffer_index;
    int redirect_status; // -1 in, 0 none, 1 out
    Command *head;
    Command *curr;
} ParsingContext;

void init_parser(ParsingContext *ctx);
void parse_input(Command *cmd, const char *input);
