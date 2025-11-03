#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

typedef struct arg_parser arg_parser;
typedef struct arg* arg;
typedef struct cmd* cmd;

struct arg_parser {
    // Function parsing arguments, returns the number of arguments parsed.
    //
    // If error occurs, returns `-1`;
    int (*parse)(void* data, int argc, const char** argv);

    // The minimum number of required arguments.
    int   count;
};

struct arg {
    const char*       name;
    const char*       help;
    const char*       usage;
    char              short_name;
    const char*       long_name;

    // [bool] to set when argument is parsed.
    bool*             check;

    void*             data;
    arg_parser        parser;
};

struct cmd {
    const char* name;
    const char* help;
    const char* desc;

    int*        data;
    int         value;

    size_t      args_len;
    size_t      args_cap;
    arg*        args;

    size_t      cmds_len;
    size_t      cmds_cap;
    cmd*        cmds;

    cmd         parent;
};

static inline bool arg_is_option(const arg arg) {
    return arg->short_name != '\0' || arg->long_name != NULL;
}

static inline size_t cmd_option_count(
    const cmd cmd
) {
    size_t count = 0;

    for (size_t i = 0; i < cmd->args_len; i++) {
        if (arg_is_option(cmd->args[i])) {
            count += 1;
        }
    }

    return count;
}

