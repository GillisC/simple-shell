#include <assert.h>
#include <linux/limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <wait.h>
#include <fcntl.h>

#include "command.h"
#include "parser.h"

void type_prompt(char *buffer) {
    printf("$ ");
    fgets(buffer, 256, stdin);
}


int is_builtin_command(const char *command_keyword);
int is_external_command(const char *command_keyword);

void run_builtin_command(Command *cmd, char *format_buffer);
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
            return 0;
        }
    }

    return 1;
}

int is_name_in_path(const char *name);
char *get_path_to_external(const char *name);

int is_name_in_path(const char *name) {
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
                return 0;
            }
            break;
        }
        closedir(dir);
        path_str = strtok(NULL, ":");
    }
    return 1;
}

char *get_path_to_external(const char *name) {
    char *path = getenv("PATH");
    char *path_str = strtok(strdup(path), ":");;
    
    DIR *dir;
    struct dirent *dir_entry;
    char path_buffer[PATH_MAX];

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
                snprintf(path_buffer, PATH_MAX, "%s/%s", path_str, name);
                return strdup(path_buffer);
            }
            break;
        }
        closedir(dir);
        path_str = strtok(NULL, ":");
    }
    return NULL;
}

int is_external_command(const char *command_keyword) {
    if (is_name_in_path(command_keyword) != 0) {
        return 1;
    }
    return 0;
}

void run_builtin_command(Command *cmd, char *format_buffer) {
    assert(cmd->token_count >= 1);
    
    int fd_in =  STDIN_FILENO;
    int fd_out = STDOUT_FILENO;

    if (cmd->in_dst != NULL) {
        fd_in = open(cmd->in_dst, O_RDONLY);
    }
    if (cmd->out_dst != NULL) {
        mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
        fd_out = open(cmd->out_dst, O_WRONLY | O_CREAT, mode);
    }

    char *command_keyword = cmd->tokens[0];

    if (strcmp("echo", command_keyword) == 0) {
        for (size_t i = 1; i < cmd->token_count; i++) {
            size_t write_size = strlen(cmd->tokens[i]);
            write(fd_out, cmd->tokens[i], write_size);

            if (i < cmd->token_count - 1) {
                write(fd_out, " ", 1);
            }
        }
        write(fd_out, "\n", 1);
    }
    else if (strcmp("type", command_keyword) == 0) {

        for (size_t i = 1; i < cmd->token_count; i++) {
            const char *argument = cmd->tokens[i];

            if (is_builtin_command(cmd->tokens[i]) == 0) {
                snprintf(
                    format_buffer,
                    PATH_MAX,
                    "%s is a builtin",
                    argument
                );
                write(fd_out, format_buffer, strlen(format_buffer));
            }
            else if (is_external_command(cmd->tokens[i]) == 0) {
                const char *path_to_external = get_path_to_external(cmd->tokens[i]);
                snprintf(
                    format_buffer,
                    PATH_MAX,
                    "%s is %s",
                    argument, path_to_external
                );
                write(fd_out, format_buffer, strlen(format_buffer));
            }
            else {
                snprintf(
                    format_buffer,
                    PATH_MAX,
                    "type: Could not find '%s'",
                    argument
                );
                write(fd_out, format_buffer, strlen(format_buffer));
            }
            write(fd_out, "\n", 1);
        }
    }
    else if (strcmp("pwd", command_keyword) == 0) {
        static char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            write(fd_out, cwd, strlen(cwd));
            write(fd_out, "\n", 1);
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

    // close file descriptors in the case of redirection
    if (cmd->in_dst != NULL) {
        if (close(fd_in) == -1) {
            perror("close() error");
            exit(EXIT_FAILURE);
        }
    }
    if (cmd->out_dst != NULL) {
        if (close(fd_out) == -1) {
            perror("close() error");
            exit(EXIT_FAILURE);
        }
    }
}

#define MALLOC_SIZE 10
void run_external_command(Command *cmd) {
    const char *external_path = get_path_to_external(cmd->tokens[0]);
    char **argv = malloc(sizeof(char**) * MALLOC_SIZE);
    extern char **environ; 
    assert(cmd->token_count + 1 <= MALLOC_SIZE);

    for (size_t i = 0; i <= cmd->token_count; i++) {
        if (i == cmd->token_count) {
            argv[i] = NULL;
        }
        else {
            argv[i] = strdup(cmd->tokens[i]);
        }
    }
    // loop over the commands in the pipeline
    // if there is another command in the pipeline later, create a pipe.

    pid_t pid;
    int status;

    pid = fork();
    if (pid == -1) {
        perror("fork()");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) {
        // child process
        execve(external_path, argv, environ);
        perror("execve() error");
        exit(EXIT_FAILURE);
    }
    else {
        // parent
        waitpid(pid, &status, WCONTINUED | WUNTRACED);   
    }
}
#undef MALLOC_SIZE

int main() {
#define TESTING 0

#if TESTING
    // do test code here
#else
    // clear the terminal
    write(STDOUT_FILENO, "\033[2J\033[H", 7);

    char buffer[256];
    char format_buffer[PATH_MAX];
    Command *cmd = init_command();

    while (1) {
        type_prompt(buffer);
        parse_input(cmd, buffer);

        if (cmd->token_count == 0) continue;

        if (is_builtin_command(cmd->tokens[0]) == 0) {
            run_builtin_command(cmd, format_buffer);
        }
        else if (is_external_command(cmd->tokens[0]) == 0) {
            run_external_command(cmd);
        }
        else {
            fprintf(stderr, "Unknown cmd: %s\n", cmd->tokens[0]);
        }
    }

#endif

    return 0;
}
