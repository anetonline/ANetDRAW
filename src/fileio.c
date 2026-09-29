/* ANSI + SAUCE save/load -- see fileio.h for the references this
 * follows. */
#include "../include/fileio.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

/* PC color <-> ANSI SGR color offset (blue/red and cyan/yellow swap). */
static const int PC_TO_ANSI[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };
static const int ANSI_TO_PC[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };

/* ------------------------------------------------------------ growable buffer */

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

/* ------------------------------------------------------------ encode */

static int blank(AdCell c) {
    return c.ch == ' ' && AD_ATTR_BG(c.attr) == 0;
}

/* Glyphs that would break the file (LF, CR, the EOF byte, ESC) become
   look-alikes -- the same substitutions Moebius makes on save. */
static unsigned char safe_glyph(unsigned char ch) {
    switch (ch) {
        case 10: return 9;
        case 13: return 14;
        case 26: return 16;
        case 27: return 17;
        default: return ch;
    }
}

static void put_le16(unsigned char *p, unsigned v) {
    p[0] = (unsigned char)(v & 0xFF);
    p[1] = (unsigned char)((v >> 8) & 0xFF);
}

static void put_field(unsigned char *p, const char *s, size_t width) {
    size_t i, n = strlen(s);
    for (i = 0; i < width; i++) p[i] = (unsigned char)(i < n ? s[i] : ' ');
}

const long AD_SPEEDS[AD_SPEED_COUNT] = {
    0, 300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 76800, 115200
};

size_t ad_ans_encode(const AdCanvas *c, const AdSauce *meta, unsigned char **out) {
    Buf b = { NULL, 0, 0, 0 };
    int x, y, rows, w = c->w;
    int cur_fg = 7, cur_bg = 0, cur_bold = 0, cur_blink = 0;
    size_t art_len;
    unsigned char rec[128];
    char date[16];

    /* A canvas the artist sized is saved at that size; a still-growing
       one is saved down to its last row that has anything on it -- but
       never shorter than a standard 25-line screen, or reopening a small
       sketch would give a canvas too short to keep drawing on. */
    rows = c->h;
    if (!c->fixed) {
        int ew, eh;
        ad_canvas_extent(c, &ew, &eh);
        rows = eh > AD_CANVAS_MIN_H ? eh : AD_CANVAS_MIN_H;
    }

    if (meta && meta->speed > 0 && meta->speed < AD_SPEED_COUNT) {
        char seq[16];
        snprintf(seq, sizeof(seq), "\x1b[0;%d*r", meta->speed);
        bputs(&b, seq);
    }
    bputs(&b, "\x1b[0m");
    if (meta && meta->clear_screen) bputs(&b, "\x1b[2J\x1b[1;1H");
    for (y = 0; y < rows; y++) {
        int last = -1;
        for (x = 0; x < w; x++)
            if (!blank(ad_canvas_get(c, x, y))) last = x;
        for (x = 0; x <= last; x++) {
            AdCell cell = ad_canvas_get(c, x, y);
            int fg = AD_ATTR_FG(cell.attr), bg = AD_ATTR_BG(cell.attr);
            int bold = fg > 7, blink = bg > 7;
            char seq[40];
            int k = 0;
            fg &= 7;
            bg &= 7;
            if (blank(cell)) fg = cur_fg;  /* a blank only needs its background */
            seq[0] = 0;
            if ((cur_bold && !bold) || (cur_blink && !blink)) {
                k += sprintf(seq + k, "0;");
                cur_fg = 7; cur_bg = 0; cur_bold = cur_blink = 0;
                if (blank(cell)) fg = 7;
            }
            if (bold && !cur_bold) { k += sprintf(seq + k, "1;"); cur_bold = 1; }
            if (blink && !cur_blink) { k += sprintf(seq + k, "5;"); cur_blink = 1; }
            if (fg != cur_fg) { k += sprintf(seq + k, "%d;", 30 + PC_TO_ANSI[fg]); cur_fg = fg; }
            if (bg != cur_bg) { k += sprintf(seq + k, "%d;", 40 + PC_TO_ANSI[bg]); cur_bg = bg; }
            if (k) {
                seq[k - 1] = 'm';  /* the last ';' becomes the final byte */
                bputs(&b, "\x1b[");
                bput(&b, seq, (size_t)k);
            }
            bputc(&b, safe_glyph(cell.ch));
        }
        /* A row that ends in blanks is trimmed and ends with CR LF; a
           full-width row relies on the viewer wrapping at the SAUCE
           width (Moebius's convention -- and it keeps clear of the
           column-80 double-wrap). The last row needs neither. */
        if (last < w - 1 && y < rows - 1) bputs(&b, "\r\n");
    }
    if (meta && meta->speed > 0 && meta->speed < AD_SPEED_COUNT)
        bputs(&b, "\x1b[0;0*r");  /* back to full speed for whatever comes next */
    art_len = b.n;

    memset(rec, 0, sizeof(rec));
    memcpy(rec, "SAUCE00", 7);
    put_field(rec + 7, meta ? meta->title : "", AD_SAUCE_TITLE_LEN);
    put_field(rec + 42, meta ? meta->author : "", AD_SAUCE_AUTHOR_LEN);
    put_field(rec + 62, meta ? meta->group : "", AD_SAUCE_GROUP_LEN);
    {
        time_t now = time(NULL);
        struct tm *tm = localtime(&now);
        if (tm) strftime(date, sizeof(date), "%Y%m%d", tm);
        else strcpy(date, "19700101");
        put_field(rec + 82, date, 8);
    }
    rec[90] = (unsigned char)(art_len & 0xFF);
    rec[91] = (unsigned char)((art_len >> 8) & 0xFF);
    rec[92] = (unsigned char)((art_len >> 16) & 0xFF);
    rec[93] = (unsigned char)((art_len >> 24) & 0xFF);
    rec[94] = 1;                           /* DataType: Character */
    rec[95] = 1;                           /* FileType: ANSi */
    put_le16(rec + 96, (unsigned)w);       /* TInfo1: width */
    put_le16(rec + 98, (unsigned)rows);    /* TInfo2: lines */
    rec[104] = 0;                          /* no comment block */
    /* TFlags: iCE (bit 0), 8-pixel letter spacing (bits 1-2 = 01),
       square-pixel aspect (bits 3-4 = 10) -- the same bits Moebius
       writes for an 8px font. */
    rec[105] = (unsigned char)((c->ice ? 1 : 0) | (1 << 1) | (1 << 4));
    memcpy(rec + 106, "IBM VGA", 7);       /* TInfoS: font, zero-padded */

    bputc(&b, 0x1A);                       /* EOF, then the record */
    bput(&b, rec, sizeof(rec));
    if (b.oom) {
        free(b.p);
        *out = NULL;
        return 0;
    }
    *out = b.p;
    return b.n;
}

