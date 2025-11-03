#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

typedef struct arg_parser arg_parser;
typedef struct arg* arg;
typedef struct cmd* cmd;
typedef struct cmd* subcmd;

// Create new `cmd`.
static cmd cmd_new(const char* name);
// Free `cmd`.
static void cmd_free(cmd cmd);
// Set help message of `cmd`.
static void cmd_help(cmd cmd, const char* help);
// Set description of `cmd`.
static void cmd_desc(cmd cmd, const char* desc);
// Add enum value to set when command is run.
static void cmd_enum(cmd cmd, void* data, int value);
// Add subcommand with `name` to `cmd`.
static cmd cmd_subcmd(cmd cmd, const char* name);
// Add `arg` with `name` to `cmd`.
static arg cmd_arg(cmd cmd, const char* name);

// Add `help` subcommand to `cmd` that displays help info for subcommands.
static cmd cmd_add_help_subcmd(cmd cmd);

// Validate `cmd` returning `false` if invalid.
static bool cmd_validate(const cmd cmd);
// Parse `cmd` exiting on errors.
static void cmd_parse(const cmd cmd, int argc, const char** argv);

// Print usage manual of `cmd` to `file`.
static void cmd_fprint_usage(FILE *file, const cmd cmd);

// Print help manual of `cmd` to `file`.
static void cmd_fprint_help(FILE *file, const cmd cmd);

// Set help message of argument.
static void arg_help(arg arg, const char* help);
// Set usage message of argument.
static void arg_usage(arg arg, const char* usage);
// Set short name, e.g. `-f`, of argument.
static void arg_short(arg arg, char name);
// Set long name, e.g. `--flag`, of argument.
static void arg_long(arg arg, const char* name);
// Set `check` boolean that is set when `arg` is parsed.
static void arg_check(arg arg, bool* check);
// Set value parser of `arg`.
static void arg_value(arg arg, void* data, arg_parser parser);

// Print a formatted argument error.
static int arg_err(const char* fmt, ...);

// Parser for an value `arg`.
struct arg_parser {
    // Function parsing arguments, returns the number of arguments parsed.
    //
    // If error occurs, returns negative number.
    int (*parse)(void* data, int argc, const char** argv);

    // Minimum number of required arguments.
    int   count;
};

// Argument in a `cmd`.
struct arg {
    // Name of the argument.
    const char* name;
    // Help message of the argument.
    const char* help;
    // Usage message of the argument.
    const char* usage;

    // Short name of the argument, `'\0'` if not set.
    char        short_name;
    // Long name of the argument, may be `NULL`.
    const char* long_name;

    // Set when argument is parsed.
    bool*       check;

    // User data of the parser.
    void*       data;
    // Argument parser.
    arg_parser  parser;
};

// Command line command.
struct cmd {
    // Name of the command.
    const char* name;
    // Help message of the command.
    const char* help;
    // Description of the command.
    const char* desc;

    // Pointer to enum, set when command is run.
    int*        uenum;
    // The enum value to set `data` to.
    int         value;

    // Number of arguments in command.
    size_t      args_len;
    // Capacity of `args` array.
    size_t      args_cap;
    // Pointer to array of arguments.
    arg*        args;

    // Number of subcommands in command.
    size_t      cmds_len;
    // Capacity of `cmds` array.
    size_t      cmds_cap;
    // Pointer to array of subcommands.
    cmd*        cmds;

    // Parent command, may be `NULL`.
    cmd         parent;
};

// Parser for required `const char*`.
static const arg_parser arg_str;
// Parser for required `int`.
static const arg_parser arg_int;
// Parser for number of instances, e.g. `-vvv` parsed as 3.
static const arg_parser arg_count;
// Parser for required `float`.
static const arg_parser arg_float;

/* ----- implementation ----- */

// Check if an `arg` is an option.
static inline bool arg__is_option(const arg arg) {
    return arg->short_name != '\0' || arg->long_name != NULL;
}

