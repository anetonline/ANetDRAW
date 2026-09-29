/* Formats besides ANSI -- see include/formats.h for the references. */
#include "../include/formats.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WRITE_NO_STDIO
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-compare"
#endif
#include "../third_party/stb/stb_image_write.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

extern const unsigned char ad_font8x16[256][16];

static const char *const EXT[AD_FMT_COUNT] = {
    ".ans", ".asc", ".bin", ".xb", ".pcb", ".pip", ".msg", ".png"
};
static const char *const NAME[AD_FMT_COUNT] = {
    "ANSI", "Plain ASCII", "BIN", "XBin", "PCBoard @X", "Pipe codes", "Synchronet Ctrl-A", "PNG image"
};
static const int PC_TO_ANSI[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };

const char *ad_fmt_ext(int f) { return f >= 0 && f < AD_FMT_COUNT ? EXT[f] : EXT[0]; }
const char *ad_fmt_name(int f) { return f >= 0 && f < AD_FMT_COUNT ? NAME[f] : NAME[0]; }

static int ends_with(const char *s, const char *ext) {
    size_t n = strlen(s), e = strlen(ext), i;
    if (n < e) return 0;
    for (i = 0; i < e; i++)
        if (tolower((unsigned char)s[n - e + i]) != ext[i]) return 0;
    return 1;
}

int ad_fmt_from_path(const char *path) {
    int f;
    for (f = 0; f < AD_FMT_COUNT; f++)
        if (ends_with(path, EXT[f])) return f;
    if (ends_with(path, ".txt") || ends_with(path, ".nfo") || ends_with(path, ".diz")) return AD_FMT_ASCII;
    return AD_FMT_ANSI;
}

int ad_fmt_known_ext(const char *name) {
    int f;
    for (f = 0; f < AD_FMT_COUNT; f++)
        if (ends_with(name, EXT[f])) return 1;
    return ends_with(name, ".ice") || ends_with(name, ".txt") || ends_with(name, ".nfo") ||
           ends_with(name, ".diz");
}

/* ------------------------------------------------------------ buffer */

typedef struct {
    unsigned char *p;
    size_t n, cap;
    int oom;
} Buf;

static void bput(Buf *b, const void *src, size_t len) {
    if (b->oom) return;
    if (b->n + len > b->cap) {
        size_t cap = b->cap ? b->cap * 2 : 4096;
        unsigned char *np;
        while (cap < b->n + len) cap *= 2;
        np = (unsigned char *)realloc(b->p, cap);
        if (!np) { b->oom = 1; return; }
        b->p = np;
        b->cap = cap;
    }
    memcpy(b->p + b->n, src, len);
    b->n += len;
}
static void bputc(Buf *b, unsigned char c) { bput(b, &c, 1); }
static void bputs(Buf *b, const char *s) { bput(b, s, strlen(s)); }

static size_t finish(Buf *b, unsigned char **out) {
    if (b->oom) {
        free(b->p);
        *out = NULL;
        return 0;
    }
    *out = b->p;
    return b->n;
}

static int blank(AdCell c) { return (c.ch == ' ' || c.ch == 0) && AD_ATTR_BG(c.attr) == 0; }

static int last_used(const AdCanvas *c, int y) {
    int x, last = -1;
    for (x = 0; x < c->w; x++)
        if (!blank(ad_canvas_get(c, x, y))) last = x;
    return last;
}

static void sauce_tail(Buf *b, const AdCanvas *c, const AdSauce *meta, size_t art_len,
                       int dt, int ft, int t1, int t2) {
    unsigned char rec[128];
    ad_sauce_fill(rec, meta, art_len, dt, ft, t1, t2, c->ice);
    bputc(b, 0x1A);
    bput(b, rec, 128);
}

/* ------------------------------------------------------------ encoders */