int ad_ans_save(const char *path, const AdCanvas *c, const AdSauce *meta,
                char *err, size_t errsz) {
    unsigned char *data = NULL;
    size_t n = ad_ans_encode(c, meta, &data);
    char tmp[1100];
    FILE *f;

    if (!n) {
        snprintf(err, errsz, "out of memory");
        return 0;
    }
    /* write beside the target, then rename over it: a crash or a
       hangup mid-save never leaves a half-written file behind */
    snprintf(tmp, sizeof(tmp), "%s.tmp%ld", path, (long)time(NULL));
    f = fopen(tmp, "wb");
    if (!f) {
        snprintf(err, errsz, "can't write there (%s)", strerror(errno));
        free(data);
        return 0;
    }
    if (fwrite(data, 1, n, f) != n || fflush(f) != 0) {
        snprintf(err, errsz, "write failed (%s)", strerror(errno));
        fclose(f);
        remove(tmp);
        free(data);
        return 0;
    }
    fclose(f);
    free(data);
#ifdef _WIN32
    if (!MoveFileExA(tmp, path, MOVEFILE_REPLACE_EXISTING)) {
        snprintf(err, errsz, "couldn't replace the file");
        remove(tmp);
        return 0;
    }
#else
    if (rename(tmp, path) != 0) {
        snprintf(err, errsz, "couldn't replace the file (%s)", strerror(errno));
        remove(tmp);
        return 0;
    }
#endif
    return 1;
}

/* ------------------------------------------------------------ decode */

static void get_field(char *dst, const unsigned char *src, size_t width) {
    size_t i, n = width;
    for (i = 0; i < width; i++) dst[i] = (char)(src[i] ? src[i] : ' ');
    while (n > 0 && dst[n - 1] == ' ') n--;
    dst[n] = 0;
}

/* The screen model of Moebius's parser (ansi.js Screen), with one
   deliberate difference: wrapping at the right edge is DEFERRED (VT
   style), so both conventions load right -- full-width rows with no
   newline (Moebius, and ours) and full-width rows followed by CR LF
   (TheDraw and friends), which an immediate-wrap parser turns into
   double spacing. */
typedef struct {
    AdCanvas *c;
    int w;
    int x, y, top, bottom;
    int wrap_pending;
    int fg, bg, bold, blink, inverse;  /* fg/bg: ANSI 0-7 */
    int save_x, save_y, saved;
    int max_y;
} Scr;

