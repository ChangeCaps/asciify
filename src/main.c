#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb/stb_image_resize.h>

#include <curl/curl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "xterm.h"
#include "download.h"
#include "opts.h"

const char *const tables[] = {
    " .:-=+*#%@",
    " _.,-=+:;cba!?0123456789$W#@",
    " '`^\",:;Il!i><~+_-?][}{1)(|\\//tfjrxnuvczXYUKCLQ0OZmwqpdbkhao*#MW&8%B@$",
};

const char edges[] = "|/-\\|/-\\";
const int colors[] = { 31, 33, 32, 36, 34, 35 };
const int colors_high[] = { 91, 93, 92, 96, 94, 95 };

typedef struct bytes {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} bytes;

typedef struct pixel {
    float r;
    float g;
    float b;
    float a;
} pixel;

static bytes read_bytes(
    const uint8_t* image,
    int            x,
    int            y,
    int            w
) {
    uint8_t r = image[(y * w + x) * 4 + 0];
    uint8_t g = image[(y * w + x) * 4 + 1];
    uint8_t b = image[(y * w + x) * 4 + 2];
    uint8_t a = image[(y * w + x) * 4 + 3];

    return (bytes) {r, g, b, a};
}

static pixel read_pixel(
    const uint8_t* image,
    int            x,
    int            y,
    int            w
) {
    bytes bytes = read_bytes(image, x, y, w);
    return (pixel) {
        .r = bytes.r / 255.0,
        .g = bytes.g / 255.0,
        .b = bytes.b / 255.0,
        .a = bytes.a / 255.0,
    };
}

#define MIN(a, b) ((a) <= (b) ? (a) : (b))
#define MAX(a, b) ((a) >= (b) ? (a) : (b))
#define CLAMP(x, min, max) MIN(MAX(x, min), max)

static float read_lightness(
    const uint8_t* image,
    int            x,
    int            y,
    int            w
) {
    pixel p = read_pixel(image, x, y, w);
    float l = 0.2126 * p.r + 0.7152 * p.g + 0.0722 * p.b;

    return CLAMP(l, 0.0, 1.0);
}

