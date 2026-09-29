/* Image -> ANSI, see include/imgimport.h. */
#include "../include/imgimport.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_ONLY_GIF
#define STBI_ONLY_BMP
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#define STBI_MAX_DIMENSIONS 8192
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
#include "../third_party/stb/stb_image.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#define MAX_PIXELS (40L * 1000 * 1000)

static const int PAL[16][3] = {
    { 0, 0, 0 }, { 0, 0, 170 }, { 0, 170, 0 }, { 0, 170, 170 }, { 170, 0, 0 }, { 170, 0, 170 },
    { 170, 85, 0 }, { 170, 170, 170 }, { 85, 85, 85 }, { 85, 85, 255 }, { 85, 255, 85 },
    { 85, 255, 255 }, { 255, 85, 85 }, { 255, 85, 255 }, { 255, 255, 85 }, { 255, 255, 255 }
};

int ad_image_name(const char *name) {
    static const char *const EXT[] = { ".png", ".jpg", ".jpeg", ".gif", ".bmp" };
    size_t n = strlen(name), i, j;
    for (i = 0; i < sizeof(EXT) / sizeof(EXT[0]); i++) {
        size_t e = strlen(EXT[i]);
        if (n <= e) continue;
        for (j = 0; j < e && tolower((unsigned char)name[n - e + j]) == EXT[i][j]; j++) {}
        if (j == e) return 1;
    }
    return 0;
}

/* Nearest palette color (weighted RGB: green counts most). */
static int nearest(float r, float g, float b, int max) {
    int i, best = 0;
    float bd = 1e30f;
    for (i = 0; i < max; i++) {
        float dr = r - PAL[i][0], dg = g - PAL[i][1], db = b - PAL[i][2];
        float d = 2 * dr * dr + 4 * dg * dg + 3 * db * db;
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}

int ad_image_to_canvas(const unsigned char *data, size_t len, int cols, int ice, int dither,
                       AdCanvas *out, int *img_w, int *img_h, char *err, size_t errsz) {
    int w, h, comp, x, y, pw, ph, rows;
    unsigned char *px;
    float *buf;          /* pw x ph RGB, scaled, as floats (for the error diffusion) */
    int *idx;

    if (len > 0x7fffffff || !stbi_info_from_memory(data, (int)len, &w, &h, &comp)) {
        /* stb tries each format in turn, so the reason is the last one
           tried's: one message covers "not an image" and "too big" */
        snprintf(err, errsz, "not an image ANetDRAW can read (PNG, JPEG, GIF or BMP, up to 8192x8192)");
        return 0;
    }
    if (w < 1 || h < 1 || w > 8192 || h > 8192 || (long)w * h > MAX_PIXELS) {
        snprintf(err, errsz, "the image is too big (%dx%d)", w, h);
        return 0;
    }
    px = stbi_load_from_memory(data, (int)len, &w, &h, &comp, 3);
    if (!px) {
        snprintf(err, errsz, "can't read the image (%s)", stbi_failure_reason());
        return 0;
    }
    if (img_w) *img_w = w;
    if (img_h) *img_h = h;
    if (cols < 1) cols = 1;
    if (cols > AD_CANVAS_MAX_W) cols = AD_CANVAS_MAX_W;
    /* half-block pixels are square: height follows the aspect ratio */
    pw = cols;
    ph = (int)((long)h * pw / w);
    if (ph < 2) ph = 2;
    if (ph > AD_CANVAS_MAX_H * 2) ph = AD_CANVAS_MAX_H * 2;
    ph += ph & 1;
    rows = ph / 2;

    buf = (float *)malloc((size_t)pw * ph * 3 * sizeof(float));
    idx = (int *)malloc((size_t)pw * ph * sizeof(int));
    if (!buf || !idx) {
        free(buf);
        free(idx);
        stbi_image_free(px);
        snprintf(err, errsz, "out of memory");
        return 0;
    }
    /* box-filter downscale (or nearest when enlarging) */
    for (y = 0; y < ph; y++) {
        int sy0 = (int)((long)y * h / ph), sy1 = (int)((long)(y + 1) * h / ph);
        if (sy1 <= sy0) sy1 = sy0 + 1;
        if (sy1 > h) sy1 = h;
        for (x = 0; x < pw; x++) {
            int sx0 = (int)((long)x * w / pw), sx1 = (int)((long)(x + 1) * w / pw), sx, sy;
            long r = 0, g = 0, b = 0, n = 0;
            if (sx1 <= sx0) sx1 = sx0 + 1;
            if (sx1 > w) sx1 = w;
            for (sy = sy0; sy < sy1; sy++)
                for (sx = sx0; sx < sx1; sx++) {
                    const unsigned char *p = px + ((size_t)sy * w + sx) * 3;
                    r += p[0]; g += p[1]; b += p[2]; n++;
                }
            buf[((size_t)y * pw + x) * 3 + 0] = (float)r / n;
            buf[((size_t)y * pw + x) * 3 + 1] = (float)g / n;
            buf[((size_t)y * pw + x) * 3 + 2] = (float)b / n;
        }
    }
    stbi_image_free(px);

    /* to the 16 colors, spreading the error (Floyd-Steinberg) if asked */
    for (y = 0; y < ph; y++)
        for (x = 0; x < pw; x++) {
            float *p = buf + ((size_t)y * pw + x) * 3;
            int c = nearest(p[0], p[1], p[2], 16), k;
            idx[(size_t)y * pw + x] = c;
            if (!dither) continue;
            for (k = 0; k < 3; k++) {
                float e = p[k] - (float)PAL[c][k];
                if (x + 1 < pw) p[3 + k] += e * 7 / 16;
                if (y + 1 < ph) {
                    float *q = buf + ((size_t)(y + 1) * pw + x) * 3;
                    if (x > 0) q[-3 + k] += e * 3 / 16;
                    q[k] += e * 5 / 16;
                    if (x + 1 < pw) q[3 + k] += e * 1 / 16;
                }
            }
        }
    free(buf);

    if (!ad_canvas_resize(out, cols, rows)) {
        free(idx);
        snprintf(err, errsz, "out of memory");
        return 0;
    }
    ad_canvas_clear(out);
    for (y = 0; y < rows; y++)
        for (x = 0; x < cols; x++) {
            int top = idx[(size_t)(y * 2) * pw + x], bot = idx[(size_t)(y * 2 + 1) * pw + x];
            unsigned char ch, attr;
            if (top == bot) {
                ch = 0xDB;
                attr = (unsigned char)((top) | ((ice ? top : top & 7) << 4));
                if (!ice && top >= 8) attr = (unsigned char)top;  /* full block, black behind */
            } else if (ice || bot < 8) {
                ch = 0xDF;                                   /* top in fg, bottom in bg */
                attr = (unsigned char)(top | bot << 4);
            } else if (top < 8) {
                ch = 0xDC;                                   /* bright bottom goes in fg */
                attr = (unsigned char)(bot | top << 4);
            } else {
                /* both bright and no iCE: keep the top, darken the bottom */
                ch = 0xDF;
                attr = (unsigned char)(top | nearest((float)PAL[bot][0], (float)PAL[bot][1], (float)PAL[bot][2], 8) << 4);
            }
            ad_canvas_set(out, x, y, ch, attr);
        }
    free(idx);
    out->h = rows;
    out->fixed = 1;
    out->ice = ice;
    return 1;
}
