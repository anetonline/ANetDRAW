/* Save/open formats (formats.c): round trips of random art through BIN,
 * XBin, PCBoard @X, pipe codes and Ctrl-A; exact bytes for the BBS
 * codes; a compressed XBin; PNG size and pixels; ASCII glyphs; format
 * sniffing.
 *
 *   cc -O1 -g -fsanitize=address,undefined -o test_formats tests/test_formats.c \
 *      src/formats.c src/fileio.c src/canvas.c src/font8x16.c
 */
#include "../include/formats.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "../third_party/stb/stb_image.h"

static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } \
                           else { printf("PASS: " __VA_ARGS__); printf("\n"); } } while (0)

static unsigned rng = 4242;
static unsigned rnd(void) { rng = rng * 1103515245u + 12345u; return (rng >> 16) & 0x7fff; }

/* Glyphs a round trip can't carry: the file-breaking controls (saved as
   look-alikes) and, in the BBS formats, their own code characters. */
static int ok_glyph(int ch, int fmt) {
    if (ch == 0 || ch == 10 || ch == 13 || ch == 26 || ch == 27) return 0;
    if (fmt == AD_FMT_PCBOARD && ch == '@') return 0;
    if (fmt == AD_FMT_PIPE && ch == '|') return 0;
    if (fmt == AD_FMT_CTRLA && ch == 1) return 0;
    return 1;
}

static void random_art(AdCanvas *c, int fmt) {
    int x, y;
    ad_canvas_clear(c);
    for (y = 0; y < 25; y++)
        for (x = 0; x < c->w; x++) {
            int ch, at;
            if (rnd() % 3 == 0) continue;  /* leave some blanks */
            do ch = (int)(rnd() % 256); while (!ok_glyph(ch, fmt));
            do at = (int)(rnd() % 256); while (fmt == AD_FMT_PCBOARD && (at == 0x00 || at == 0xFF));
            ad_canvas_set(c, x, y, (unsigned char)ch, (unsigned char)at);
        }
    c->h = 25;
    c->fixed = 1;
    c->ice = 1;
}

static int same_art(const AdCanvas *a, const AdCanvas *b, int glyphs_only) {
    int x, y;
    for (y = 0; y < 25; y++)
        for (x = 0; x < a->w; x++) {
            AdCell p = ad_canvas_get(a, x, y), q = ad_canvas_get(b, x, y);
            int pb = (p.ch == ' ' || p.ch == 0) && AD_ATTR_BG(p.attr) == 0;
            int qb = (q.ch == ' ' || q.ch == 0) && AD_ATTR_BG(q.attr) == 0;
            if (glyphs_only) {
                if ((p.ch ? p.ch : ' ') != (q.ch ? q.ch : ' ')) return printf("  (%d,%d) %02x vs %02x\n", x, y, p.ch, q.ch), 0;
            } else if (pb && qb) {
                continue;
            } else if (p.ch != q.ch || p.attr != q.attr) {
                printf("  (%d,%d) %02x/%02x vs %02x/%02x\n", x, y, p.ch, p.attr, q.ch, q.attr);
                return 0;
            }
        }
    return 1;
}

static int roundtrip(int fmt, int glyphs_only) {
    AdCanvas a, b;
    AdSauce m, got;
    unsigned char *enc = NULL;
    size_t n;
    char err[80];
    int ok;
    ad_canvas_init(&a, 80, 25);
    ad_canvas_init(&b, 80, 25);
    random_art(&a, fmt);
    memset(&m, 0, sizeof(m));
    strcpy(m.title, "Round trip");
    n = ad_fmt_encode(&a, &m, fmt, &enc);
    ok = n > 0 && ad_fmt_decode(enc, n, fmt, &b, &got, err, sizeof(err)) && same_art(&a, &b, glyphs_only) &&
         (got.format == fmt || (fmt == AD_FMT_ASCII && got.format == AD_FMT_ASCII));
    if (!ok) printf("  %s: n=%zu fmt=%d err=%s\n", ad_fmt_name(fmt), n, got.format, err);
    free(enc);
    ad_canvas_free(&a);
    ad_canvas_free(&b);
    return ok;
}

static int eq(const unsigned char *p, size_t n, const char *want, size_t wn) {
    return n == wn && memcmp(p, want, n) == 0;
}
#define EQ(p, n, lit) eq(p, n, lit, sizeof(lit) - 1)

static size_t enc_small(int fmt, unsigned char **out, int clear) {
    AdCanvas c;
    AdSauce m;
    size_t n;
    ad_canvas_init(&c, 80, 25);
    ad_canvas_set(&c, 0, 0, 'A', AD_ATTR(12, 1));   /* bright red on blue */
    ad_canvas_set(&c, 1, 0, 'B', AD_ATTR(12, 1));
    ad_canvas_set(&c, 2, 0, 'C', AD_ATTR(14, 0));   /* yellow on black */
    memset(&m, 0, sizeof(m));
    m.clear_screen = clear;
    n = ad_fmt_encode(&c, &m, fmt, out);
    ad_canvas_free(&c);
    return n;
}