static inline void cmd_fprint_arguments(
    FILE*     file,
    const cmd cmd
) {
    if (cmd_option_count(cmd) < cmd->args_len) {
        fprintf(file, "\n");
        fprintf(file, "\e[0;32m");
        fprintf(file, "arguments:\n");
        fprintf(file, "\e[0;0m");

        for (size_t i = 0; i < cmd->args_len; i++) {
            arg arg = cmd->args[i];

            if (arg_is_option(arg)) continue;

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

static inline void cmd_fprint_options(
    FILE*     file,
    const cmd cmd
) {
    if (cmd_option_count(cmd) > 0) {
        fprintf(file, "\n");
        fprintf(file, "\e[0;32m");
        fprintf(file, "options:\n");
        fprintf(file, "\e[0;0m");

        for (size_t i = 0; i < cmd->args_len; i++) {
            arg arg = cmd->args[i];

            if (!arg_is_option(arg)) continue;

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

static inline void cmd_fprint_commands(
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

static inline void cmd_fprint_path(
    FILE*     file,
    const cmd cmd
) {
    if (cmd->parent) {
        cmd_fprint_path(file, cmd->parent);
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

    cmd_fprint_path(file, cmd);

    for (size_t i = 0; i < cmd->args_len; i++) {
        if (arg_is_option(cmd->args[i])) continue;
        fprintf(file, " <%s>", cmd->args[i]->name);
    }

    if (cmd_option_count(cmd) > 0) {
        fprintf(file, " [options]");
    }

    if (cmd->cmds_len > 0) {
        fprintf(file, " [command]");
    }

    fprintf(file, "\e[0;0m\n");

    cmd_fprint_arguments(file, cmd);
    cmd_fprint_options(file, cmd);
    cmd_fprint_commands(file, cmd);
}
 
static inline void cmd_fprint_help(
    FILE*     file,
    const cmd cmd
) {
    if (cmd->desc) {
        fprintf(file, "%s\n\n", cmd->desc);
    }

    cmd_fprint_usage(file, cmd);
}

static int arg_parse_help(void* data, int argc, const char** argv) {
    (void) argc;
    (void) argv;

    cmd_fprint_help(stderr, data);
    exit(0);
}

static inline cmd cmd_new(
    const char* name
) {
    cmd cmd = malloc(sizeof *cmd);

    cmd->name     = name;
    cmd->help     = NULL;
    cmd->desc     = NULL;

    cmd->data     = NULL;
    cmd->value    = 0;

    cmd->args_len = 1;
    cmd->args_cap = 1;
    cmd->args     = malloc(sizeof *cmd->args);

    *cmd->args = malloc(sizeof **cmd->args);
    **cmd->args = (struct arg) {
        .name       = "help" ,
        .help       = "print help",
        .usage      = NULL,
        .short_name = 'h',
        .long_name  = "help",

        .check     = NULL,

        .data       = cmd,
        .parser     = {
            .parse = arg_parse_help,
            .count = 0,
        },
    };

    cmd->cmds_len = 0;
    cmd->cmds_cap = 0;
    cmd->cmds     = NULL;
    cmd->parent   = NULL;

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

static inline void cmd_value(
    cmd   cmd,
    void* data,
    int   value
) {
    cmd->data  = data;
    cmd->value = value;
}

static inline cmd cmd_subcmd(
    cmd         cmd,
    const char* name
) { 
    if (cmd->cmds_len == cmd->cmds_cap) {
        if (cmd->cmds_cap == 0) cmd->cmds_cap  = 1;
        else                    cmd->cmds_cap *= 2;

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

    arg->check     = NULL;

    arg->data       = NULL;
    arg->parser     = (arg_parser){0};

    cmd->args_len++;

    return arg;
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

static inline int cmd_parse_arg(
    const cmd    cmd,
    const arg    arg,
    int          argc,
    const char** argv
) {
    if (argc < (int) arg->parser.count) {
        arg_err("too few arguments, expected:\n");
        fprintf(stderr, "  ");
        fprintf(stderr, "\e[0;36m");

        if (arg_is_option(arg)) {
            fprintf(stderr, "%s", argv[-1]);
            if (arg->usage) fprintf(stderr, " %s", arg->usage);
        } else {
            if (arg->usage) fprintf(stderr, "%s", arg->usage);
            else            fprintf(stderr, "<%s>", arg->name);
        }

        fprintf(stderr, "\e[0;0m");
        fprintf(stderr, "\n\n");

        cmd_fprint_usage(stderr, cmd);
        exit(0);
    }

    if (arg->check) *arg->check = true;

    if (arg->parser.parse) {
        int count = arg->parser.parse(
            arg->data,
            (int) arg->parser.count,
            &argv[0]
        );

        if (count < 0) {
            fprintf(stderr, "\n");
            cmd_fprint_usage(stderr, cmd);
            exit(0);
        }

        return count;
    }

    return arg->parser.count;
}

static inline int cmd_parse_long(
    const cmd    cmd,
    int          argc,
    const char** argv
) {
    for (size_t i = 0; i < cmd->args_len; i++) {
        arg arg = cmd->args[i];

        if (!arg->long_name)                          continue;
        if (strcmp(arg->long_name, argv[0] + 2) != 0) continue;

        return cmd_parse_arg(cmd, arg, argc - 1, argv + 1);
    }

    arg_err("no such option: `%s`\n\n", argv[0]);
    cmd_fprint_usage(stderr, cmd);
    exit(0);
}

static inline int cmd_parse_short(
    const cmd    cmd,
    int          argc,
    const char** argv
) {
    if (argv[0][1] == '\0') {
        arg_err("no such option: `%s`\n\n", argv[0]);
        cmd_fprint_usage(stderr, cmd);
        exit(0);
    }

    int count = 0;

    for (size_t i = 1; argv[0][i] != '\0'; i++) {
        for (size_t j = 0; j < cmd->args_len; j++) {
            arg arg = cmd->args[j];

            if (arg->short_name != argv[0][i]) continue;

            count += cmd_parse_arg(
                cmd,
                arg,
                argc - 1 - count,
                argv + 1 + count
            );

            goto end;
        }

        arg_err("no such option: `%s`\n\n", argv[0]);
        cmd_fprint_usage(stderr, cmd);
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
    if (cmd->data) *cmd->data = cmd->value;

    // index of the current positional argument in the command
    size_t index = 0;

    for (int i = 1; i < argc; i++) {
        // parse long command if present
        if (strncmp("--", argv[i], 2) == 0) {
            i += cmd_parse_long(cmd, argc - i, &argv[i]);
            continue;
        } 

        // parse short command if present
        if (strncmp("-", argv[i], 1) == 0) {
            i += cmd_parse_short(cmd, argc - i, &argv[i]);
            continue;
        }

        // parse positional argument if expected
        bool found_positional = false;

        while (index < cmd->args_len) {
            arg arg = cmd->args[index];
            index++;

            if (arg_is_option(arg)) continue;

            i += cmd_parse_arg(cmd, arg, argc - i, argv + i) - 1;

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
        exit(0);
    }

    // if more positional arguments are expected, report error and exit
    while (index < cmd->args_len) {
        arg arg = cmd->args[index];
        index++;

        if (arg_is_option(arg)) continue;

        arg_err("the following arguments were not provided:\n"); 
        fprintf(stderr, "\e[0;36m");

        if (arg->usage) fprintf(stderr, "  %s\n\n", arg->usage);
        else            fprintf(stderr, "  <%s>\n\n", arg->name);

        fprintf(stderr, "\e[0;0m");

        cmd_fprint_usage(stderr, cmd);
        exit(0);
    }

    // if subcommand is expected, report error and exit
    if (cmd->cmds_len > 0) {
        cmd_fprint_help(stderr, cmd);
        exit(0);
    }
}

static int arg_parse_str(void* data, int argc, const char** argv) {
    (void) argc;

    *(const char**) data = argv[0];

    return 1;
}

static const arg_parser arg_str = {
    .parse = arg_parse_str,
    .count = 1,
};

static int arg_parse_int(void* data, int argc, const char** argv) {
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
    .parse = arg_parse_int,
    .count = 1,
};

static int arg_parse_count(void* data, int argc, const char** argv) {
    (void) argc;
    (void) argv;

    *(int*) data += 1;

    return 0;
}

static const arg_parser arg_count = {
    .parse = arg_parse_count,
    .count = 0,
};

static int arg_parse_float(void* data, int argc, const char** argv) {
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
    .parse = arg_parse_float,
    .count = 1,
};