// Get count of options in `cmd`.
static inline size_t cmd__option_count(
    const cmd cmd
) {
    size_t count = 0;

    for (size_t i = 0; i < cmd->args_len; i++) {
        if (arg__is_option(cmd->args[i])) {
            count += 1;
        }
    }

    return count;
}

static inline void cmd__fprint_arguments(
    FILE*     file,
    const cmd cmd
) {
    if (cmd__option_count(cmd) < cmd->args_len) {
        fprintf(file, "\n");
        fprintf(file, "\e[0;32m");
        fprintf(file, "arguments:\n");
        fprintf(file, "\e[0;0m");

        for (size_t i = 0; i < cmd->args_len; i++) {
            arg arg = cmd->args[i];

            if (arg__is_option(arg)) continue;

            fprintf(file, "\e[0;36m");

            size_t len = 0;

            if (arg->usage) {
                fprintf(file, "  %s", arg->usage);
                len += strlen(arg->usage);
            } else {
                fprintf(file, "  <%s>", arg->name);
                len += 2 + strlen(arg->name);
            }

            fprintf(file, "\e[0;0m");

            if (arg->help) {
                for (size_t j = len; j < 32; j++) fprintf(file, " ");
                fprintf(file, " %s", arg->help);
            }

            fprintf(file, "\n");
        }
    }
}

static inline void cmd__fprint_options(
    FILE*     file,
    const cmd cmd
) {
    if (cmd__option_count(cmd) > 0) {
        fprintf(file, "\n");
        fprintf(file, "\e[0;32m");
        fprintf(file, "options:\n");
        fprintf(file, "\e[0;0m");

        for (size_t i = 0; i < cmd->args_len; i++) {
            arg arg = cmd->args[i];

            if (!arg__is_option(arg)) continue;

            fprintf(file, "  ");
            size_t len = 2;

            fprintf(file, "\e[0;36m");
            
            if (arg->short_name != '\0') {
                fprintf(file, "-%c", arg->short_name);

                if (arg->long_name) {
                    fprintf(file, ", ");
                    len += 2;
                }
            } else {
                fprintf(file, "    ");
                len += 2;
            }

            if (arg->long_name) {
                fprintf(file, "--%s", arg->long_name);
                len += 2 + strlen(arg->long_name);
            }

            if (arg->usage) {
                fprintf(file, " %s", arg->usage);
                len += 1 + strlen(arg->usage);
            }

            fprintf(file, "\e[0;0m");

            if (arg->help) {
                for (size_t j = len; j < 32; j++) fprintf(file, " ");
                fprintf(file, " %s", arg->help);
            }

            fprintf(file, "\n");
        }
    }
}

static inline void cmd__fprint_commands(
    FILE*     file,
    const cmd cmd
) {
    if (cmd->cmds_len > 0) {
        fprintf(file, "\n");
        fprintf(file, "\e[0;32m");
        fprintf(file, "commands:\n");
        fprintf(file, "\e[0;0m");

        for (size_t i = 0; i < cmd->cmds_len; i++) {
            struct cmd* subcmd = cmd->cmds[i];

            fprintf(file, "\e[0;36m");
            fprintf(file, "  %s", subcmd->name);
            fprintf(file, "\e[0;0m");

            size_t len = strlen(subcmd->name);

            if (subcmd->help) {
                for (size_t j = len; j < 32; j++) fprintf(file, " ");
                fprintf(file, " %s", subcmd->help);
            }

            fprintf(file, "\n");
        }
    }
}

static inline void cmd__fprint_path(
    FILE*     file,
    const cmd cmd
) {
    if (cmd->parent) {
        cmd__fprint_path(file, cmd->parent);
    }

    fprintf(file, " %s", cmd->name);
}

