/* Image import: exact cells for a two-color picture, the no-iCE fallback,
 * JPEG, dithering mixes colors, oversize/garbage rejected, fuzzed input.
 *
 *   cc -O1 -g -fsanitize=address,undefined -o test_imgimport tests/test_imgimport.c \
 *      src/imgimport.c src/canvas.c -lm
 */
#include "../include/imgimport.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WRITE_NO_STDIO
#include "../third_party/stb/stb_image_write.h"

static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } \
                           else { printf("PASS: " __VA_ARGS__); printf("\n"); } } while (0)

typedef struct { unsigned char *p; size_t n; } Mem;
static void mem_out(void *ctx, void *data, int size) {
    Mem *m = (Mem *)ctx;
    m->p = (unsigned char *)realloc(m->p, m->n + (size_t)size);
    memcpy(m->p + m->n, data, (size_t)size);
    m->n += (size_t)size;
}

/* 160x100: red (255,85,85) over blue (0,0,170), split at y = 50 */
static unsigned char *two_color(int *w, int *h) {
    unsigned char *px = (unsigned char *)malloc(160 * 100 * 3);
    int x, y;
    for (y = 0; y < 100; y++)
        for (x = 0; x < 160; x++) {
            unsigned char *p = px + (y * 160 + x) * 3;
            if (y < 50) { p[0] = 255; p[1] = 85; p[2] = 85; }
            else { p[0] = 0; p[1] = 0; p[2] = 170; }
        }
    *w = 160;
    *h = 100;
    return px;
}