static void scr_reset_attrs(Scr *s) {
    s->fg = 7; s->bg = 0; s->bold = s->blink = s->inverse = 0;
}

static void scr_newline(Scr *s) {
    s->x = 0;
    s->y++;
    s->wrap_pending = 0;
    if (s->y > s->bottom) { s->top++; s->bottom++; }
}

static void scr_put(Scr *s, unsigned char ch) {
    int pfg, pbg;
    if (s->wrap_pending) scr_newline(s);
    pfg = ANSI_TO_PC[s->fg & 7] + (s->bold ? 8 : 0);
    pbg = ANSI_TO_PC[s->bg & 7] + (s->blink ? 8 : 0);
    if (s->inverse) {
        int t = pfg;
        pfg = pbg;
        pbg = t;
    }
    if (s->y < AD_CANVAS_MAX_H) {
        ad_canvas_set(s->c, s->x, s->y, ch, AD_ATTR(pfg, pbg));
        if (s->y > s->max_y) s->max_y = s->y;
    }
    s->x++;
    if (s->x >= s->w) {
        s->x = s->w - 1;
        s->wrap_pending = 1;
    }
}

static void scr_sgr(Scr *s, const int *v, int n) {
    int i;
    if (n == 0) { scr_reset_attrs(s); return; }
    for (i = 0; i < n; i++) {
        int p = v[i];
        if (p == 0) scr_reset_attrs(s);
        else if (p == 1) s->bold = 1;
        else if (p == 5 || p == 6) s->blink = 1;
        else if (p == 7) s->inverse = 1;
        else if (p == 21 || p == 22) s->bold = 0;
        else if (p == 25) s->blink = 0;
        else if (p == 27) s->inverse = 0;
        else if (p >= 30 && p <= 37) s->fg = p - 30;
        else if (p == 39) s->fg = 7;
        else if (p >= 40 && p <= 47) s->bg = p - 40;
        else if (p == 49) s->bg = 0;
        else if (p >= 90 && p <= 97) { s->fg = p - 90; s->bold = 1; }
        else if (p >= 100 && p <= 107) { s->bg = p - 100; s->blink = 1; }
        else if (p == 38 || p == 48) break;  /* 256/true color: not representable, skip the rest */
    }
}

static void scr_csi(Scr *s, char final, const int *v, int n) {
    int a = n > 0 && v[0] > 0 ? v[0] : 1;
    switch (final) {
        case 'A': s->y -= a; if (s->y < s->top) s->y = s->top; s->wrap_pending = 0; break;
        case 'B': s->y += a; if (s->y > s->bottom) s->y = s->bottom; s->wrap_pending = 0; break;
        case 'C': s->x += a; if (s->x > s->w - 1) s->x = s->w - 1; s->wrap_pending = 0; break;
        case 'D': s->x -= a; if (s->x < 0) s->x = 0; s->wrap_pending = 0; break;
        case 'H': case 'f': {
            int row = n > 0 && v[0] > 0 ? v[0] : 1, col = n > 1 && v[1] > 0 ? v[1] : 1;
            s->y = row - 1 + s->top;
            s->x = col - 1;
            if (s->x > s->w - 1) s->x = s->w - 1;
            s->wrap_pending = 0;
            break;
        }
        case 'm': scr_sgr(s, v, n); break;
        case 's': s->save_x = s->x; s->save_y = s->y; s->saved = 1; break;
        case 'u': if (s->saved) { s->x = s->save_x; s->y = s->save_y; s->wrap_pending = 0; } break;
        default: break;  /* J, K, and everything else: ignored, like Moebius */
    }
}

