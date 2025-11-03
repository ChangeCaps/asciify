#include "download.h"

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

void free_image_data(image_data* data) {
    free(data->data);
}

static size_t image_write_callback(
    void*  contents,
    size_t size,
    size_t nmemb,
    void*  user_data
) {
    size_t      total = size * nmemb;
    image_data* image = user_data;
    image->data       = realloc(image->data, image->size + total);

    memcpy(image->data + image->size, contents, total);

    image->size += total;

    return total;
}

bool download_image(image_data* data, const char* url) {
    *data = (image_data) {0};

    CURL* curl = curl_easy_init();

    if (!curl) return false;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, image_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);

    CURLcode status = curl_easy_perform(curl);

    curl_easy_cleanup(curl);

    return status == CURLE_OK;
}

static size_t search_write_callback(
    void*  contents,
    size_t size,
    size_t nmemb,
    void*  user_data
) {
    size_t total    = size * nmemb;
    char** response = user_data;
    *response       = realloc(*response, strlen(*response) + total + 1);

    strncat(*response, contents, total);

    return total;
}

bool search_images(
    size_t*     url_count,
    char***     urls,
    int         offset,
    const char* search_term
) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    char* escaped = curl_easy_escape(curl, search_term, 0);

    if (!escaped) {
        curl_easy_cleanup(curl);
        return false;
    }

    size_t len = snprintf(
        NULL,
        0,
        "https://www.google.com/"
        "search?tbm=isch&safe=off&start=%i&q=%s",
        offset,
        escaped
    );

    char* url = malloc(len + 1);

    snprintf(
        url,
        len + 1,
        "https://www.google.com/"
        "search?tbm=isch&safe=off&start=%i&q=%s",
        offset,
        escaped
    );

    char* response = calloc(1, 1);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, search_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode result = curl_easy_perform(curl);

    curl_easy_cleanup(curl);
    free(escaped);
    free(url);

    if (result != CURLE_OK) return false;

    *url_count = 0;
    *urls      = NULL;

    const char* haystack = response;
    const char* needle   = "<img class=\"DS1iW\" alt=\"\" src=\"";
    while ((haystack = strstr(haystack, needle))) {
        haystack += strlen(needle);

        char* end = strstr(haystack, "\"");
        if (!end) return 1;

        char* url = NULL;
        url       = malloc(end - haystack + 1);
        memcpy(url, haystack, end - haystack);
        url[end - haystack] = '\0';

        *url_count += 1;
        *urls                   = realloc(*urls, (sizeof *urls) * *url_count);
        (*urls)[*url_count - 1] = url;
    }

    free(response);

    return true;
}