int main(void) {
    unsigned char *enc = NULL;
    size_t n;
    int f;

    for (f = AD_FMT_BIN; f <= AD_FMT_CTRLA; f++)
        CHECK(roundtrip(f, 0), "%s: random art round-trips cell for cell", ad_fmt_name(f));
    CHECK(roundtrip(AD_FMT_ASCII, 1), "Plain ASCII: glyphs round-trip");
    CHECK(roundtrip(AD_FMT_ANSI, 0), "ANSI still round-trips through the dispatcher");

    n = enc_small(AD_FMT_PCBOARD, &enc, 1);
    CHECK(EQ(enc, n, "@X07@CLS@@X1CAB@X0EC@X07"), "PCBoard: @X<bg><fg> hex, @CLS@ first: %.*s", (int)n, enc);
    free(enc);
    n = enc_small(AD_FMT_PIPE, &enc, 1);
    CHECK(EQ(enc, n, "|07|16|CL|12|17AB|14|16C|07|16"), "Pipe: |fg |16+bg, |CL first: %.*s", (int)n, enc);
    free(enc);
    n = enc_small(AD_FMT_CTRLA, &enc, 1);
    CHECK(EQ(enc, n, "\x01N\x01L\x01N\x01H\x01R\x01" "4AB\x01N\x01H\x01YC\x01N"),
          "Ctrl-A: N, H, letter, bg digit, L first (%zu bytes)", n);
    free(enc);

    /* @X00 / @XFF would mean save/restore: never written */
    {
        AdCanvas c;
        AdSauce m;
        ad_canvas_init(&c, 80, 25);
        ad_canvas_set(&c, 0, 0, 'Z', 0x00);
        ad_canvas_set(&c, 1, 0, 'Y', 0xFF);
        memset(&m, 0, sizeof(m));
        n = ad_fmt_encode(&c, &m, AD_FMT_PCBOARD, &enc);
        CHECK(!memmem(enc, n, "@X00", 4) && !memmem(enc, n, "@XFF", 4), "PCBoard never writes @X00 or @XFF: %.*s", (int)n, enc);
        free(enc);
        ad_canvas_free(&c);
    }

    /* a compressed XBin, built by hand: row 0 = "ABAB" in 1E, then "xyz" as a char run */
    {
        static const unsigned char xb[] = {
            'X', 'B', 'I', 'N', 0x1A, 8, 0, 1, 0, 16, 4 | 8,
            0x80 | 3, 0x1E, 'A', 'B', 'A', 'B',   /* attr run: 4 chars in 1E */
            0x40 | 2, 'x', 0x0C, 0x0D, 0x0E,        /* char run: 3 x 'x', 3 attrs */
            0xC0 | 0, 'z', 0x0F                     /* both: 1 x 'z' in 0F */
        };
        AdCanvas c;
        AdSauce m;
        char err[80];
        ad_canvas_init(&c, 80, 25);
        CHECK(ad_fmt_decode(xb, sizeof(xb), AD_FMT_XBIN, &c, &m, err, sizeof(err)) && c.w == 8 && c.ice &&
              ad_canvas_get(&c, 2, 0).ch == 'A' && ad_canvas_get(&c, 2, 0).attr == 0x1E &&
              ad_canvas_get(&c, 5, 0).ch == 'x' && ad_canvas_get(&c, 5, 0).attr == 0x0D &&
              ad_canvas_get(&c, 7, 0).ch == 'z' && ad_canvas_get(&c, 7, 0).attr == 0x0F,
              "a compressed XBin decodes (w=%d ice=%d)", c.w, c.ice);
        ad_canvas_free(&c);
    }

    /* PNG: 8x16 per cell, the glyph in its colors */
    {
        int w, h, comp;
        unsigned char *px;
        n = enc_small(AD_FMT_PNG, &enc, 0);
        px = stbi_load_from_memory(enc, (int)n, &w, &h, &comp, 3);
        CHECK(px && w == 640 && h == 400, "PNG is 640x400 for 80x25 (%dx%d)", w, h);
        if (px) {
            /* cell (0,0) 'A' bright red on blue: background pixel at its top-left */
            unsigned char *p0 = px;
            int red = 0, x, y;
            for (y = 0; y < 16; y++)
                for (x = 0; x < 8; x++) {
                    unsigned char *p = px + ((size_t)y * w + x) * 3;
                    if (p[0] == 255 && p[1] == 85 && p[2] == 85) red++;
                }
            CHECK(p0[0] == 0 && p0[1] == 0 && p0[2] == 170 && red > 10, "PNG draws the glyph in its colors (%d red px)", red);
            stbi_image_free(px);
        }
        free(enc);
    }

    /* sniffing: an .asc carrying @X codes is PCBoard */
    {
        static const char pcb[] = "@X0FHi @X1Cthere@X07\r\n";
        AdCanvas c;
        AdSauce m;
        char err[80];
        ad_canvas_init(&c, 80, 25);
        CHECK(ad_fmt_decode((const unsigned char *)pcb, strlen(pcb), ad_fmt_from_path("menu.asc"), &c, &m, err, sizeof(err)) &&
              m.format == AD_FMT_PCBOARD && ad_canvas_get(&c, 3, 0).ch == 't' && ad_canvas_get(&c, 3, 0).attr == 0x1C,
              "an .asc with @X codes is read as PCBoard");
        ad_canvas_free(&c);
    }
    CHECK(ad_fmt_from_path("x.PCB") == AD_FMT_PCBOARD && ad_fmt_from_path("x.msg") == AD_FMT_CTRLA &&
          ad_fmt_from_path("x.xb") == AD_FMT_XBIN && ad_fmt_from_path("x.nfo") == AD_FMT_ASCII,
          "extensions map to formats");

    printf("\n%d failure(s)\n", fails);
    return fails ? 1 : 0;
}