static inline void cmd_fprint_usage(
    FILE*     file,
    const cmd cmd
) {
    fprintf(file, "\e[0;32m");
    fprintf(file, "usage:");
    fprintf(file, "\e[0;36m");

    cmd__fprint_path(file, cmd);

    for (size_t i = 0; i < cmd->args_len; i++) {
        arg arg = cmd->args[i];

        if (arg__is_option(arg)) continue;
        if (arg->usage) fprintf(file, " %s", arg->usage);
        else            fprintf(file, " <%s>", arg->name);
    }

    if (cmd__option_count(cmd) > 0) {
        fprintf(file, " [options]");
    }

    if (cmd->cmds_len > 0) {
        fprintf(file, " [command]");
    }

    fprintf(file, "\e[0;0m");
    fprintf(file, "\n");
}
 
static inline void cmd_fprint_help(
    FILE*     file,
    const cmd cmd
) {
    if (cmd->desc) {
        fprintf(file, "%s\n\n", cmd->desc);
    }

    cmd_fprint_usage(file, cmd);

    cmd__fprint_arguments(file, cmd);
    cmd__fprint_options(file, cmd);
    cmd__fprint_commands(file, cmd);
}

static int arg__parse_help(void* data, int argc, const char** argv) {
    (void) argc;
    (void) argv;

    cmd_fprint_help(stderr, data);

    exit(0);
}

static const arg_parser arg__help = {
    .parse = arg__parse_help,
    .count = 0,
};

static inline cmd cmd_new(
    const char* name
) {
    cmd cmd = malloc(sizeof *cmd);

    cmd->name     = name;
    cmd->help     = NULL;
    cmd->desc     = NULL;

    cmd->uenum     = NULL;
    cmd->value    = 0;

    cmd->args_len = 0;
    cmd->args_cap = 1;
    cmd->args     = malloc(sizeof *cmd->args);

    cmd->cmds_len = 0;
    cmd->cmds_cap = 1;
    cmd->cmds     = malloc(sizeof *cmd->cmds);

    cmd->parent   = NULL;

    /* create help option */

    arg help_arg = cmd_arg(cmd, "help");
    arg_help (help_arg, "print help");
    arg_short(help_arg, 'h');
    arg_long (help_arg, "help");
    arg_value(help_arg, cmd, arg__help);

    return cmd;
}

static inline void cmd_free(cmd cmd) {
    for (size_t i = 0; i < cmd->cmds_len; i++) {
        cmd_free(cmd->cmds[i]);
    }

    for (size_t i = 0; i < cmd->args_len; i++) {
        free(cmd->args[i]);
    }

    free(cmd->args);
    free(cmd->cmds);
    free(cmd);
}

static inline void cmd_help(
    cmd         cmd,
    const char* help
) {
    cmd->help = help;
}

static inline void cmd_desc(
    cmd         cmd,
    const char* desc
) {
    cmd->desc = desc;
}

static inline void cmd_enum(
    cmd   cmd,
    void* data,
    int   value
) {
    cmd->uenum  = data;
    cmd->value = value;
}

static inline cmd cmd_subcmd(
    cmd         cmd,
    const char* name
) { 
    if (cmd->cmds_len == cmd->cmds_cap) {
        cmd->cmds_cap *= 2;
        cmd->cmds = realloc(
            cmd->cmds,
            (sizeof *cmd->cmds) * cmd->cmds_cap
        );
    }

    cmd->cmds_len++;

    cmd->cmds[cmd->cmds_len - 1]         = cmd_new(name);
    cmd->cmds[cmd->cmds_len - 1]->parent = cmd;

    return cmd->cmds[cmd->cmds_len - 1];
}

