#pragma once

#include "arg.h"

struct opts {
    char* input;

    int   offset;

    int   watch;
    bool  has_watch;

    int   width;
    bool  has_width;

    int   height;
    bool  has_height;

    bool  center;

    bool  ansi;
    bool  xterm;

    enum {
        DETAIL_LOW  = 0,
        DETAIL_MID  = 1,
        DETAIL_HIGH = 2,
    } detail;

    int  quant;
    bool has_quant;
};

static int parse_detail(void* data, int argc, const char** argv) {
    (void) argc;

    if (strcmp(argv[0], "low") == 0) {
        *(int*) data = DETAIL_LOW;
        return 1;
    } else if (strcmp(argv[0], "mid") == 0) {
        *(int*) data = DETAIL_MID;
        return 1;
    } else if (strcmp(argv[0], "high") == 0) {
        *(int*) data = DETAIL_HIGH;
        return 1;
    } else {
        fprintf(stderr, "invalid level of detail `%s`\n", argv[0]);

        return -1;
    }
}

struct opts parse_opts(int argc, const char** argv) {
    struct opts opts = {0};
    opts.width = 100;
    opts.height = 50;
    opts.detail = DETAIL_MID;

    cmd main = cmd_new("asciify");
    cmd_desc(
        main,
        "Asciify\n\n"
        "search for an image and display it as ASCII art."
    );

    arg input = cmd_arg(main, "input");
    arg_help (input, "search term");
    arg_value(input, &opts.input, arg_str);

    arg offset = cmd_arg(main, "offset");
    arg_help (offset, "search offset");
    arg_usage(offset, "<offset>");
    arg_long (offset, "offset");
    arg_short(offset, 'o');
    arg_value(offset, &opts.offset, arg_int);

    arg watch = cmd_arg(main, "watch");
    arg_help (watch, "repeatedly run command");
    arg_usage(watch, "<seconds>");
    arg_long (watch, "watch");
    arg_short(watch, 'w');
    arg_check(watch, &opts.has_watch);
    arg_value(watch, &opts.watch, arg_int);

    arg width = cmd_arg(main, "width");
    arg_help (width, "width of output image");
    arg_usage(width, "<width>");
    arg_long (width, "width");
    arg_check(width, &opts.has_width);
    arg_value(width, &opts.width, arg_int);

    arg height = cmd_arg(main, "height");
    arg_help (height, "height of output image");
    arg_usage(height, "<height>");
    arg_long (height, "height");
    arg_check(height, &opts.has_height);
    arg_value(height, &opts.height, arg_int);

    arg center = cmd_arg(main, "center");
    arg_help (center, "center the image");
    arg_long (center, "center");
    arg_short(center, 'c');
    arg_check(center, &opts.center);

    arg detail = cmd_arg(main, "detail");
    arg_help (detail, "level of detail in character set");
    arg_usage(detail, "<low|mid|high>");
    arg_long (detail, "detail");
    arg_short(detail, 'd');
    arg_value(
        detail,
        &opts.detail,
        (arg_parser){
            .parse = parse_detail,
            .count = 1,
        }
    );

    arg ansi = cmd_arg(main, "color");
    arg_help (ansi, "enable ansi colors");
    arg_long (ansi, "ansi");
    arg_short(ansi, 'a');
    arg_check(ansi, &opts.ansi);

    arg xterm = cmd_arg(main, "xterm");
    arg_help (xterm, "enable xterm colors");
    arg_long (xterm, "xterm");
    arg_short(xterm, 'x');
    arg_check(xterm, &opts.xterm);

    arg quant = cmd_arg(main, "quantize");
    arg_help (quant, "quantize colors");
    arg_usage(quant, "<count>");
    arg_long (quant, "quantize");
    arg_short(quant, 'q');
    arg_check(quant, &opts.has_quant);
    arg_value(quant, &opts.quant, arg_int);

    cmd_parse(main, argc, argv);

    return opts;
}