static size_t enc_ascii(const AdCanvas *c, const AdSauce *meta, unsigned char **out) {
    Buf b = { NULL, 0, 0, 0 };
    int rows = ad_save_rows(c), x, y;
    size_t art;
    for (y = 0; y < rows; y++) {
        int last = last_used(c, y);
        for (x = 0; x <= last; x++) {
            AdCell cell = ad_canvas_get(c, x, y);
            bputc(&b, ad_safe_glyph(cell.ch ? cell.ch : ' '));
        }
        if (last < c->w - 1 && y < rows - 1) bputs(&b, "\r\n");
    }
    art = b.n;
    sauce_tail(&b, c, meta, art, 1, 0, c->w, rows);  /* Character / ASCII */
    return finish(&b, out);
}

static size_t enc_bin(const AdCanvas *c, const AdSauce *meta, unsigned char **out) {
    Buf b = { NULL, 0, 0, 0 };
    int rows = ad_save_rows(c), w = c->w + (c->w & 1), x, y;
    for (y = 0; y < rows; y++)
        for (x = 0; x < w; x++) {
            AdCell cell = ad_canvas_get(c, x, y);
            bputc(&b, cell.ch);
            bputc(&b, cell.attr);
        }
    /* BinaryText: FileType is the width / 2 */
    sauce_tail(&b, c, meta, b.n, 5, w / 2, 0, 0);
    return finish(&b, out);
}

static size_t enc_xbin(const AdCanvas *c, const AdSauce *meta, unsigned char **out) {
    Buf b = { NULL, 0, 0, 0 };
    int rows = ad_save_rows(c), x, y;
    unsigned char h[11];
    memcpy(h, "XBIN\x1A", 5);
    h[5] = (unsigned char)(c->w & 0xFF);
    h[6] = (unsigned char)(c->w >> 8);
    h[7] = (unsigned char)(rows & 0xFF);
    h[8] = (unsigned char)(rows >> 8);
    h[9] = 16;                       /* font height */
    h[10] = (unsigned char)(c->ice ? 8 : 0);  /* flags: non-blink (iCE) */
    bput(&b, h, 11);
    for (y = 0; y < rows; y++)
        for (x = 0; x < c->w; x++) {
            AdCell cell = ad_canvas_get(c, x, y);
            bputc(&b, cell.ch);
            bputc(&b, cell.attr);
        }
    sauce_tail(&b, c, meta, b.n, 6, 0, 0, 0);  /* XBin keeps its own size */
    return finish(&b, out);
}

/* PCBoard can't write @X00 (save color) or @XFF (restore color). */
static AdCell pcb_safe(AdCell c) {
    if (c.attr == 0x00) { c.ch = ' '; c.attr = 0x07; }          /* invisible anyway */
    else if (c.attr == 0xFF) { c.ch = 0xDB; c.attr = 0x0F; }     /* solid bright white */
    return c;
}

