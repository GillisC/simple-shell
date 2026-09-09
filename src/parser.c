#include "parser.h"
#include "command.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>


typedef void (*action_func)(ParsingContext *ctx);

typedef struct {
    action_func next_action;
    ParsingState new_state;
} Transition;

void action_do_nothing(ParsingContext *ctx) { 
    (void)ctx;
}

void action_append_char(ParsingContext *ctx) { 
    ctx->buffer[ctx->curr_buffer_index++] = ctx->curr_char;
}

void action_add_token(ParsingContext *ctx) { 
    ctx->buffer[ctx->curr_buffer_index] = 0;
    append_token(ctx->curr, strdup(ctx->buffer));
    ctx->curr_buffer_index = 0;
}

void action_redirect_out_mode(ParsingContext *ctx) {
    ctx->redirect_status = 1;
}

void action_redirect_in_mode(ParsingContext *ctx) {
    ctx->redirect_status = -1;
}

void action_error(ParsingContext *ctx) {
    fprintf(stderr, "Error when parsing input at character %c\n", ctx->curr_char);
    exit(1);
}

void action_add_redirection_dst(ParsingContext *ctx) {
    assert(ctx->redirect_status != 0);

    if (ctx->redirect_status == -1) {
        // input
        ctx->buffer[ctx->curr_buffer_index] = 0;
        ctx->curr->in_dst = strdup(ctx->buffer);
        ctx->curr_buffer_index = 0;
    }
    else if (ctx->redirect_status == 1) {
        // output 
        ctx->buffer[ctx->curr_buffer_index] = 0;
        ctx->curr->out_dst = strdup(ctx->buffer);
        ctx->curr_buffer_index = 0;
    }
}

void action_append_command(ParsingContext *ctx) {
    assert(ctx->curr->next_command == NULL);
    ctx->curr->next_command = init_command();
    ctx->curr = ctx->curr->next_command;
}

static const Transition parser_transition_table[NUM_PARSING_STATES][NUM_CHARACTER_TYPES] = {
    [START] = {
        [SPACE] = { action_do_nothing, START },
        [CHAR] = { action_append_char, OUTSIDE },
        [NEWLINE] = { action_do_nothing, START },
        [REDIRECT_OUT] = { action_redirect_out_mode, REDIRECT_SEEKING },
        [REDIRECT_IN] = { action_redirect_in_mode, REDIRECT_SEEKING },
        [PIPE] = { action_error, START}
    },
    [OUTSIDE] = {
        [SPACE] = { action_add_token, OUTSIDE },
        [CHAR] = { action_append_char, OUTSIDE },
        [NEWLINE] = { action_add_token, OUTSIDE },
        [REDIRECT_OUT] = { action_redirect_out_mode, REDIRECT_SEEKING },
        [REDIRECT_IN] = { action_redirect_in_mode, REDIRECT_SEEKING },
        [PIPE] = { action_append_command, START}
    },
    [REDIRECT_SEEKING] = {
        [SPACE] = { action_do_nothing, REDIRECT_SEEKING },
        [CHAR] = { action_append_char, REDIRECT_INSIDE },
        [NEWLINE] = { action_error, REDIRECT_SEEKING },
        [REDIRECT_OUT] = { action_error, REDIRECT_SEEKING },
        [REDIRECT_IN] = { action_error, REDIRECT_SEEKING },
        [PIPE] = { action_error, REDIRECT_SEEKING}
    },
    [REDIRECT_INSIDE] = {
        [SPACE] = { action_add_redirection_dst, OUTSIDE },
        [CHAR] = { action_append_char, REDIRECT_INSIDE },
        [NEWLINE] = { action_add_redirection_dst, OUTSIDE },
        [REDIRECT_OUT] = { action_error, REDIRECT_INSIDE },
        [REDIRECT_IN] = { action_error, REDIRECT_INSIDE },
        [PIPE] = { action_error, REDIRECT_SEEKING}
    },
};

void init_parser(ParsingContext *ctx) {
    ctx->curr_buffer_index = 0;
    ctx->curr_state = START;
}

CharacterType get_character_type(const char c) {
    switch (c) {
        case ' ':
            return SPACE;
        case '\n':
            return NEWLINE;
        case '>':
            return REDIRECT_OUT;
        case '<':
            return REDIRECT_IN;
        case '|':
            return PIPE;
        default:
            return CHAR;
    }
}

void parse_input(Command *cmd, const char *input) {
    ParsingContext parsing_context; 
    init_parser(&parsing_context);

    clear_commands(cmd);
    assert(cmd->next_command == NULL);
    parsing_context.head = cmd;
    parsing_context.curr = cmd;

    const char *curr_char = input;

    while (1) {
        char character = *curr_char++;
        if (character) {
            parsing_context.curr_char = character;
            CharacterType char_type = get_character_type(parsing_context.curr_char);
            Transition transition = parser_transition_table[parsing_context.curr_state][char_type];

            transition.next_action(&parsing_context);
            parsing_context.curr_state = transition.new_state;
        }
        else {
            // we are at the end, append the current token or quit
            if (parsing_context.curr_buffer_index == 0) {
                return;
            }
            else {
                action_add_token(&parsing_context);
            }
            break;
        }
    }
} 
