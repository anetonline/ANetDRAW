#include "../include/tools.h"
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------ surface */

static AdCell mkcell(unsigned char ch, unsigned char attr) {
    AdCell c;
    c.ch = ch;
    c.attr = attr;
    return c;
}

AdCell ad_surf_get(const AdSurface *s, int x, int y) {
    if (!s->commit && s->ov && y >= s->ov_top && y < s->ov_top + s->ov_h &&
        x >= 0 && x < s->canvas->w) {
        size_t i = (size_t)(y - s->ov_top) * s->canvas->w + x;
        if (s->ov_mask[i]) return s->ov[i];
    }
    return ad_canvas_get(s->canvas, x, y);
}

/* The one place a cell actually changes. */
static void put_raw(AdSurface *s, int x, int y, AdCell cell) {
    AdCanvas *c = s->canvas;
    if (x < 0 || x >= c->w || y < 0 || y >= AD_CANVAS_MAX_H) return;
    if (s->commit) {
        AdCell before = ad_canvas_get(c, x, y);
        if (before.ch == cell.ch && before.attr == cell.attr && y < c->h) return;
        if (s->hook) s->hook(s->hook_ctx, x, y, before, cell);
        ad_canvas_set(c, x, y, cell.ch, cell.attr);
    } else if (s->ov && y >= s->ov_top && y < s->ov_top + s->ov_h) {
        size_t i = (size_t)(y - s->ov_top) * c->w + x;
        s->ov[i] = cell;
        s->ov_mask[i] = 1;
    }
}

void ad_surf_clear_overlay(AdSurface *s) {
    if (s->ov_mask) memset(s->ov_mask, 0, (size_t)s->ov_h * s->canvas->w);
}

void ad_surf_put(AdSurface *s, int x, int y, unsigned char ch, unsigned char attr) {
    int w = s->canvas->w, h = s->canvas->h;
    put_raw(s, x, y, mkcell(ch, attr));
    if (s->mirror & AD_MIRROR_H)
        put_raw(s, w - 1 - x, y, mkcell(ad_mirror_glyph_h(ch), attr));
    if (s->mirror & AD_MIRROR_V)
        put_raw(s, x, h - 1 - y, mkcell(ad_mirror_glyph_v(ch), attr));
    if ((s->mirror & AD_MIRROR_HV) == AD_MIRROR_HV)
        put_raw(s, w - 1 - x, h - 1 - y,
                mkcell(ad_mirror_glyph_v(ad_mirror_glyph_h(ch)), attr));
}

/* ------------------------------------------------------------ mirroring */

/* CP437 glyph pairs that are left/right (or top/bottom) images of each
   other. Box-drawing corners, tees, half blocks, brackets, arrows. */
static const unsigned char MIRROR_H_PAIRS[][2] = {
    {0xDD, 0xDE}, {0xDA, 0xBF}, {0xC0, 0xD9}, {0xC3, 0xB4}, {0xC9, 0xBB},
    {0xC8, 0xBC}, {0xCC, 0xB9}, {0xD5, 0xB8}, {0xD4, 0xBE}, {0xD6, 0xB7},
    {0xD3, 0xBD}, {0xC6, 0xB5}, {0xC7, 0xB6}, {'/', '\\'}, {'(', ')'},
    {'<', '>'}, {'[', ']'}, {'{', '}'}, {0x10, 0x11}, {0x1A, 0x1B},
};
static const unsigned char MIRROR_V_PAIRS[][2] = {
    {0xDF, 0xDC}, {0xDA, 0xC0}, {0xBF, 0xD9}, {0xC2, 0xC1}, {0xC9, 0xC8},
    {0xBB, 0xBC}, {0xCB, 0xCA}, {0xD5, 0xD4}, {0xB8, 0xBE}, {0xD6, 0xD3},
    {0xB7, 0xBD}, {0xD1, 0xCF}, {0xD2, 0xD0}, {'/', '\\'}, {0x1E, 0x1F},
    {0x18, 0x19},
};

static unsigned char mirror_lookup(const unsigned char (*pairs)[2], size_t n, unsigned char ch) {
    size_t i;
    for (i = 0; i < n; i++) {
        if (pairs[i][0] == ch) return pairs[i][1];
        if (pairs[i][1] == ch) return pairs[i][0];
    }
    return ch;
}

