#include "command.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/wait.h>

Command *init_command() {
    Command *result = malloc(sizeof(Command));
    if (!result) {
        fprintf(stderr, "Error: failed to allocate Command\n");
        exit(1);
    }
    result->tokens_allocation_size = 8;
    result->token_count = 0;
    result->tokens = calloc(result->tokens_allocation_size, sizeof(char *));
    result->tokens[0] = NULL;
    result->in_dst = NULL;
    result->out_dst = NULL;
    result->next_command = NULL;

    return result;
}

Command *next_command(Command *cmd) {
    assert(cmd && "should not reach a situation where cmd is null");
    if (cmd->next_command == NULL) return NULL;
    return cmd->next_command;
}

void append_token(Command *cmd, const char *str) {
    if (str == NULL)
        return;

    if (cmd->token_count + 1 >= cmd->tokens_allocation_size) {
        // realloc
        size_t new_allocation_size = (int)(cmd->tokens_allocation_size * 2);
        void *temp_data = realloc(cmd->tokens, new_allocation_size * sizeof(char *));
        if (!temp_data) {
            fprintf(stderr, "Error: failed to reallocate growing string array\n");
            exit(1);
        }
        cmd->tokens = (char **)temp_data;
        cmd->tokens_allocation_size = new_allocation_size;
    }

    // append
    cmd->tokens[cmd->token_count] = strdup(str);
    cmd->token_count++;
    cmd->tokens[cmd->token_count] = NULL;
}

void print_cmd(Command *cmd) {
    Command *curr = cmd;
    while (curr) {
        printf("--------------------------------\n");
        printf("input:       %s\n", curr->in_dst);
        printf("output:      %s\n", curr->out_dst);
        printf("background?: %s\n", curr->background == 0 ? "no" : "yes");
        printf("command: "); 
        for (size_t i = 0; i < curr->token_count; i++) {
            printf("%s ", curr->tokens[i]);
        }
        printf("\n");
        printf("--------------------------------\n");

        curr = next_command(curr);
    }
}

void clear_commands(Command *cmd) {
    assert(cmd);

    for (size_t i = 0; i < cmd->token_count; i++) {
        cmd->tokens[i] = NULL;
    }
    cmd->token_count = 0;
    cmd->in_dst = NULL;
    cmd->out_dst = NULL;
    cmd->background = 0;

    free_command(cmd->next_command); // free the next command
    cmd->next_command = NULL;
}

void reap_processes(Command *cmd) {
    Command *curr = cmd;
    int status;
    while (curr) {
        if (curr->pid != -1)
            waitpid(curr->pid, &status, 0);
        curr = next_command(curr);
    }
}

void free_command(Command *cmd) {
    if (!cmd) return;

    free_command(cmd->next_command);

    for (size_t i = 0; i < cmd->token_count; i++) {
        free(cmd->tokens[i]);
    }
    free(cmd->tokens);
    free(cmd);
}