int ad_ans_decode(const unsigned char *data, size_t len, AdCanvas *c, AdSauce *meta,
                  char *err, size_t errsz) {
    size_t art_len = len, i;
    int width = AD_CANVAS_W, height = 0, ice = 0;
    Scr s;

    memset(meta, 0, sizeof(*meta));
    if (len >= 128 && memcmp(data + len - 128, "SAUCE00", 7) == 0) {
        const unsigned char *r = data + len - 128;
        unsigned long fsz = (unsigned long)r[90] | ((unsigned long)r[91] << 8) |
                            ((unsigned long)r[92] << 16) | ((unsigned long)r[93] << 24);
        meta->has_sauce = 1;
        get_field(meta->title, r + 7, AD_SAUCE_TITLE_LEN);
        get_field(meta->author, r + 42, AD_SAUCE_AUTHOR_LEN);
        get_field(meta->group, r + 62, AD_SAUCE_GROUP_LEN);
        get_field(meta->date, r + 82, 8);
        if (r[94] == 1) {  /* Character: TInfo1 = width, TInfo2 = lines */
            int tw = r[96] | (r[97] << 8), th = r[98] | (r[99] << 8);
            if (tw > 0) width = tw;
            if (th > 0) height = th;
            ice = r[105] & 1;
        }
        art_len = len - 128;
        if (fsz > 0 && fsz < art_len) art_len = fsz;
    }
    if (width > AD_CANVAS_MAX_W) width = AD_CANVAS_MAX_W;
    if (height > AD_CANVAS_MAX_H) height = AD_CANVAS_MAX_H;

    /* start from a blank, growable canvas of the right width */
    if (!ad_canvas_resize(c, width, height > 0 ? height : AD_CANVAS_MIN_H)) {
        snprintf(err, errsz, "out of memory");
        return 0;
    }
    c->fixed = 0;
    c->h = 1;
    ad_canvas_clear(c);

    memset(&s, 0, sizeof(s));
    s.c = c;
    s.w = width;
    s.bottom = 24;
    s.max_y = -1;
    scr_reset_attrs(&s);

    for (i = 0; i < art_len; i++) {
        unsigned char ch = data[i];
        if (ch == 0x1A) break;  /* EOF: SAUCE-aware readers stop here */
        if (ch == 0x1B && i + 1 < art_len && data[i + 1] == '[') {
            int v[16], n = 0, cur = -1, priv = 0;
            size_t j = i + 2;
            char final = 0, inter = 0;
            for (; j < art_len && j < i + 64; j++) {
                unsigned char d = data[j];
                if (d >= '0' && d <= '9') {
                    cur = (cur < 0 ? 0 : cur) * 10 + (d - '0');
                    if (cur > 9999) cur = 9999;
                } else if (d == ';') {
                    if (n < 16) v[n++] = cur < 0 ? 0 : cur;
                    cur = -1;
                } else if (d == '?' || d == '=' || d == '<' || d == '>') {
                    priv = 1;
                } else if (d >= 0x40 && d <= 0x7E) {
                    final = (char)d;
                    break;
                } else if (d >= 0x20 && d <= 0x2F) {
                    inter = (char)d;
                } else if (d < 0x20 || d > 0x7E) {
                    break;  /* malformed: stop the sequence here */
                }
            }
            if (cur >= 0 && n < 16) v[n++] = cur;
            /* display options written before the art (see AdSauce) */
            if (s.max_y < 0 && s.x == 0 && final == 'r' && inter == '*' && n >= 2 &&
                v[1] > 0 && v[1] < AD_SPEED_COUNT)
                meta->speed = v[1];
            if (s.max_y < 0 && final == 'J' && !inter && n >= 1 && v[0] == 2)
                meta->clear_screen = 1;
            if (final && !priv && !inter) scr_csi(&s, final, v, n);
            i = final ? j : j - 1;
            continue;
        }
        if (ch == '\n') { scr_newline(&s); continue; }
        if (ch == '\r') { s.x = 0; s.wrap_pending = 0; continue; }
        scr_put(&s, ch);
    }

    if (height <= 0) {
        /* no SAUCE size: every row that was written, at least a screen */
        height = s.max_y + 1;
        if (height < AD_CANVAS_MIN_H) height = AD_CANVAS_MIN_H;
    }
    c->h = height;
    c->fixed = 1;
    c->ice = ice;
    return 1;
}

int ad_ans_load(const char *path, AdCanvas *c, AdSauce *meta, char *err, size_t errsz) {
    FILE *f = fopen(path, "rb");
    unsigned char *data;
    long len;
    int ok;
    if (!f) {
        snprintf(err, errsz, "can't open it (%s)", strerror(errno));
        return 0;
    }
    if (fseek(f, 0, SEEK_END) != 0 || (len = ftell(f)) < 0 || fseek(f, 0, SEEK_SET) != 0) {
        snprintf(err, errsz, "can't read it");
        fclose(f);
        return 0;
    }
    if (len > AD_MAX_FILE_BYTES) {
        snprintf(err, errsz, "too big (over %ld KB)", AD_MAX_FILE_BYTES / 1024);
        fclose(f);
        return 0;
    }
    data = (unsigned char *)malloc(len > 0 ? (size_t)len : 1);
    if (!data) {
        snprintf(err, errsz, "out of memory");
        fclose(f);
        return 0;
    }
    if (len > 0 && fread(data, 1, (size_t)len, f) != (size_t)len) {
        snprintf(err, errsz, "read failed");
        free(data);
        fclose(f);
        return 0;
    }
    fclose(f);
    ok = ad_ans_decode(data, (size_t)len, c, meta, err, errsz);
    free(data);
    return ok;
}