static size_t enc_bbs(const AdCanvas *c, const AdSauce *meta, int fmt, unsigned char **out) {
    static const char CTRLA_FG[8] = { 'K', 'B', 'G', 'C', 'R', 'M', 'Y', 'W' };  /* PC order */
    Buf b = { NULL, 0, 0, 0 };
    int x, y, cur = -1, ew, rows;
    char seq[32];
    int clear = meta && meta->clear_screen;
    /* a display file stops at its last used row: blank lines after it
       would only scroll the caller's screen */
    ad_canvas_extent(c, &ew, &rows);
    if (fmt == AD_FMT_PCBOARD) { bputs(&b, "@X07"); if (clear) bputs(&b, "@CLS@"); cur = 0x07; }
    if (fmt == AD_FMT_PIPE) { bputs(&b, "|07|16"); if (clear) bputs(&b, "|CL"); cur = 0x07; }
    if (fmt == AD_FMT_CTRLA) { bputs(&b, "\x01N"); if (clear) bputs(&b, "\x01L"); cur = 0x07; }
    for (y = 0; y < rows; y++) {
        int last = last_used(c, y);
        for (x = 0; x <= last; x++) {
            AdCell cell = ad_canvas_get(c, x, y);
            int fg, bg;
            if (fmt == AD_FMT_PCBOARD) cell = pcb_safe(cell);
            if (cell.ch == 0) cell.ch = ' ';
            fg = AD_ATTR_FG(cell.attr);
            bg = AD_ATTR_BG(cell.attr);
            if (cell.attr != cur) {
                if (fmt == AD_FMT_PCBOARD) {
                    snprintf(seq, sizeof(seq), "@X%X%X", bg, fg);
                    bputs(&b, seq);
                } else if (fmt == AD_FMT_PIPE) {
                    if (cur < 0 || fg != AD_ATTR_FG(cur)) { snprintf(seq, sizeof(seq), "|%02d", fg); bputs(&b, seq); }
                    if (cur < 0 || bg != AD_ATTR_BG(cur)) {
                        snprintf(seq, sizeof(seq), "|%02d", bg < 8 ? 16 + bg : 24 + bg - 8);
                        bputs(&b, seq);
                    }
                } else {
                    /* ^A N resets to light gray on black; build up from there */
                    bputs(&b, "\x01N");
                    if (fg > 7) bputs(&b, "\x01H");
                    bputc(&b, 0x01);
                    bputc(&b, (unsigned char)CTRLA_FG[fg & 7]);
                    if (bg & 7) { bputc(&b, 0x01); bputc(&b, (unsigned char)('0' + PC_TO_ANSI[bg & 7])); }
                    if (bg > 7) bputs(&b, c->ice ? "\x01" "E" : "\x01" "I");
                }
                cur = cell.attr;
            }
            bputc(&b, ad_safe_glyph(cell.ch));
        }
        if (last < c->w - 1 && y < rows - 1) bputs(&b, "\r\n");
    }
    if (fmt == AD_FMT_PCBOARD) bputs(&b, "@X07");
    if (fmt == AD_FMT_PIPE) bputs(&b, "|07|16");
    if (fmt == AD_FMT_CTRLA) bputs(&b, "\x01N");
    return finish(&b, out);
}

static const unsigned char VGA_RGB[16][3] = {
    { 0, 0, 0 }, { 0, 0, 170 }, { 0, 170, 0 }, { 0, 170, 170 }, { 170, 0, 0 }, { 170, 0, 170 },
    { 170, 85, 0 }, { 170, 170, 170 }, { 85, 85, 85 }, { 85, 85, 255 }, { 85, 255, 85 },
    { 85, 255, 255 }, { 255, 85, 85 }, { 255, 85, 255 }, { 255, 255, 85 }, { 255, 255, 255 }
};

static void png_out(void *ctx, void *data, int size) {
    bput((Buf *)ctx, data, (size_t)size);
}

static size_t enc_png(const AdCanvas *c, unsigned char **out) {
    Buf b = { NULL, 0, 0, 0 };
    int rows = ad_save_rows(c), W = c->w * 8, H = rows * 16, x, y, gy, gx;
    unsigned char *px = (unsigned char *)malloc((size_t)W * H * 3);
    if (!px) { *out = NULL; return 0; }
    for (y = 0; y < rows; y++)
        for (x = 0; x < c->w; x++) {
            AdCell cell = ad_canvas_get(c, x, y);
            int fg = AD_ATTR_FG(cell.attr), bg = AD_ATTR_BG(cell.attr);
            if (!c->ice) bg &= 7;  /* blink mode: the steady background */
            for (gy = 0; gy < 16; gy++) {
                unsigned char bits = ad_font8x16[cell.ch][gy];
                unsigned char *p = px + ((size_t)(y * 16 + gy) * W + x * 8) * 3;
                for (gx = 0; gx < 8; gx++, p += 3)
                    memcpy(p, VGA_RGB[(bits & (0x80 >> gx)) ? fg : bg], 3);
            }
        }
    if (!stbi_write_png_to_func(png_out, &b, W, H, 3, px, W * 3)) b.oom = 1;
    free(px);
    return finish(&b, out);
}

size_t ad_fmt_encode(const AdCanvas *c, const AdSauce *meta, int fmt, unsigned char **out) {
    switch (fmt) {
        case AD_FMT_ASCII: return enc_ascii(c, meta, out);
        case AD_FMT_BIN: return enc_bin(c, meta, out);
        case AD_FMT_XBIN: return enc_xbin(c, meta, out);
        case AD_FMT_PCBOARD: case AD_FMT_PIPE: case AD_FMT_CTRLA: return enc_bbs(c, meta, fmt, out);
        case AD_FMT_PNG: return enc_png(c, out);
        default: return ad_ans_encode(c, meta, out);
    }
}

