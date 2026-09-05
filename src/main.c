#include <assert.h>
#include <linux/limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

typedef struct {
    char **tokens;
    size_t token_count;
    size_t tokens_allocation_size;
} Command;

Command *init_command();
void append_command(Command *cmd, const char *str);
void clear_command(Command *command);
void free_command(Command *command);

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

    return result;
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

void clear_command(Command *cmd) {
    assert(cmd);

    for (size_t i = 0; i < cmd->token_count; i++) {
        cmd->tokens[i] = NULL;
    }

    cmd->token_count = 0;
}

void free_command(Command *cmd) {
    assert(cmd);

    for (size_t i = 0; i < cmd->token_count; i++) {
        free(cmd->tokens[i]);
    }
    free(cmd->tokens);
    free(cmd);
}

void type_prompt(char *buffer) {
    printf("$ ");
    fgets(buffer, 256, stdin);
}


// Parser

void read_command(Command *cmd, const char *input);
void free_command(Command *cmd);

typedef enum {
    START,
    OUTSIDE,

    NUM_PARSING_STATES
} ParsingState;

typedef enum {
    SPACE,
    CHAR,
    NEWLINE,
    
    NUM_CHARACTER_TYPES 
} CharacterType;


typedef struct {
    ParsingState curr_state;
    char curr_char;
    char buffer[512];
    size_t curr_buffer_index;
} ParsingContext;

void init_parser(ParsingContext *ctx);

typedef void (*action_func)(Command *cmd, ParsingContext *ctx);

typedef struct {
    action_func next_action;
    ParsingState new_state;
} Transition;

void action_do_nothing(Command *cmd, ParsingContext *ctx) { return; }

void action_append_char(Command *cmd, ParsingContext *ctx) { 
    ctx->buffer[ctx->curr_buffer_index++] = ctx->curr_char;
}

void action_add_token(Command *cmd, ParsingContext *ctx) { 
    ctx->buffer[ctx->curr_buffer_index] = 0;
    append_token(cmd, strdup(ctx->buffer));
    ctx->curr_buffer_index = 0;
}

static const Transition parser_transition_table[NUM_PARSING_STATES][NUM_CHARACTER_TYPES] = {
    [START] = {
        [SPACE] = { action_do_nothing, START },
        [CHAR] = { action_append_char, OUTSIDE },
        [NEWLINE] = { action_do_nothing, START },
    },
    [OUTSIDE] = {
        [SPACE] = { action_add_token, START },
        [CHAR] = { action_append_char, OUTSIDE },
        [NEWLINE] = { action_add_token, OUTSIDE },
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
        default:
            return CHAR;
    }
}

void read_command(Command *cmd, const char *input) {
    // echo      hello -> 'echo' 'hello'
    ParsingContext parsing_context; 
    init_parser(&parsing_context);
    clear_command(cmd);

    const char *curr_char = input;

    while (1) {
        char character = *curr_char++;
        if (character) {
            parsing_context.curr_char = character;
            CharacterType char_type = get_character_type(parsing_context.curr_char);
            Transition transition = parser_transition_table[parsing_context.curr_state][char_type];

            transition.next_action(cmd, &parsing_context);
            parsing_context.curr_state = transition.new_state;
        }
        else {
            // we are at the end, append the current token or quit
            if (parsing_context.curr_buffer_index == 0) {
                return;
            }
            else {
                action_add_token(cmd, &parsing_context);
            }
            break;
        }
    }
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
        read_command(cmd, buffer);

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