unsigned char ad_mirror_glyph_h(unsigned char ch) {
    return mirror_lookup(MIRROR_H_PAIRS, sizeof(MIRROR_H_PAIRS) / sizeof(MIRROR_H_PAIRS[0]), ch);
}

unsigned char ad_mirror_glyph_v(unsigned char ch) {
    return mirror_lookup(MIRROR_V_PAIRS, sizeof(MIRROR_V_PAIRS) / sizeof(MIRROR_V_PAIRS[0]), ch);
}

/* ------------------------------------------------------------ pixels */

#define G_UPPER 0xDF
#define G_LOWER 0xDC
#define G_FULL  0xDB

void ad_cell_to_pixels(AdCell c, int *top, int *bot) {
    int fg = AD_ATTR_FG(c.attr), bg = AD_ATTR_BG(c.attr);
    switch (c.ch) {
        case G_UPPER: *top = fg; *bot = bg; break;
        case G_LOWER: *top = bg; *bot = fg; break;
        case G_FULL:  *top = fg; *bot = fg; break;
        /* anything else -- space or a text/line glyph -- is its
           background color in both halves (painting a pixel over text
           replaces the text) */
        default:      *top = bg; *bot = bg; break;
    }
}

/* Prefers encodings whose background is 0-7, so pixel art stays
   correct even with iCE off; two bright pixels in one cell need iCE
   (without it the bottom one is darkened to its normal-intensity
   color -- the closest non-blinking choice). */
AdCell ad_pixels_to_cell(int top, int bot, int ice) {
    if (top == bot) {
        if (top == 0) return mkcell(' ', AD_ATTR(7, 0));
        return mkcell(G_FULL, AD_ATTR(top, 0));
    }
    if (bot < 8) return mkcell(G_UPPER, AD_ATTR(top, bot));
    if (top < 8) return mkcell(G_LOWER, AD_ATTR(bot, top));
    return mkcell(G_UPPER, AD_ATTR(top, ice ? bot : (bot & 7)));
}

static void pixel_raw(AdSurface *s, int x, int py, int color) {
    int top, bot, y = py >> 1;
    AdCell c;
    if (x < 0 || x >= s->canvas->w || py < 0 || y >= AD_CANVAS_MAX_H) return;
    ad_cell_to_pixels(ad_surf_get(s, x, y), &top, &bot);
    if (py & 1) bot = color; else top = color;
    c = ad_pixels_to_cell(top, bot, s->canvas->ice);
    put_raw(s, x, y, c);
}

void ad_surf_pixel(AdSurface *s, int x, int py, int color) {
    int w = s->canvas->w, ph = s->canvas->h * 2;
    pixel_raw(s, x, py, color);
    if (s->mirror & AD_MIRROR_H) pixel_raw(s, w - 1 - x, py, color);
    if (s->mirror & AD_MIRROR_V) pixel_raw(s, x, ph - 1 - py, color);
    if ((s->mirror & AD_MIRROR_HV) == AD_MIRROR_HV) pixel_raw(s, w - 1 - x, ph - 1 - py, color);
}

/* ------------------------------------------------------------ shapes */

static void order(int *a, int *b) {
    if (*a > *b) { int t = *a; *a = *b; *b = t; }
}

typedef void (*PlotFn)(AdSurface *s, int x, int y, const void *arg);

typedef struct { unsigned char ch, attr; } BrushArg;

static void plot_brush(AdSurface *s, int x, int y, const void *arg) {
    const BrushArg *b = (const BrushArg *)arg;
    ad_surf_put(s, x, y, b->ch, b->attr);
}

static void plot_pixel(AdSurface *s, int x, int y, const void *arg) {
    ad_surf_pixel(s, x, y, *(const int *)arg);
}

static void bresenham(AdSurface *s, int x0, int y0, int x1, int y1, PlotFn plot, const void *arg) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        plot(s, x0, y0, arg);
        if (x0 == x1 && y0 == y1) break;
        {
            int e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    }
}

void ad_draw_line(AdSurface *s, int x0, int y0, int x1, int y1,
                  unsigned char ch, unsigned char attr) {
    BrushArg b;
    b.ch = ch;
    b.attr = attr;
    bresenham(s, x0, y0, x1, y1, plot_brush, &b);
}

/* ------------------------------------------------------------ colorize */