/* ------------------------------------------------------------ decoders */

/* Raw cells into a fresh canvas (BIN, XBin). */
static int load_cells(AdCanvas *c, int w, int h, const unsigned char *cells, size_t ncells,
                      int ice, char *err, size_t errsz) {
    size_t i;
    if (w < 1 || w > AD_CANVAS_MAX_W || h < 1) {
        snprintf(err, errsz, "bad size %dx%d", w, h);
        return 0;
    }
    if (h > AD_CANVAS_MAX_H) h = AD_CANVAS_MAX_H;
    if (!ad_canvas_resize(c, w, h)) {
        snprintf(err, errsz, "out of memory");
        return 0;
    }
    ad_canvas_clear(c);
    for (i = 0; i < ncells && i < (size_t)w * h; i++)
        ad_canvas_set(c, (int)(i % w), (int)(i / w), cells[i * 2], cells[i * 2 + 1]);
    c->h = h;
    c->fixed = 1;
    c->ice = ice;
    return 1;
}

static int sauce_at(const unsigned char *d, size_t len, const unsigned char **rec, size_t *art_len) {
    *art_len = len;
    *rec = NULL;
    if (len >= 128 && memcmp(d + len - 128, "SAUCE00", 7) == 0) {
        *rec = d + len - 128;
        *art_len = len - 128;
        if (*art_len && d[*art_len - 1] == 0x1A) (*art_len)--;
        return 1;
    }
    return 0;
}

static void sauce_meta(const unsigned char *r, AdSauce *meta) {
    size_t i;
    struct { char *dst; int off, n; } F[] = {
        { meta->title, 7, AD_SAUCE_TITLE_LEN }, { meta->author, 42, AD_SAUCE_AUTHOR_LEN },
        { meta->group, 62, AD_SAUCE_GROUP_LEN }, { meta->date, 82, 8 } };
    for (i = 0; i < sizeof(F) / sizeof(F[0]); i++) {
        int n = F[i].n;
        memcpy(F[i].dst, r + F[i].off, (size_t)n);
        F[i].dst[n] = 0;
        while (n > 0 && (F[i].dst[n - 1] == ' ' || F[i].dst[n - 1] == 0)) F[i].dst[--n] = 0;
    }
    meta->has_sauce = 1;
}

static int dec_bin(const unsigned char *d, size_t len, AdCanvas *c, AdSauce *meta, char *err, size_t errsz) {
    const unsigned char *rec;
    size_t art;
    int w = 160, ice = 0;
    if (sauce_at(d, len, &rec, &art)) {
        sauce_meta(rec, meta);
        if (rec[94] == 5 && rec[95]) w = rec[95] * 2;
        ice = rec[105] & 1;
    }
    return load_cells(c, w, (int)((art / 2 + (size_t)w - 1) / (size_t)w), d, art / 2, ice, err, errsz);
}