static inline arg cmd_arg(
    cmd         cmd,
    const char* name
) {
    if (cmd->args_len == cmd->args_cap) {
        if (cmd->args_cap == 0) cmd->args_cap  = 1;
        else                    cmd->args_cap *= 2;

        cmd->args = realloc(
            cmd->args,
            (sizeof *cmd->args) * cmd->args_cap
        );
    }

    cmd->args[cmd->args_len] = malloc(sizeof *cmd->args[cmd->args_len]);
    arg arg = cmd->args[cmd->args_len];

    arg->name       = name;
    arg->help       = NULL;
    arg->usage      = NULL;
    arg->short_name = '\0';
    arg->long_name  = NULL;

    arg->check      = NULL;

    arg->data       = NULL;
    arg->parser     = (arg_parser){0};

    cmd->args_len++;

    return arg;
}

static inline int cmd__parse_help(
    void*        data,
    int          argc,
    const char** argv
) {
    cmd curr = data;

    for (int i = 0; i < argc; i++) {
        bool cmd_found = false;

        for (size_t j = 0; j < curr->cmds_len; j++) {
            if (strcmp(curr->cmds[j]->name, argv[i]) != 0) continue;

            curr = curr->cmds[j];  
            cmd_found = true;
            break;
        }

        if (!cmd_found) {
            arg_err("no such subcommand:");
            fprintf(stderr, "\e[0;36m");
            cmd__fprint_path(stderr, curr);
            fprintf(stderr, " %s", argv[i]);
            fprintf(stderr, "\e[0;0m");
            fprintf(stderr, "\n\n");

            return -1;
        }
    }

    cmd_fprint_help(stderr, curr);

    exit(0);
}

static const arg_parser cmd__help = {
    .parse = cmd__parse_help,
    .count = 0,
};

static inline cmd cmd_add_help_subcmd(cmd cmd) {
    subcmd help = cmd_subcmd(cmd, "help"); 
    cmd_help(help, "print help");
    cmd_desc(help, "display help for a subcommand");

    arg arg = cmd_arg(help, "command");
    arg_value(arg, cmd, cmd__help);

    return cmd;
}

static inline void arg_help(
    arg         arg,
    const char* help
) {
    arg->help = help;
}

static inline void arg_usage(
    arg         arg,
    const char* usage
) {
    arg->usage = usage;
}

static inline void arg_short(
    arg  arg,
    char name
) {
    arg->short_name = name;
}

static inline void arg_long(
    arg         arg,
    const char* name
) {
    arg->long_name = name;
}

static inline void arg_check(
    arg   arg,
    bool* check
) {
    arg->check = check;
}

static inline void arg_value(
    arg        arg,
    void*      data,
    arg_parser parser
) {
    arg->data   = data;
    arg->parser = parser;
}

static inline int arg_err(const char* fmt, ...) {
    va_list args;

    fprintf(stderr, "\e[0;31merror:\e[0;0m ");

    va_start(args, fmt);
    int result = vfprintf(stderr, fmt, args);
    va_end(args);

    return result;
}

static inline bool cmd_validate(const cmd cmd) {
    bool valid = true;

    for (size_t i = 0; i < cmd->args_len; i++) {
        for (size_t j = i + 1; j < cmd->args_len; j++) {
            if (strcmp(cmd->args[i]->name, cmd->args[j]->name) == 0) {
                arg_err(
                    "invalid command, duplicate arguments: `%s`\n",
                    cmd->args[i]->name
                );

                valid = false;
            }

            if (
                cmd->args[i]->short_name != '\0' && 
                cmd->args[i]->short_name == cmd->args[j]->short_name
            ) {
                arg_err(
                    "invalid command, duplicate arguments: `-%c`\n",
                    cmd->args[i]->short_name
                );

                valid = false;
            }

            if (
                cmd->args[i]->long_name != NULL && 
                cmd->args[j]->long_name != NULL && 
                strcmp(cmd->args[i]->long_name, cmd->args[j]->long_name) == 0
            ) {
                arg_err(
                    "invalid command, duplicate arguments: `--%s`\n",
                    cmd->args[i]->long_name
                );

                valid = false;
            }
        }
    }

    for (size_t i = 0; i < cmd->cmds_len; i++) {
        for (size_t j = i + 1; j < cmd->cmds_len; j++) {
            if (strcmp(cmd->cmds[i]->name, cmd->cmds[j]->name) == 0) {
                arg_err(
                    "invalid command, duplicate subcommands: `%s`\n",
                    cmd->cmds[i]->name
                );

                valid = false;
            }
        }

        valid &= cmd_validate(cmd->cmds[i]);
    }

    return valid;
}