static unsigned char recolored(unsigned char attr, int fg, int bg, int mode) {
    int f = AD_ATTR_FG(attr), b = AD_ATTR_BG(attr);
    if (mode != AD_RECOLOR_BG) f = fg;
    if (mode != AD_RECOLOR_FG) b = bg;
    return AD_ATTR(f, b);
}

static void recolor_one(AdSurface *s, int x, int y, int fg, int bg, int mode) {
    AdCell c;
    if (x < 0 || x >= s->canvas->w || y < 0 || y >= AD_CANVAS_MAX_H) return;
    c = ad_surf_get(s, x, y);
    put_raw(s, x, y, mkcell(c.ch, recolored(c.attr, fg, bg, mode)));
}

void ad_surf_recolor(AdSurface *s, int x, int y, int fg, int bg, int mode) {
    int w = s->canvas->w, h = s->canvas->h;
    /* every mirrored copy keeps its own glyph; only the colors change */
    recolor_one(s, x, y, fg, bg, mode);
    if (s->mirror & AD_MIRROR_H) recolor_one(s, w - 1 - x, y, fg, bg, mode);
    if (s->mirror & AD_MIRROR_V) recolor_one(s, x, h - 1 - y, fg, bg, mode);
    if ((s->mirror & AD_MIRROR_HV) == AD_MIRROR_HV) recolor_one(s, w - 1 - x, h - 1 - y, fg, bg, mode);
}

typedef struct { int fg, bg, mode; } RecolorArg;

static void plot_recolor(AdSurface *s, int x, int y, const void *arg) {
    const RecolorArg *r = (const RecolorArg *)arg;
    ad_surf_recolor(s, x, y, r->fg, r->bg, r->mode);
}

void ad_recolor_line(AdSurface *s, int x0, int y0, int x1, int y1, int fg, int bg, int mode) {
    RecolorArg r;
    r.fg = fg;
    r.bg = bg;
    r.mode = mode;
    bresenham(s, x0, y0, x1, y1, plot_recolor, &r);
}

void ad_draw_line_pixels(AdSurface *s, int x0, int py0, int x1, int py1, int color) {
    bresenham(s, x0, py0, x1, py1, plot_pixel, &color);
}

static void rect(AdSurface *s, int x0, int y0, int x1, int y1, int filled,
                 PlotFn plot, const void *arg) {
    int x, y;
    order(&x0, &x1);
    order(&y0, &y1);
    for (y = y0; y <= y1; y++)
        for (x = x0; x <= x1; x++)
            if (filled || y == y0 || y == y1 || x == x0 || x == x1)
                plot(s, x, y, arg);
}

static void line_box(AdSurface *s, int x0, int y0, int x1, int y1, int dbl, unsigned char attr) {
    /* corners TL TR BL BR, horizontal, vertical */
    static const unsigned char SINGLE[6] = { 0xDA, 0xBF, 0xC0, 0xD9, 0xC4, 0xB3 };
    static const unsigned char DOUBLE[6] = { 0xC9, 0xBB, 0xC8, 0xBC, 0xCD, 0xBA };
    const unsigned char *g = dbl ? DOUBLE : SINGLE;
    int x, y;
    order(&x0, &x1);
    order(&y0, &y1);
    if (y0 == y1) { for (x = x0; x <= x1; x++) ad_surf_put(s, x, y0, g[4], attr); return; }
    if (x0 == x1) { for (y = y0; y <= y1; y++) ad_surf_put(s, x0, y, g[5], attr); return; }
    for (x = x0 + 1; x < x1; x++) {
        ad_surf_put(s, x, y0, g[4], attr);
        ad_surf_put(s, x, y1, g[4], attr);
    }
    for (y = y0 + 1; y < y1; y++) {
        ad_surf_put(s, x0, y, g[5], attr);
        ad_surf_put(s, x1, y, g[5], attr);
    }
    ad_surf_put(s, x0, y0, g[0], attr);
    ad_surf_put(s, x1, y0, g[1], attr);
    ad_surf_put(s, x0, y1, g[2], attr);
    ad_surf_put(s, x1, y1, g[3], attr);
}