static int dec_xbin(const unsigned char *d, size_t len, AdCanvas *c, AdSauce *meta, char *err, size_t errsz) {
    const unsigned char *rec, *p, *end;
    size_t art, need, got = 0;
    int w, h, flags, fh;
    unsigned char *cells;
    int ok;
    if (sauce_at(d, len, &rec, &art)) sauce_meta(rec, meta);
    if (art < 11 || memcmp(d, "XBIN\x1A", 5) != 0) {
        snprintf(err, errsz, "not an XBin file");
        return 0;
    }
    w = d[5] | d[6] << 8;
    h = d[7] | d[8] << 8;
    fh = d[9] ? d[9] : 16;
    flags = d[10];
    p = d + 11;
    end = d + art;
    if (flags & 1) p += 48;                                   /* palette: not used */
    if (flags & 2) p += (size_t)fh * ((flags & 16) ? 512 : 256);  /* font: not used */
    if (p > end || w < 1 || h < 1 || w > AD_CANVAS_MAX_W) {
        snprintf(err, errsz, "damaged XBin header");
        return 0;
    }
    if (h > AD_CANVAS_MAX_H) h = AD_CANVAS_MAX_H;
    need = (size_t)w * h;
    cells = (unsigned char *)calloc(need, 2);
    if (!cells) {
        snprintf(err, errsz, "out of memory");
        return 0;
    }
    if (!(flags & 4)) {
        size_t n = (size_t)(end - p) / 2;
        memcpy(cells, p, (n < need ? n : need) * 2);
        got = n;
    } else {
        /* runs: 2 bits type (0 none, 1 char, 2 attr, 3 both) + count-1 */
        while (p < end && got < need) {
            int type = *p >> 6, count = (*p & 63) + 1, i;
            unsigned char ch = 0, at = 0;
            p++;
            if (type == 1 || type == 3) { if (p >= end) break; ch = *p++; }
            if (type == 2 || type == 3) { if (p >= end) break; at = *p++; }
            for (i = 0; i < count && got < need; i++, got++) {
                if (type == 0 || type == 2) { if (p >= end) break; cells[got * 2] = *p++; }
                else cells[got * 2] = ch;
                if (type == 0 || type == 1) { if (p >= end) break; cells[got * 2 + 1] = *p++; }
                else cells[got * 2 + 1] = at;
            }
        }
    }
    ok = load_cells(c, w, h, cells, need, (flags & 8) != 0, err, errsz);
    free(cells);
    return ok;
}

/* The BBS formats become ANSI, which the ANSI decoder then reads. */
static void sgr(Buf *b, int attr) {
    char s[32];
    int fg = AD_ATTR_FG(attr), bg = AD_ATTR_BG(attr);
    snprintf(s, sizeof(s), "\x1b[0;%s%s3%d;4%dm", fg > 7 ? "1;" : "", bg > 7 ? "5;" : "",
             PC_TO_ANSI[fg & 7], PC_TO_ANSI[bg & 7]);
    bputs(b, s);
}

static int hexval(unsigned char c) {
    if (c >= '0' && c <= '9') return c - '0';
    c = (unsigned char)toupper(c);
    return c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
}

static int count_codes(const unsigned char *d, size_t len, int fmt) {
    size_t i;
    int n = 0;
    for (i = 0; i + 3 < len && n < 3; i++) {
        if (fmt == AD_FMT_PCBOARD && d[i] == '@' && (d[i + 1] == 'X' || d[i + 1] == 'x') &&
            hexval(d[i + 2]) >= 0 && hexval(d[i + 3]) >= 0) n++;
        if (fmt == AD_FMT_PIPE && d[i] == '|' && isdigit(d[i + 1]) && isdigit(d[i + 2]) &&
            (d[i + 1] - '0') * 10 + (d[i + 2] - '0') < 32) n++;
        if (fmt == AD_FMT_CTRLA && d[i] == 0x01 && strchr("KRGYBMCWkrgybmcwHhNn01234567", d[i + 1]) && d[i + 1]) n++;
    }
    return n;
}

