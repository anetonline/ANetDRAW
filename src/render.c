#include "../include/render.h"
#include "../include/anetdraw.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* PC color index (0 black, 1 blue, 2 green, 3 cyan, 4 red, 5 magenta,
   6 brown, 7 light gray) -> ANSI SGR color offset (0 black, 1 red,
   2 green, 3 yellow, 4 blue, 5 magenta, 6 cyan, 7 white). Blue/red and
   cyan/yellow swap -- getting this wrong paints every drawing in the
   wrong colors while looking perfectly plausible. */
static const int PC_TO_ANSI[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };

void ad_sgr(unsigned char attr, int ice, char *buf, size_t bufsz) {
    int fg = AD_ATTR_FG(attr), bg = AD_ATTR_BG(attr);
    int bgcode = (bg >= 8 && ice) ? 100 + PC_TO_ANSI[bg & 7]
                                  : 40 + PC_TO_ANSI[bg & 7];
    snprintf(buf, bufsz, "\x1b[0;%s%s%d;%dm",
             fg >= 8 ? "1;" : "",
             (bg >= 8 && !ice) ? "5;" : "",
             30 + PC_TO_ANSI[fg & 7], bgcode);
}

unsigned char ad_display_glyph(unsigned char ch) {
    switch (ch) {
        case 0x00: return ' ';
        case 0x07: return 0xF9;  /* bullet        -> small bullet */
        case 0x08: return 0xDB;  /* inverse dot   -> full block */
        case 0x09: return 'o';   /* circle        -> o */
        case 0x0A: return 0xDB;  /* inverse ring  -> full block */
        case 0x0D: return 0x0E;  /* single note   -> double note */
        case 0x1B: return 0x11;  /* left arrow    -> left triangle */
        case 0x7F: return 0x1E;  /* house (DEL is swallowed by xterm) -> up triangle */
        default:   return ch;
    }
}

int ad_screen_init(AdScreen *s, int w, int h) {
    size_t n = (size_t)w * (size_t)h;
    s->w = w;
    s->h = h;
    s->curr = (AdCell *)malloc(n * sizeof(AdCell));
    s->prev = (AdCell *)malloc(n * sizeof(AdCell));
    s->full_redraw = 1;
    s->ice = 1;
    s->ice_sent = -1;
    s->cur_col = s->cur_row = -1;
    if (!s->curr || !s->prev) {
        free(s->curr); free(s->prev);
        s->curr = s->prev = NULL;
        return 0;
    }
    {
        size_t i;
        for (i = 0; i < n; i++) {
            s->curr[i].ch = ' ';
            s->curr[i].attr = AD_BLANK_ATTR;
        }
    }
    return 1;
}

void ad_screen_free(AdScreen *s) {
    free(s->curr); free(s->prev);
    s->curr = s->prev = NULL;
}

void ad_screen_full_redraw(AdScreen *s) { s->full_redraw = 1; }

void ad_screen_set_ice(AdScreen *s, int ice) {
    ice = ice ? 1 : 0;
    if (s->ice != ice) {
        s->ice = ice;
        /* Every bright-bg cell's SGR changes meaning -- resend them all. */
        s->full_redraw = 1;
    }
}

void ad_screen_put(AdScreen *s, int col, int row, unsigned char ch, unsigned char attr) {
    AdCell *c;
    if (col < 0 || col >= s->w || row < 0 || row >= s->h) return;
    c = &s->curr[row * s->w + col];
    c->ch = ch;
    c->attr = attr;
}

void ad_screen_puts(AdScreen *s, int col, int row, const char *str, unsigned char attr) {
    for (; *str && col < s->w; ++str, ++col)
        ad_screen_put(s, col, row, (unsigned char)*str, attr);
}

/* Output accumulator -- one ad_dout() per ~8KB instead of per cell. */
typedef struct {
    char buf[8192];
    size_t n;
} OutBuf;

static size_t g_flush_bytes = 0;  /* bytes sent by the current flush (trace) */

static void ob_flush(OutBuf *o) {
    if (o->n) {
        o->buf[o->n] = 0;
        /* in UTF-8 mode the buffer is already UTF-8 (see ob_glyph) */
        if (ad_output_utf8()) ad_dout_utf8(o->buf);
        else ad_dout(o->buf);
        g_flush_bytes += o->n;
        o->n = 0;
    }
}