void ad_draw_box(AdSurface *s, int x0, int y0, int x1, int y1, AdBoxStyle style,
                 unsigned char ch, unsigned char attr, int color) {
    BrushArg b;
    b.ch = ch;
    b.attr = attr;
    switch (style) {
        case AD_BOX_BRUSH:        rect(s, x0, y0, x1, y1, 0, plot_brush, &b); break;
        case AD_BOX_BRUSH_FILLED: rect(s, x0, y0, x1, y1, 1, plot_brush, &b); break;
        case AD_BOX_SINGLE:       line_box(s, x0, y0, x1, y1, 0, attr); break;
        case AD_BOX_DOUBLE:       line_box(s, x0, y0, x1, y1, 1, attr); break;
        case AD_BOX_PIXEL:        rect(s, x0, y0, x1, y1, 0, plot_pixel, &color); break;
        case AD_BOX_PIXEL_FILLED: rect(s, x0, y0, x1, y1, 1, plot_pixel, &color); break;
        default: break;
    }
}

/* Point-in-ellipse over the bounding box's cell centers; radii get +0.5
   so the extreme cells sit on the curve instead of just outside it. */
static int in_ellipse(double x, double y, double cx, double cy, double rx, double ry) {
    double dx = (x - cx) / rx, dy = (y - cy) / ry;
    return dx * dx + dy * dy <= 1.0;
}

static void ellipse(AdSurface *s, int x0, int y0, int x1, int y1, int filled,
                    PlotFn plot, const void *arg) {
    double cx, cy, rx, ry;
    int x, y;
    order(&x0, &x1);
    order(&y0, &y1);
    cx = (x0 + x1) / 2.0;
    cy = (y0 + y1) / 2.0;
    rx = (x1 - x0) / 2.0 + 0.5;
    ry = (y1 - y0) / 2.0 + 0.5;
    for (y = y0; y <= y1; y++) {
        for (x = x0; x <= x1; x++) {
            if (!in_ellipse(x, y, cx, cy, rx, ry)) continue;
            /* outline = inside cells with a 4-neighbour outside */
            if (filled ||
                !in_ellipse(x - 1, y, cx, cy, rx, ry) || !in_ellipse(x + 1, y, cx, cy, rx, ry) ||
                !in_ellipse(x, y - 1, cx, cy, rx, ry) || !in_ellipse(x, y + 1, cx, cy, rx, ry))
                plot(s, x, y, arg);
        }
    }
}

void ad_draw_ellipse(AdSurface *s, int x0, int y0, int x1, int y1, AdEllipseStyle style,
                     unsigned char ch, unsigned char attr, int color) {
    BrushArg b;
    b.ch = ch;
    b.attr = attr;
    switch (style) {
        case AD_ELLIPSE_BRUSH:        ellipse(s, x0, y0, x1, y1, 0, plot_brush, &b); break;
        case AD_ELLIPSE_BRUSH_FILLED: ellipse(s, x0, y0, x1, y1, 1, plot_brush, &b); break;
        case AD_ELLIPSE_PIXEL:        ellipse(s, x0, y0, x1, y1, 0, plot_pixel, &color); break;
        case AD_ELLIPSE_PIXEL_FILLED: ellipse(s, x0, y0, x1, y1, 1, plot_pixel, &color); break;
        default: break;
    }
}

/* ------------------------------------------------------------ fill */

int ad_flood_fill(AdSurface *s, int x, int y, unsigned char ch, unsigned char attr,
                  int color_only) {
    AdCanvas *c = s->canvas;
    AdCell start;
    int w = c->w, h = c->h, changed = 0, sp = 0;
    int *stack;
    unsigned char *seen;

    if (!s->commit || x < 0 || x >= w || y < 0 || y >= h) return 0;
    start = ad_canvas_get(c, x, y);
    if (color_only ? start.attr == attr : (start.ch == ch && start.attr == attr)) return 0;

    stack = (int *)malloc((size_t)w * h * sizeof(int));
    seen = (unsigned char *)calloc((size_t)w * h, 1);
    if (!stack || !seen) { free(stack); free(seen); return 0; }

    stack[sp++] = y * w + x;
    seen[y * w + x] = 1;
    while (sp > 0) {
        int i = stack[--sp], cx = i % w, cy = i / w, k;
        AdCell cur = ad_canvas_get(c, cx, cy);
        static const int DX[4] = { 1, -1, 0, 0 }, DY[4] = { 0, 0, 1, -1 };
        put_raw(s, cx, cy, mkcell(color_only ? cur.ch : ch, attr));
        changed++;
        for (k = 0; k < 4; k++) {
            int nx = cx + DX[k], ny = cy + DY[k], ni;
            AdCell n;
            if (nx < 0 || nx >= w || ny < 0 || ny >= h) continue;
            ni = ny * w + nx;
            if (seen[ni]) continue;
            n = ad_canvas_get(c, nx, ny);
            if (color_only ? n.attr != start.attr : (n.ch != start.ch || n.attr != start.attr))
                continue;
            seen[ni] = 1;
            stack[sp++] = ni;  /* each cell pushed at most once: fits w*h */
        }
    }
    free(stack);
    free(seen);
    return changed;
}