static inline void cmd__print_try_help() {
    fprintf(stderr, "for more information, try '");
    fprintf(stderr, "\e[0;36m");
    fprintf(stderr, "--help");
    fprintf(stderr, "\e[0;0m");
    fprintf(stderr, "'.\n");
}

static inline int cmd__parse_arg(
    const arg    arg,
    int          argc,
    const char** argv
) {
    if (argc < (int) arg->parser.count) {
        arg_err("too few arguments for:\n");
        fprintf(stderr, "  ");
        fprintf(stderr, "\e[0;36m");

        if (arg__is_option(arg)) {
            if (arg->short_name != '\0') {
                fprintf(stderr, "-%c", arg->short_name);

                if (arg->long_name) {
                    fprintf(stderr, ", --%s", arg->long_name);
                }
            } else {
                fprintf(stderr, "--%s", arg->long_name);
            }

            fprintf(stderr, " ");
        }

        if (arg->usage) fprintf(stderr, "%s", arg->usage);
        else            fprintf(stderr, "<%s>", arg->name);

        fprintf(stderr, "\e[0;0m");
        fprintf(stderr, "\n\n");
        cmd__print_try_help();

        exit(0);
    }

    if (arg->check) *arg->check = true;

    if (arg->parser.parse) {
        int count = arg->parser.parse(
            arg->data,
            argc,
            &argv[0]
        );

        if (count < 0) {
            cmd__print_try_help();
            exit(0);
        }

        return count;
    }

    return arg->parser.count;
}

static inline int cmd__parse_long(
    const cmd    cmd,
    int          argc,
    const char** argv
) {
    for (size_t i = 0; i < cmd->args_len; i++) {
        arg arg = cmd->args[i];

        if (!arg->long_name)                          continue;
        if (strcmp(arg->long_name, argv[0] + 2) != 0) continue;

        return cmd__parse_arg(arg, argc - 1, argv + 1);
    }

    arg_err("no such option: `%s`\n\n", argv[0]);
    cmd_fprint_usage(stderr, cmd);
    fprintf(stderr, "\n");
    cmd__print_try_help();

    exit(0);
}

static inline int cmd__parse_short(
    const cmd    cmd,
    int          argc,
    const char** argv
) {
    if (argv[0][1] == '\0') {
        arg_err("no such option: `%s`\n\n", argv[0]);
        cmd_fprint_usage(stderr, cmd);
        fprintf(stderr, "\n");
        cmd__print_try_help();

        exit(0);
    }

    int count = 0;

    for (size_t i = 1; argv[0][i] != '\0'; i++) {
        for (size_t j = 0; j < cmd->args_len; j++) {
            arg arg = cmd->args[j];

            if (arg->short_name != argv[0][i]) continue;

            count += cmd__parse_arg(
                arg,
                argc - 1 - count,
                argv + 1 + count
            );

            goto end;
        }

        arg_err("no such option: `-%c`\n\n", argv[0][i]);
        cmd_fprint_usage(stderr, cmd);
        fprintf(stderr, "\n");
        cmd__print_try_help();

        exit(0);

        end: continue;
    }

    return count;
}

