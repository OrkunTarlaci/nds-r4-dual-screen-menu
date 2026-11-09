// PNG cozucu: stb_image (public domain / MIT, Sean Barrett) - yalnizca PNG
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_NO_THREAD_LOCALS
#define STBI_MAX_DIMENSIONS 2048
#include "stb_image.h"

// DS'in 4 MB RAM'i icin sinir: RGBA olarak en fazla ~2.4 MB
#define MAX_PIXELS (780 * 780)

unsigned char *macro_png_load(const char *path, int *w, int *h, const char **err)
{
    int comp;
    if (!stbi_info(path, w, h, &comp)) {
        *err = "PNG okunamadi";
        return NULL;
    }
    if ((long)(*w) * (*h) > MAX_PIXELS) {
        *err = "Resim cok buyuk";
        return NULL;
    }
    unsigned char *p = stbi_load(path, w, h, &comp, 4);
    if (!p) *err = stbi_failure_reason();
    return p;
}

void macro_png_free(unsigned char *p)
{
    stbi_image_free(p);
}
