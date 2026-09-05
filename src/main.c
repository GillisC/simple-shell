#include <assert.h>
#include <linux/limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

#include "command.h"
#include "parser.h"



void type_prompt(char *buffer) {
    printf("$ ");
    fgets(buffer, 256, stdin);
}


int is_builtin_command(const char *command_keyword);
int is_external_command(const char *command_keyword);

void run_builtin_command(Command *cmd);
void run_external_command(Command *cmd);

int is_builtin_command(const char *command_keyword) {
    static const char *builtins[4] = {
        "echo",
        "type",
        "pwd",
        "cd",
    };
    
    size_t num_builtins = sizeof(builtins) / sizeof(const char *);

    for (size_t i = 0; i < num_builtins; i++) {
        if (strcmp(builtins[i], command_keyword) == 0) {
            return 1;
        }
    }

    return 0;
}

// returns the directory if it finds it or NULL if not
const char *lookup_name_in_path(const char *name) {
    char *path = getenv("PATH");
    char *path_str = strtok(strdup(path), ":");;
    
    DIR *dir;
    struct dirent *dir_entry;

    while (path_str) {
        if ((dir = opendir(path_str)) == NULL) {
            perror("opendir() error\n");
            exit(1);
        }

        while (1) {
            if ((dir_entry = readdir(dir)) != NULL) {
                if (strcmp(dir_entry->d_name, name) != 0) {
                    continue;
                }
                closedir(dir);
                return path_str;
            }
            break;
        }
        closedir(dir);
        path_str = strtok(NULL, ":");
    }
    return NULL;
}

int is_external_command(const char *command_keyword) {
    if (lookup_name_in_path(command_keyword) != NULL) {
        return 1;
    }
    return 0;
}


void run_builtin_command(Command *cmd) {
    assert(cmd->token_count >= 1);

    char *command_keyword = cmd->tokens[0];

    if (strcmp("echo", command_keyword) == 0) {
        for (size_t i = 1; i < cmd->token_count; i++) {
            size_t write_size = strlen(cmd->tokens[i]);
            write(STDOUT_FILENO, cmd->tokens[i], write_size);

            if (i < cmd->token_count - 1) {
                write(STDOUT_FILENO, " ", 1);
            }
        }
        write(STDOUT_FILENO, "\n", 1);
    }
    else if (strcmp("type", command_keyword) == 0) {

        for (size_t i = 1; i < cmd->token_count; i++) {
            const char *argument = cmd->tokens[i];
            if (is_builtin_command(cmd->tokens[i])) {
                write(STDOUT_FILENO, argument, strlen(argument));
                char *to_write = " is a builtin";
                write(STDOUT_FILENO, to_write, strlen(to_write));
            }
            else if (is_external_command(cmd->tokens[i])) {
                const char *external_path = lookup_name_in_path(cmd->tokens[i]);
                write(STDOUT_FILENO, argument, strlen(argument));
                char *to_write = " is ";
                write(STDOUT_FILENO, to_write, strlen(to_write));
                write(STDOUT_FILENO, external_path, strlen(external_path));
                write(STDOUT_FILENO, "/", 1);
                write(STDOUT_FILENO, cmd->tokens[i], strlen(cmd->tokens[i]));
            }
            else {
                char *could_not_find = "type: Could not find '";
                write(STDOUT_FILENO, could_not_find, strlen(could_not_find));
                write(STDOUT_FILENO, argument, strlen(argument));
                write(STDOUT_FILENO, "'", 1);
            }
            write(STDOUT_FILENO, "\n", 1);
        }
    }
    else if (strcmp("pwd", command_keyword) == 0) {
        static char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            write(STDOUT_FILENO, cwd, strlen(cwd));
            write(STDOUT_FILENO, "\n", 1);
        }
        else {
            perror("getcwd() error");
            return;
        }
    }
    else if (strcmp("cd", command_keyword) == 0) {
        if (cmd->token_count == 2) {
            if (chdir(cmd->tokens[1]) == -1) {
                perror("chdir() error");
            }
        }
        else {
            fprintf(stderr, "cd: expected 1 arguments; got %zu\n", cmd->token_count - 1);
        }
    }
    else {
        fprintf(stderr, "%s: not implemented", command_keyword);
    }
}

void run_external_command(Command *cmd);

int main() {
#define TESTING 0
#if TESTING

#else
    // clear the terminal
    write(STDOUT_FILENO, "\033[2J\033[H", 7);

    char buffer[256];
    Command *cmd = init_command();

    while (1) {
        type_prompt(buffer);
        parse_input(cmd, buffer);

        if (cmd->token_count == 0) continue;

        if (is_builtin_command(cmd->tokens[0])) {
            run_builtin_command(cmd);
        }
        else if (is_external_command(cmd->tokens[0])) {
            // run_external_cmd();
        }
        else {
            fprintf(stderr, "Unknown cmd: %s\n", cmd->tokens[0]);
        }



        // for (size_t i = 0; i < command->tokens->count; i++) {
        //     printf("'%s' ", command->tokens->data[i]);
        // }
        // printf("\n");
    }

#endif

    return 0;
}
