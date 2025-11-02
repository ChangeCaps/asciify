#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb/stb_image_resize.h>

#include <curl/curl.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "arg.h"

struct opts {
    char* input;
    int   width;
    int   height;

    bool  has_width;
    bool  has_height;

    bool  color;
};

struct opts parse_opts(int argc, const char** argv) {
    struct opts opts = {0};
    opts.width = 100;
    opts.height = 50;

    cmd main = cmd_new("asciify");
    cmd_help(main, "Asciify");

    arg input = cmd_arg(main, "input");
    arg_help (input, "search term");
    arg_value(input, &opts.input, arg_str);

    arg width = cmd_arg(main, "width");
    arg_help (width, "width of output image");
    arg_usage(width, "<WIDTH>");
    arg_long (width, "width");
    arg_check(width, &opts.has_width);
    arg_value(width, &opts.width, arg_int);

    arg height = cmd_arg(main, "height");
    arg_help (height, "height of output image");
    arg_usage(height, "<HEIGHT>");
    arg_long (height, "height");
    arg_check(height, &opts.has_height);
    arg_value(height, &opts.height, arg_int);

    arg color = cmd_arg(main, "color");
    arg_help (color, "enable color rendering");
    arg_long (color, "color");
    arg_check(color, &opts.color);

    cmd_parse(main, argc, argv);

    return opts;
}

const char table[] = " .-=+*x#$&X@";
const char edges[] = "|/-\\|/-\\";
const int colors[] = { 31, 33, 32, 36, 34, 35 };
const int colors_high[] = { 91, 93, 92, 96, 94, 95 };

typedef struct pixel {
    float r;
    float g;
    float b;
    float a;
} pixel;

static pixel read_pixel(
    const uint8_t* image,
    int            x,
    int            y,
    int            w
) {
    float r = (float) image[(y * w + x) * 4 + 0] / 255.0;
    float g = (float) image[(y * w + x) * 4 + 1] / 255.0;
    float b = (float) image[(y * w + x) * 4 + 2] / 255.0;
    float a = (float) image[(y * w + x) * 4 + 3] / 255.0;

    return (pixel) {r, g, b, a};
}

static float read_lightness(
    const uint8_t* image,
    int            x,
    int            y,
    int            w
) {
    pixel p = read_pixel(image, x, y, w);

    return 0.2126 * p.r + 0.7152 * p.g + 0.0722 * p.b;
}

struct image_data {
    size_t   size;
    uint8_t* data;
};

static size_t search_write_callback(
    void*  contents,
    size_t size,
    size_t nmemb,
    void*  user_data
) {
    size_t total = size * nmemb;
    char** response = user_data;

    *response = realloc(*response, strlen(*response) + total + 1);

    strncat(*response, contents, total);

    return total;
}

static size_t image_write_callback(
    void*  contents,
    size_t size,
    size_t nmemb,
    void*  user_data
) {
    size_t total = size * nmemb;
    struct image_data* image = user_data;

    image->data = realloc(image->data, image->size + total);

    memcpy(
        image->data + image->size,
        contents,
        total
    );

    image->size += total;

    return total;
}

int main(int argc, const char** argv) {
    struct opts opts = parse_opts(argc, argv);

    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL* curl = curl_easy_init();
    char* response = calloc(1, 1);

    size_t len = snprintf(
        NULL,
        0, 
        "https://www.google.com/search?tbm=isch&q=%s",
        opts.input
    );

    char* url = malloc(len + 1);

    snprintf(
        url,
        len + 1,
        "https://www.google.com/search?tbm=isch&q=%s",
        opts.input
    );

    if (!curl) return 1;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, search_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    if (curl_easy_perform(curl) != CURLE_OK) {
        printf("image search failed");
        return 1;
    }

    curl_easy_cleanup(curl);

    char* image_url = NULL;
    const char* needle = "<img class=\"DS1iW\" alt=\"\" src=\"";
    while ((response = strstr(response, needle))) {
        response += strlen(needle);

        char* end = strstr(response, "\"");
        if (!end) return 1;

        image_url = malloc(end - response + 1);
        memcpy(image_url, response, end - response);
        image_url[end - response] = '\0';

        break;
    }

    curl = curl_easy_init();

    struct image_data image_data = {0};

    curl_easy_setopt(curl, CURLOPT_URL, image_url);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, image_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &image_data);

    if (curl_easy_perform(curl) != CURLE_OK) {
        printf("image download failed");
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

    if (!opts.has_width) {
        float aspect = (float) width / (float) height * 2.0;
        opts.width = (int) floorf((float) opts.height * aspect);
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

    for (int y = 0; y < opts.height; y++) {
        for (int x = 0; x < opts.width; x++) {
            pixel p = read_pixel(scaled, x, y, opts.width);

            float l11 = read_lightness(scaled, x, y, opts.width);

            int idx = floorf(l11 * (float) ((sizeof table) - 2));

            if (opts.color) {
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

            if (x > 0 && x < opts.width - 1 && y > 1 && y < opts.height - 1) {
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
            
            printf("%c", table[idx]);
        }

        printf("\n");
    }

    stbi_image_free(image);
    free(scaled);
}
