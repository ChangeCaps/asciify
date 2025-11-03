#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct image_data {
    size_t   size;
    uint8_t* data;
} image_data;

void free_image_data(image_data* data);

bool download_image(image_data* data, const char* url);
bool search_images(
    size_t*     url_count,
    char***     urls,
    int         offset,
    const char* search_term
);
