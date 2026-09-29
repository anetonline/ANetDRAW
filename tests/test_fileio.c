#define _GNU_SOURCE  /* memmem */
/* Unit tests for fileio.c: SAUCE record layout (offsets from the SAUCE
 * spec), round trips of random canvases, the control-glyph substitutions,
 * and loading both line conventions -- Moebius-style full rows with no
 * newline and TheDraw-style full rows followed by CR LF.
 *
 *   cc -O1 -g -fsanitize=address,undefined -o test_fileio \
 *      tests/test_fileio.c src/fileio.c src/canvas.c \
 *      src/formats.c src/font8x16.c
 */
#include "../include/fileio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } \
                           else { printf("PASS: " __VA_ARGS__); printf("\n"); } } while (0)

static unsigned rng = 12345;
static unsigned rnd(void) { rng = rng * 1103515245u + 12345u; return (rng >> 16) & 0x7fff; }

static int same_art(const AdCanvas *a, const AdCanvas *b, int w, int h, int *bad_x, int *bad_y) {
    int x, y;
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++) {
            AdCell p = ad_canvas_get(a, x, y), q = ad_canvas_get(b, x, y);
            unsigned char pc = p.ch;
            /* the four substituted glyphs come back as their stand-ins */
            if (pc == 10) pc = 9; else if (pc == 13) pc = 14; else if (pc == 26) pc = 16; else if (pc == 27) pc = 17;
            /* a blank on black may come back with any foreground */
            if (pc == ' ' && AD_ATTR_BG(p.attr) == 0 && q.ch == ' ' && AD_ATTR_BG(q.attr) == 0) continue;
            if (pc != q.ch || p.attr != q.attr) { *bad_x = x; *bad_y = y; return 0; }
        }
    return 1;
}