static int run(struct opts opts) {
    size_t urlc;
    char** urls;

    if (!search_images(&urlc, &urls, opts.offset, opts.input)) {
        printf("search failed\n");
        return 1;
    }

    srand(time(NULL));

    size_t idx = rand() % urlc;

    image_data image_data;

    if (!download_image(&image_data, urls[idx])) {
        printf("download failed\n");
        return 1;
    }

    int width, height, channels;
    uint8_t* image = stbi_load_from_memory(
        image_data.data,
        image_data.size,
        &width,
        &height,
        &channels,
        4
    );

    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    if (!opts.has_width && !opts.has_height) { 
        float aspect = (float) width / (float) height * 2.0;

        // fit on screen
        w.ws_row -= 2;

        if ((float) w.ws_col / aspect > (float) w.ws_row) {
            opts.width  = (int) floorf((float) w.ws_row * aspect);
            opts.height = w.ws_row;
        } else {
            opts.width  = w.ws_col;
            opts.height = (int) floorf((float) w.ws_col / aspect);
        }
    }

    if (!opts.has_width && opts.has_height) {
        float aspect = (float) width / (float) height * 2.0;
        opts.width = (int) floorf((float) opts.height * aspect);
    }

    if (opts.has_width && !opts.has_height) {
        float aspect = (float) width / (float) height * 2.0;
        opts.height = (int) floorf((float) opts.width / aspect);
    }

    uint8_t* scaled = malloc(opts.width * opts.height * 4);

    stbir_resize_uint8(
        image,
        width,
        height,
        width * 4,
        scaled,
        opts.width,
        opts.height,
        opts.width * 4,
        4
    );

    if (opts.center) {
        for (int i = 0; i < (w.ws_row - opts.height) / 2; i++) {
            printf("\n");
        }
    }

    printf("\n");

    for (int y = 0; y < opts.height; y++) {
        if (opts.center) {
            for (int i = 0; i < (w.ws_col - opts.width) / 2; i++) {
                printf(" ");
            }
        }

        for (int x = 0; x < opts.width; x++) {
            pixel p = read_pixel(scaled, x, y, opts.width);

            if (opts.has_quant) {
                p.r = roundf(p.r * opts.quant) / opts.quant;
                p.g = roundf(p.g * opts.quant) / opts.quant;
                p.b = roundf(p.b * opts.quant) / opts.quant;
                p.a = roundf(p.a * opts.quant) / opts.quant;
            }

            if (opts.xterm) {
                bytes bytes = {
                    .r = roundf(p.r * 255.0),
                    .g = roundf(p.g * 255.0),
                    .b = roundf(p.b * 255.0),
                    .a = roundf(p.a * 255.0),
                };

                uint8_t index = rgb_to_xterm(bytes.r, bytes.g, bytes.b);

                printf("\e[38;5;%hhim", index);
            } if (opts.ansi) {
                float cmax = fmaxf(fmaxf(p.r, p.g), p.b);
                float cmin = fminf(fminf(p.r, p.g), p.b);
                float dc   = (cmax - cmin) / 2.0;
                float h;

                float l = (cmax + cmin) / 2.0;
                float s = l < 0.5 ? dc / (cmax + cmin) : dc / (2.0 - cmax - cmin);

                if (s > 0.1) {
                    if (cmax == p.r) {
                        h = fmodf((p.g - p.b) / dc, 6.0);
                    } else if (cmax == p.g) {
                        h = (p.b - p.r) / dc + 2.0;
                    } else {
                        h = (p.r - p.g) / dc + 4.0;
                    }

                    int index = (int) roundf(h + 5.5) % 6;
                    int color = l > 0.7 ? colors_high[index] : colors[index];

                    printf("\e[0;%im", color);
                } else {
                    printf("\e[0;0m");
                }
            }

            float l11 = read_lightness(scaled, x, y, opts.width);

            if (x > 0 && x < opts.width - 1 && y > 1 && y < opts.height - 1 && opts.edge) {
                float l00 = read_lightness(scaled, x - 1, y - 1, opts.width);
                float l10 = read_lightness(scaled, x    , y - 1, opts.width);
                float l20 = read_lightness(scaled, x + 1, y - 1, opts.width);
                float l01 = read_lightness(scaled, x - 1, y    , opts.width);
                float l21 = read_lightness(scaled, x + 1, y    , opts.width);
                float l02 = read_lightness(scaled, x - 1, y + 1, opts.width);
                float l12 = read_lightness(scaled, x    , y + 1, opts.width);
                float l22 = read_lightness(scaled, x + 1, y + 1, opts.width);

                float dx = -1.0 * l00 + 1.0 * l20 + 
                           -2.0 * l01 + 2.0 * l21 + 
                           -1.0 * l02 + 1.0 * l22;

                float dy = -1.0 * l00 + -2.0 * l10 + -1.0 * l20 +
                            1.0 * l02 +  2.0 * l12 +  1.0 * l22;

                float d = sqrtf(dx * dx + dy * dy);
                float o = atan2(dy, dx);

                if (d > 0.9) {
                    printf("%c", edges[(int) round((o / 3.14159 * 3.5 + 8.0)) % 8]);
                    continue;
                }
            }

            int idx = floorf(l11 * (float) (strlen(tables[opts.detail]) - 1));
            
            printf("%c", tables[opts.detail][idx]);
        }

        printf("\n");
    }

    if (opts.center) {
        for (int i = 0; i < (w.ws_row - opts.height) / 2; i++) {
            printf("\n");
        }
    }

    stbi_image_free(image);
    free(scaled);

    return 0;
}

int main(int argc, const char** argv) {
    struct opts opts = parse_opts(argc, argv);

    curl_global_init(CURL_GLOBAL_DEFAULT);

    int result = 0;

    if (opts.has_watch) {
        while (result == 0) {
            result = run(opts);

            sleep(opts.watch);
        }  
    } else {
        result = run(opts);
    }

    curl_global_cleanup();

    return result;
}