/* ------------------------------------------------------------ shading */

static const unsigned char SHADE_RAMP[5] = { ' ', 0xB0, 0xB1, 0xB2, 0xDB };

void ad_shade_step(AdSurface *s, int x, int y, int fg, int dir) {
    AdCell c = ad_surf_get(s, x, y);
    int idx = 0, i, bg = AD_ATTR_BG(c.attr);
    for (i = 1; i < 5; i++)
        if (c.ch == SHADE_RAMP[i]) idx = i;
    /* shading over a different color (or over non-shade glyphs) starts
       fresh from the lightest step in the new color */
    if (idx > 0 && AD_ATTR_FG(c.attr) != fg) idx = 0;
    idx += dir;
    if (idx < 0) idx = 0;
    if (idx > 4) idx = 4;
    ad_surf_put(s, x, y, SHADE_RAMP[idx], AD_ATTR(idx == 0 ? AD_ATTR_FG(c.attr) : fg, bg));
}

/* ------------------------------------------------------------ clipboard */

void ad_clip_free(AdClipboard *cb) {
    free(cb->cells);
    cb->cells = NULL;
    cb->w = cb->h = 0;
}

int ad_clip_copy(AdClipboard *cb, const AdCanvas *c, int x0, int y0, int x1, int y1) {
    int x, y, w, h;
    AdCell *cells;
    order(&x0, &x1);
    order(&y0, &y1);
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= c->w) x1 = c->w - 1;
    if (y1 >= AD_CANVAS_MAX_H) y1 = AD_CANVAS_MAX_H - 1;
    if (x1 < x0 || y1 < y0) return 0;
    w = x1 - x0 + 1;
    h = y1 - y0 + 1;
    cells = (AdCell *)malloc((size_t)w * h * sizeof(AdCell));
    if (!cells) return 0;
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++)
            cells[y * w + x] = ad_canvas_get(c, x0 + x, y0 + y);
    ad_clip_free(cb);
    cb->cells = cells;
    cb->w = w;
    cb->h = h;
    return 1;
}

void ad_clip_flip_h(AdClipboard *cb) {
    int x, y;
    for (y = 0; y < cb->h; y++) {
        AdCell *row = cb->cells + (size_t)y * cb->w;
        for (x = 0; x < cb->w / 2; x++) {
            AdCell t = row[x];
            row[x] = row[cb->w - 1 - x];
            row[cb->w - 1 - x] = t;
        }
        for (x = 0; x < cb->w; x++) row[x].ch = ad_mirror_glyph_h(row[x].ch);
    }
}

void ad_clip_flip_v(AdClipboard *cb) {
    int x, y;
    for (y = 0; y < cb->h / 2; y++) {
        for (x = 0; x < cb->w; x++) {
            AdCell *a = &cb->cells[(size_t)y * cb->w + x];
            AdCell *b = &cb->cells[(size_t)(cb->h - 1 - y) * cb->w + x];
            AdCell t = *a;
            *a = *b;
            *b = t;
        }
    }
    for (x = 0; x < cb->w * cb->h; x++) cb->cells[x].ch = ad_mirror_glyph_v(cb->cells[x].ch);
}

void ad_clip_paste(AdSurface *s, const AdClipboard *cb, int x, int y, int transparent) {
    int cx, cy;
    if (!cb->cells) return;
    for (cy = 0; cy < cb->h; cy++) {
        for (cx = 0; cx < cb->w; cx++) {
            AdCell c = cb->cells[(size_t)cy * cb->w + cx];
            if (transparent && (c.ch == ' ' || c.ch == 0 || c.ch == 0xFF) && AD_ATTR_BG(c.attr) == 0)
                continue;
            ad_surf_put(s, x + cx, y + cy, c.ch, c.attr);
        }
    }
}