int main(void) {
    AdCanvas c;
    char err[120];
    int w, h, iw, ih, i;
    unsigned char *px = two_color(&w, &h);
    Mem png = { NULL, 0 }, jpg = { NULL, 0 };
    stbi_write_png_to_func(mem_out, &png, w, h, 3, px, w * 3);
    stbi_write_jpg_to_func(mem_out, &jpg, w, h, 3, px, 95);
    ad_canvas_init(&c, 80, 25);

    CHECK(ad_image_to_canvas(png.p, png.n, 80, 1, 0, &c, &iw, &ih, err, sizeof(err)) && c.w == 80 && c.h == 25 &&
          iw == 160 && ih == 100, "a 160x100 PNG becomes 80x25 (square half-block pixels): %dx%d", c.w, c.h);
    {
        AdCell a = ad_canvas_get(&c, 10, 5), b = ad_canvas_get(&c, 10, 20), m = ad_canvas_get(&c, 10, 12);
        CHECK(a.ch == 0xDB && AD_ATTR_FG(a.attr) == 12, "red area: full blocks in bright red (%02x/%02x)", a.ch, a.attr);
        CHECK(b.ch == 0xDB && AD_ATTR_FG(b.attr) == 1, "blue area: full blocks in blue (%02x/%02x)", b.ch, b.attr);
        CHECK(m.ch == 0xDF && AD_ATTR_FG(m.attr) == 12 && AD_ATTR_BG(m.attr) == 1,
              "the edge row: upper half red over blue (%02x/%02x)", m.ch, m.attr);
    }
    /* flip it: blue over red with iCE off -> bright bottom goes to a lower-half block */
    {
        unsigned char *fl = (unsigned char *)malloc(160 * 100 * 3);
        Mem p2 = { NULL, 0 };
        AdCell m;
        for (i = 0; i < 100; i++) memcpy(fl + i * 160 * 3, px + (99 - i) * 160 * 3, 160 * 3);
        stbi_write_png_to_func(mem_out, &p2, w, h, 3, fl, w * 3);
        CHECK(ad_image_to_canvas(p2.p, p2.n, 80, 0, 0, &c, NULL, NULL, err, sizeof(err)), "no-iCE import works");
        m = ad_canvas_get(&c, 10, 12);
        CHECK(m.ch == 0xDC && AD_ATTR_FG(m.attr) == 12 && AD_ATTR_BG(m.attr) == 1 && !c.ice,
              "no iCE: a bright bottom pixel uses a lower-half block (%02x/%02x)", m.ch, m.attr);
        free(fl);
        free(p2.p);
    }
    CHECK(ad_image_to_canvas(jpg.p, jpg.n, 40, 1, 0, &c, NULL, NULL, err, sizeof(err)) && c.w == 40 &&
          AD_ATTR_FG(ad_canvas_get(&c, 5, 2).attr) == 12, "JPEG works too, at 40 columns");
    /* fuzz the JPEG decoder as well (uploads reach it) */
    {
        unsigned seed = 7;
        unsigned char *f = (unsigned char *)malloc(jpg.n);
        for (i = 0; i < 2000; i++) {
            int k;
            memcpy(f, jpg.p, jpg.n);
            for (k = 0; k < 6; k++) {
                seed = seed * 1103515245u + 12345u;
                f[(seed >> 8) % jpg.n] = (unsigned char)(seed >> 3);
            }
            ad_image_to_canvas(f, jpg.n, 40, 1, 1, &c, NULL, NULL, err, sizeof(err));
        }
        CHECK(1, "2000 corrupted JPEGs, no crash");
        free(f);
    }

    /* a gray ramp: dithering mixes colors, plain mapping makes bands */
    {
        unsigned char *g = (unsigned char *)malloc(64 * 64 * 3);
        Mem p3 = { NULL, 0 };
        int x, y, mixed = 0;
        for (y = 0; y < 64; y++)
            for (x = 0; x < 64; x++) memset(g + (y * 64 + x) * 3, 40 + x * 2, 3);
        stbi_write_png_to_func(mem_out, &p3, 64, 64, 3, g, 64 * 3);
        CHECK(ad_image_to_canvas(p3.p, p3.n, 64, 1, 1, &c, NULL, NULL, err, sizeof(err)), "dithered import works");
        for (y = 0; y < c.h; y++)
            for (x = 1; x < 64; x++)
                if (ad_canvas_get(&c, x, y).attr != ad_canvas_get(&c, x - 1, y).attr) mixed++;
        CHECK(mixed > 200, "dithering mixes neighbouring colors (%d changes)", mixed);
        free(g);
        free(p3.p);
    }

    /* rejected before decoding: garbage, and a header claiming 9000x9000 */
    CHECK(!ad_image_to_canvas((const unsigned char *)"hello", 5, 80, 1, 0, &c, NULL, NULL, err, sizeof(err)),
          "garbage is refused: %s", err);
    {
        unsigned char *big = (unsigned char *)malloc(png.n);
        memcpy(big, png.p, png.n);
        /* IHDR width/height at 16..23 */
        big[16] = 0; big[17] = 0; big[18] = 0x23; big[19] = 0x28;
        big[20] = 0; big[21] = 0; big[22] = 0x23; big[23] = 0x28;
        CHECK(!ad_image_to_canvas(big, png.n, 80, 1, 0, &c, NULL, NULL, err, sizeof(err)) && strstr(err, "8192"),
              "a 9000x9000 image is refused before decoding: %s", err);
        free(big);
    }
    /* fuzz: corrupted PNGs never crash */
    {
        unsigned seed = 99;
        unsigned char *f = (unsigned char *)malloc(png.n);
        int ok = 0;
        for (i = 0; i < 2000; i++) {
            int k;
            memcpy(f, png.p, png.n);
            for (k = 0; k < 8; k++) {
                seed = seed * 1103515245u + 12345u;
                f[(seed >> 8) % png.n] = (unsigned char)(seed >> 3);
            }
            ok += ad_image_to_canvas(f, png.n, 80, i & 1, i & 2, &c, NULL, NULL, err, sizeof(err));
        }
        CHECK(1, "2000 corrupted PNGs, no crash (%d still decoded)", ok);
        free(f);
    }
    CHECK(ad_image_name("Photo.JPG") && ad_image_name("a.png") && !ad_image_name("a.ans"), "image names recognized");

    ad_canvas_free(&c);
    free(px);
    free(png.p);
    free(jpg.p);
    printf("\n%d failure(s)\n", fails);
    return fails ? 1 : 0;
}