static int dec_bbs(const unsigned char *d, size_t len, int fmt, AdCanvas *c, AdSauce *meta,
                   char *err, size_t errsz) {
    static const char SYNC_FG[] = "KBGCRMYW";  /* PC color order */
    Buf b = { NULL, 0, 0, 0 };
    size_t i;
    int attr = 0x07, clear = 0, any = 0, ok, ice = 0;
    for (i = 0; i < len; i++) {
        unsigned char ch = d[i];
        if (ch == 0x1A) break;
        if (fmt == AD_FMT_PCBOARD && ch == '@' && i + 3 < len && (d[i + 1] == 'X' || d[i + 1] == 'x') &&
            hexval(d[i + 2]) >= 0 && hexval(d[i + 3]) >= 0) {
            int v = hexval(d[i + 2]) << 4 | hexval(d[i + 3]);
            if (v != 0x00 && v != 0xFF) { attr = v; sgr(&b, attr); }  /* save/restore: ignored */
            i += 3;
            continue;
        }
        if (fmt == AD_FMT_PCBOARD && ch == '@' && i + 4 < len && memcmp(d + i, "@CLS@", 5) == 0) {
            if (!any) clear = 1;
            i += 4;
            continue;
        }
        if (fmt == AD_FMT_PIPE && ch == '|' && i + 2 < len) {
            if (isdigit(d[i + 1]) && isdigit(d[i + 2])) {
                int v = (d[i + 1] - '0') * 10 + (d[i + 2] - '0');
                if (v < 16) attr = (attr & 0xF0) | v;
                else if (v < 24) attr = (attr & 0x0F) | (v - 16) << 4;
                else if (v < 32) { attr = (attr & 0x0F) | (v - 24 + 8) << 4; ice = 1; }
                if (v < 32) { sgr(&b, attr); i += 2; continue; }
            }
            if (d[i + 1] == 'C' && d[i + 2] == 'L') {
                if (!any) clear = 1;
                i += 2;
                continue;
            }
        }
        if (fmt == AD_FMT_CTRLA && ch == 0x01 && i + 1 < len) {
            unsigned char k = d[++i];
            const char *pos = strchr(SYNC_FG, toupper(k));
            if (k && pos) attr = (attr & 0xF8) | (int)(pos - SYNC_FG);
            else if (k >= '0' && k <= '7') {
                static const int ANSI_TO_PC[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };
                attr = (attr & 0x8F) | ANSI_TO_PC[k - '0'] << 4;
            }
            else if (k == 'H' || k == 'h') attr |= 0x08;
            else if (k == 'I' || k == 'i') attr |= 0x80;
            else if (k == 'E' || k == 'e') { attr |= 0x80; ice = 1; }
            else if (k == 'N' || k == 'n') attr = 0x07;
            else if ((k == 'L' || k == 'l') && !any) clear = 1;
            sgr(&b, attr);
            continue;
        }
        if (ch != '\r' && ch != '\n') any = 1;
        bputc(&b, ch);
    }
    if (b.oom) {
        free(b.p);
        snprintf(err, errsz, "out of memory");
        return 0;
    }
    ok = ad_ans_decode(b.p ? b.p : (const unsigned char *)"", b.n, c, meta, err, errsz);
    free(b.p);
    if (ok) {
        meta->clear_screen = clear;
        if (ice) c->ice = 1;
    }
    return ok;
}

int ad_fmt_decode(const unsigned char *data, size_t len, int fmt, AdCanvas *c, AdSauce *meta,
                  char *err, size_t errsz) {
    int ok;
    memset(meta, 0, sizeof(*meta));
    if (fmt == AD_FMT_PNG) {
        snprintf(err, errsz, "that's an image -- use Import image");
        return 0;
    }
    if (len >= 5 && memcmp(data, "XBIN\x1A", 5) == 0) fmt = AD_FMT_XBIN;
    /* .asc / .txt / .ans files carrying BBS color codes instead of ANSI */
    if (fmt == AD_FMT_ANSI || fmt == AD_FMT_ASCII) {
        if (!memchr(data, 0x1B, len > 65536 ? 65536 : len)) {
            if (count_codes(data, len, AD_FMT_PCBOARD) >= 3) fmt = AD_FMT_PCBOARD;
            else if (count_codes(data, len, AD_FMT_CTRLA) >= 3) fmt = AD_FMT_CTRLA;
            else if (count_codes(data, len, AD_FMT_PIPE) >= 3) fmt = AD_FMT_PIPE;
        }
    }
    switch (fmt) {
        case AD_FMT_BIN: ok = dec_bin(data, len, c, meta, err, errsz); break;
        case AD_FMT_XBIN: ok = dec_xbin(data, len, c, meta, err, errsz); break;
        case AD_FMT_PCBOARD: case AD_FMT_PIPE: case AD_FMT_CTRLA:
            ok = dec_bbs(data, len, fmt, c, meta, err, errsz);
            break;
        default: {
            int has_esc = memchr(data, 0x1B, len) != NULL;
            ok = ad_ans_decode(data, len, c, meta, err, errsz);
            /* an .asc with ANSI color codes is really ANSI */
            if (fmt == AD_FMT_ASCII && has_esc) fmt = AD_FMT_ANSI;
            break;
        }
    }
    if (ok) meta->format = fmt;
    return ok;
}