static void ob_puts(OutBuf *o, const char *str) {
    size_t len = strlen(str);
    if (o->n + len + 1 >= sizeof(o->buf)) ob_flush(o);
    memcpy(o->buf + o->n, str, len);
    o->n += len;
}

static void ob_putc(OutBuf *o, unsigned char ch) {
    /* ad_display_glyph() never returns 0x00, so the NUL-terminated
       ad_dout() path can't truncate, and never returns 0x0A, so
       ad_dout()'s CRLF fix-up never fires inside the canvas. */
    if (o->n + 2 >= sizeof(o->buf)) ob_flush(o);
    o->buf[o->n++] = (char)ch;
}

/* One canvas cell's glyph. A UTF-8 terminal (local mode) gets the real
   CP437 picture glyph for every byte, 0x01-0x1F and 0x7F included; a
   CP437 terminal gets the byte, with the few SyncTERM acts on swapped
   for look-alikes (ad_display_glyph). */
static void ob_glyph(OutBuf *o, unsigned char ch) {
    if (ad_output_utf8()) {
        char u[4];
        size_t n = ad_cp437_utf8(ch, u);
        if (o->n + n + 1 >= sizeof(o->buf)) ob_flush(o);
        memcpy(o->buf + o->n, u, n);
        o->n += n;
    } else {
        ob_putc(o, ad_display_glyph(ch));
    }
}

void ad_screen_flush(AdScreen *s, int cur_col, int cur_row) {
    static OutBuf o;
    int tc = -1, tr = -1;   /* where the terminal cursor is, -1 = unknown */
    int last_attr = -1;     /* SGR last sent this flush, -1 = unknown */
    int row, col;
    char seq[48];

    if (!s->curr) return;
    o.n = 0;
    g_flush_bytes = 0;

    if (s->ice_sent != s->ice) {
        ob_puts(&o, s->ice ? "\x1b[?33h" : "\x1b[?33l");
        s->ice_sent = s->ice;
    }
    if (s->full_redraw) ob_puts(&o, "\x1b[0m\x1b[2J");

    for (row = 0; row < s->h; row++) {
        for (col = 0; col < s->w; col++) {
            int i = row * s->w + col;
            AdCell *c = &s->curr[i];
            AdCell *p = &s->prev[i];

            /* Bottom-right cell: writing it scrolls the screen on
               SyncTERM (immediate wrap). Never touch it. */
            if (row == s->h - 1 && col == s->w - 1) continue;
            if (!s->full_redraw && c->ch == p->ch && c->attr == p->attr) continue;

            if (tr != row || tc != col) {
                snprintf(seq, sizeof(seq), "\x1b[%d;%dH", row + 1, col + 1);
                ob_puts(&o, seq);
            }
            if (last_attr != c->attr) {
                ad_sgr(c->attr, s->ice, seq, sizeof(seq));
                ob_puts(&o, seq);
                last_attr = c->attr;
            }
            ob_glyph(&o, c->ch);
            *p = *c;

            tr = row;
            tc = col + 1;
            /* Last column: SyncTERM has already wrapped, VT terminals
               are in pending-wrap -- the position is ambiguous, so
               always reposition before the next write. */
            if (tc >= s->w) tr = tc = -1;
        }
    }

    /* Park the cursor -- unless nothing was drawn and it's already
       there. (SyncTERM reports every pixel of mouse movement, so most
       drag events land in the same cell; they used to cost a useless
       7-byte cursor move each.) */
    if (cur_col >= 0 && cur_row >= 0 &&
        (o.n > 0 || cur_col != s->cur_col || cur_row != s->cur_row)) {
        snprintf(seq, sizeof(seq), "\x1b[%d;%dH", cur_row + 1, cur_col + 1);
        ob_puts(&o, seq);
    }
    s->cur_col = cur_col;
    s->cur_row = cur_row;
    ob_flush(&o);
    s->full_redraw = 0;
    if (g_flush_bytes) ad_trace("OUT %lu bytes", (unsigned long)g_flush_bytes);
}

void ad_screen_restore_terminal(void) {
    /* blink mode, mouse reporting (sent even if it was never turned
       on -- harmless), colors */
    ad_dout("\x1b[?33l\x1b[?1006l\x1b[?1002l\x1b[0 q\x1b[0m");
}