static inline void cmd_parse(
    const cmd    cmd,
    int          argc,
    const char** argv
) {
    if (!cmd->parent
        && argc == 1
        && (cmd__option_count(cmd) < cmd->args_len
            || cmd->cmds_len > 0)) {
        cmd_fprint_help(stderr, cmd);
        exit(0);
    }

    if (cmd->uenum) *cmd->uenum = cmd->value;

    // index of the current positional argument in the command
    size_t index = 0;

    for (int i = 1; i < argc; i++) {
        // parse long command if present
        if (strncmp("--", argv[i], 2) == 0) {
            i += cmd__parse_long(cmd, argc - i, &argv[i]);
            continue;
        } 

        // parse short command if present
        if (strncmp("-", argv[i], 1) == 0) {
            i += cmd__parse_short(cmd, argc - i, &argv[i]);
            continue;
        }

        // parse positional argument if expected
        bool found_positional = false;

        while (index < cmd->args_len) {
            arg arg = cmd->args[index];
            index++;

            if (arg__is_option(arg)) continue;

            i += cmd__parse_arg(arg, argc - i, argv + i) - 1;

            found_positional = true;
            break;
        }

        if (found_positional) continue;

        // parse subcommand if expected
        for (size_t j = 0; j < cmd->cmds_len; j++) {
            if (strcmp(cmd->cmds[j]->name, argv[i]) != 0) continue;
            cmd_parse(cmd->cmds[j], argc - i, &argv[i]);
            return;
        }

        // report error and exit
        arg_err("no such command: `%s`\n\n", argv[i]);
        cmd_fprint_usage(stderr, cmd);
        fprintf(stderr, "\n");
        cmd__print_try_help();

        exit(0);
    }

    // if more positional arguments are expected, report error and exit
    bool needs_arguments = false;

    while (index < cmd->args_len) {
        arg arg = cmd->args[index];
        index++;

        if (arg__is_option(arg)) continue;
        if (arg->parser.count == 0) {
            if (arg->parser.parse) {
                cmd__parse_arg(arg, 0, NULL);
            }

            continue;
        }

        if (!needs_arguments) {
            arg_err("the following arguments were not provided:\n"); 
            needs_arguments = true;
        }
        
        fprintf(stderr, "\e[0;36m");

        if (arg->usage) fprintf(stderr, "  %s\n\n", arg->usage);
        else            fprintf(stderr, "  <%s>\n\n", arg->name);

        fprintf(stderr, "\e[0;0m");

        cmd_fprint_usage(stderr, cmd);
        fprintf(stderr, "\n");
        cmd__print_try_help();
    }
    
    if (needs_arguments) exit(0);

    // if subcommand is expected, report error and exit
    if (cmd->cmds_len > 0) {
        arg_err("expected a command\n\n");
        cmd_fprint_usage(stderr, cmd);
        fprintf(stderr, "\n");
        cmd__print_try_help();

        exit(0);
    }
}

/* ----- value parsers ----- */

static int arg__parse_str(void* data, int argc, const char** argv) {
    (void) argc;

    *(const char**) data = argv[0];

    return 1;
}

static const arg_parser arg_str = {
    .parse = arg__parse_str,
    .count = 1,
};

static int arg__parse_int(void* data, int argc, const char** argv) {
    (void) argc;

    char* end;
    *(int*) data = strtol(argv[0], &end, 10);

    if (end < argv[0] + strlen(argv[0])) {
        arg_err("expected integer, found: `%s`\n", argv[0]);
        return -1;
    }

    return 1;
}

static const arg_parser arg_int = {
    .parse = arg__parse_int,
    .count = 1,
};

static int arg__parse_count(void* data, int argc, const char** argv) {
    (void) argc;
    (void) argv;

    *(int*) data += 1;

    return 0;
}

static const arg_parser arg_count = {
    .parse = arg__parse_count,
    .count = 0,
};

static int arg__parse_float(void* data, int argc, const char** argv) {
    (void) argc;

    char* end;
    *(float*) data = strtof(argv[0], &end);

    if (end < argv[0] + strlen(argv[0])) {
        arg_err("expected float, found: `%s`\n", argv[0]);
        return -1;
    }

    return 1;
}

static const arg_parser arg_float = {
    .parse = arg__parse_float,
    .count = 1,
};