int main(void) {
    AdCanvas a, b;
    AdSauce meta, got;
    unsigned char *enc;
    size_t n;
    int x, y, t, bx = -1, by = -1;
    char err[128];

    /* ---- SAUCE record layout */
    ad_canvas_init(&a, 80, 25);
    ad_canvas_resize(&a, 80, 25);  /* fixed 80x25 */
    a.ice = 1;
    ad_canvas_set(&a, 0, 0, 'A', AD_ATTR(15, 9));
    memset(&meta, 0, sizeof(meta));
    strcpy(meta.title, "Smiley");
    strcpy(meta.author, "StingRay");
    strcpy(meta.group, "A-Net");
    n = ad_ans_encode(&a, &meta, &enc);
    {
        const unsigned char *r = enc + n - 128;
        size_t art = (size_t)(r[90] | (r[91] << 8) | (r[92] << 16) | ((size_t)r[93] << 24));
        CHECK(memcmp(r, "SAUCE00", 7) == 0, "record starts SAUCE00 at EOF-128");
        CHECK(memcmp(r + 7, "Smiley ", 7) == 0 && r[41] == ' ', "title at 7, space-padded to 35");
        CHECK(memcmp(r + 42, "StingRay", 8) == 0 && r[61] == ' ', "author at 42, 20 wide");
        CHECK(memcmp(r + 62, "A-Net", 5) == 0 && r[81] == ' ', "group at 62, 20 wide");
        CHECK(r[82] >= '1' && r[82] <= '9' && r[89] >= '0' && r[89] <= '9', "date CCYYMMDD at 82");
        CHECK(art == n - 129 && enc[art] == 0x1A, "FileSize = art bytes, followed by 0x1A EOF");
        CHECK(r[94] == 1 && r[95] == 1, "DataType 1 Character, FileType 1 ANSi");
        CHECK(r[96] == 80 && r[97] == 0 && r[98] == 25 && r[99] == 0, "TInfo1 width 80, TInfo2 lines 25");
        CHECK(r[104] == 0, "no comment block");
        CHECK((r[105] & 1) == 1 && ((r[105] >> 1) & 3) == 1 && ((r[105] >> 3) & 3) == 2,
              "TFlags: iCE, 8px letter spacing, square aspect (0x%02X)", r[105]);
        CHECK(memcmp(r + 106, "IBM VGA", 7) == 0 && r[113] == 0, "TInfoS font name, zero-padded");
        /* white is already the current fg after the reset, so no 37 */
        CHECK(memcmp(enc, "\x1b[0m\x1b[1;5;44mA", 13) == 0, "bright fg as bold, bright bg as blink: %.20s", enc + 1);
    }
    ad_canvas_init(&b, 80, 25);
    CHECK(ad_ans_decode(enc, n, &b, &got, err, sizeof(err)), "decodes its own output");
    CHECK(got.has_sauce && strcmp(got.title, "Smiley") == 0 && strcmp(got.author, "StingRay") == 0 &&
          strcmp(got.group, "A-Net") == 0, "SAUCE fields read back");
    CHECK(b.w == 80 && b.h == 25 && b.ice == 1 && b.fixed, "size and iCE flag read back");
    CHECK(ad_canvas_get(&b, 0, 0).ch == 'A' && ad_canvas_get(&b, 0, 0).attr == AD_ATTR(15, 9), "cell read back");
    free(enc);

    /* ---- random round trips, several widths */
    for (t = 0; t < 300; t++) {
        int w = (t % 3 == 0) ? 80 : (t % 3 == 1) ? 132 : 1 + (int)(rnd() % 160);
        int h = 1 + (int)(rnd() % 60), ok;
        ad_canvas_free(&a);
        ad_canvas_init(&a, w, h);
        ad_canvas_resize(&a, w, h);
        a.ice = (int)(rnd() & 1);
        for (y = 0; y < h; y++)
            for (x = 0; x < w; x++) {
                unsigned r = rnd() % 10;
                if (r < 4) continue;  /* leave blank: exercises trimming */
                ad_canvas_set(&a, x, y, (unsigned char)(r < 6 ? ' ' : rnd() % 256),
                              (unsigned char)(rnd() % 256));
            }
        memset(&meta, 0, sizeof(meta));
        n = ad_ans_encode(&a, &meta, &enc);
        ok = ad_ans_decode(enc, n, &b, &got, err, sizeof(err));
        if (!ok || b.w != w || b.h != h || b.ice != a.ice || !same_art(&a, &b, w, h, &bx, &by)) {
            CHECK(0, "round trip %d (%dx%d) differs at %d,%d", t, w, h, bx, by);
            free(enc);
            break;
        }
        free(enc);
        if (t == 299) CHECK(1, "300 random canvases (widths 1-160, iCE on/off) round-trip exactly");
    }

    /* ---- the two line conventions, no SAUCE */
    {
        char full[81];
        unsigned char buf[400];
        size_t k = 0;
        memset(full, 'X', 80);
        full[80] = 0;
        /* TheDraw style: 80 chars + CR LF, then a short line */
        memcpy(buf + k, full, 80); k += 80;
        memcpy(buf + k, "\r\nnext", 6); k += 6;
        CHECK(ad_ans_decode(buf, k, &b, &got, err, sizeof(err)), "decodes TheDraw-style text");
        CHECK(ad_canvas_get(&b, 79, 0).ch == 'X' && ad_canvas_get(&b, 0, 1).ch == 'n' &&
              ad_canvas_get(&b, 0, 2).ch == ' ', "80 chars + CR LF is ONE line break (no blank line)");
        CHECK(!got.has_sauce && b.w == 80 && b.h == 25 && b.ice == 0, "no SAUCE: 80 wide, at least 25 rows, blink mode");
        /* Moebius style: 80 chars, no newline, then the next row */
        k = 0;
        memcpy(buf + k, full, 80); k += 80;
        memcpy(buf + k, "next", 4); k += 4;
        CHECK(ad_ans_decode(buf, k, &b, &got, err, sizeof(err)) && ad_canvas_get(&b, 0, 1).ch == 'n',
              "80 chars with no newline wraps to the next row");
        /* cursor moves, save/restore, SGR forms, EOF stops parsing */
        k = 0;
        {
            const char *s = "\x1b[2J\x1b[3;5HA\x1b[s\x1b[10CB\x1b[u\x1b[1;31mC\x1b[0;97;104mD\x1b[7mE\x1a" "Z";
            memcpy(buf, s, strlen(s) + 1);
            k = strlen(s);
            k += 2;  /* include the 0x1A and the trailing Z */
        }
        CHECK(ad_ans_decode(buf, k, &b, &got, err, sizeof(err)), "decodes cursor/SGR test");
        CHECK(ad_canvas_get(&b, 4, 2).ch == 'A', "CUP 3;5 puts A at column 5 row 3");
        CHECK(ad_canvas_get(&b, 15, 2).ch == 'B', "save, CUF 10 skips 10 columns (A at 4, cursor 5, B at 15)");
        CHECK(ad_canvas_get(&b, 5, 2).ch == 'C' && ad_canvas_get(&b, 5, 2).attr == AD_ATTR(12, 0),
              "restore brings the cursor back; 1;31 = bright red");
        CHECK(ad_canvas_get(&b, 6, 2).attr == AD_ATTR(15, 9), "97;104 = bright white on bright blue");
        CHECK(ad_canvas_get(&b, 7, 2).ch == 'E' && ad_canvas_get(&b, 7, 2).attr == AD_ATTR(9, 15),
              "SGR 7 swaps foreground and background");
        CHECK(ad_canvas_get(&b, 8, 2).ch == ' ', "parsing stops at the 0x1A EOF");
    }

    /* ---- hostile input: truncated sequences, huge params, junk SAUCE */
    {
        unsigned char junk[4096];
        int i;
        for (i = 0; i < 200; i++) {
            size_t len = rnd() % sizeof(junk), j;
            for (j = 0; j < len; j++) junk[j] = (unsigned char)((rnd() % 4 == 0) ? (unsigned)(unsigned char)"\x1b[;0123456789mHABCDsu\r\n"[rnd() % 23] : rnd());
            if (len >= 128 && rnd() % 2) memcpy(junk + len - 128, "SAUCE00", 7);
            ad_ans_decode(junk, len, &b, &got, err, sizeof(err));
        }
        CHECK(1, "200 random junk files decode without crashing (run under ASan)");
    }

    ad_canvas_free(&a);
    ad_canvas_free(&b);
    /* TheDraw-style display options: clear screen + DECSCS speed */
    {
        AdCanvas a, b;
        AdSauce meta, got;
        unsigned char *enc = NULL;
        size_t n;
        char err[80];
        ad_canvas_init(&a, 80, 25);
        ad_canvas_init(&b, 80, 25);
        ad_canvas_set(&a, 3, 2, 'X', AD_ATTR(12, 1));
        memset(&meta, 0, sizeof(meta));
        meta.clear_screen = 1;
        meta.speed = 6;  /* 9600 */
        n = ad_ans_encode(&a, &meta, &enc);
        CHECK(n > 20 && memcmp(enc, "\x1b[0;6*r\x1b[0m\x1b[2J\x1b[1;1H", 21) == 0,
              "options: speed, then clear screen + home, before the art");
        {
            size_t fs = (size_t)enc[n - 128 + 90] | (size_t)enc[n - 128 + 91] << 8;
            CHECK(fs > 7 && memcmp(enc + fs - 7, "\x1b[0;0*r", 7) == 0, "options: full speed restored at the end of the art");
        }
        CHECK(ad_ans_decode(enc, n, &b, &got, err, sizeof(err)), "options: decodes");
        CHECK(got.clear_screen == 1 && got.speed == 6, "options: read back (clear=%d speed=%d)", got.clear_screen, got.speed);
        CHECK(ad_canvas_get(&b, 3, 2).ch == 'X' && ad_canvas_get(&b, 3, 2).attr == AD_ATTR(12, 1),
              "options: the art itself is unchanged");
        free(enc);
        memset(&meta, 0, sizeof(meta));
        n = ad_ans_encode(&a, &meta, &enc);
        CHECK(memcmp(enc, "\x1b[0m", 4) == 0 && !memmem(enc, n, "*r", 2) && !memmem(enc, n, "[2J", 3),
              "no options: plain art, nothing extra");
        CHECK(ad_ans_decode(enc, n, &b, &got, err, sizeof(err)) && !got.clear_screen && !got.speed,
              "no options: none read back");
        free(enc);
        ad_canvas_free(&a);
        ad_canvas_free(&b);
    }

    printf("\n%d failure(s)\n", fails);
    return fails ? 1 : 0;
}
