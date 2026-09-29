#include "../include/canvas.h"
#include <stdlib.h>

int ad_canvas_init(AdCanvas *c, int w, int h) {
    if (w < 1) w = AD_CANVAS_W;
    if (h < 1) h = AD_CANVAS_MIN_H;
    if (h > AD_CANVAS_MAX_H) h = AD_CANVAS_MAX_H;
    c->w = w;
    c->h = h;
    c->ice = 1;
    c->fixed = 0;
    c->cells = (AdCell *)malloc((size_t)w * AD_CANVAS_MAX_H * sizeof(AdCell));
    if (!c->cells) return 0;
    ad_canvas_clear(c);
    return 1;
}

void ad_canvas_free(AdCanvas *c) {
    free(c->cells);
    c->cells = NULL;
}

void ad_canvas_clear(AdCanvas *c) {
    size_t i, n = (size_t)c->w * AD_CANVAS_MAX_H;
    for (i = 0; i < n; i++) {
        c->cells[i].ch = ' ';
        c->cells[i].attr = AD_BLANK_ATTR;
    }
}

AdCell ad_canvas_get(const AdCanvas *c, int x, int y) {
    AdCell blank;
    if (x < 0 || x >= c->w || y < 0 || y >= c->h) {
        blank.ch = ' ';
        blank.attr = AD_BLANK_ATTR;
        return blank;
    }
    return c->cells[(size_t)y * c->w + x];
}

void ad_canvas_touch_row(AdCanvas *c, int y) {
    if (!c->fixed && y >= c->h && y < AD_CANVAS_MAX_H) c->h = y + 1;
}

void ad_canvas_set(AdCanvas *c, int x, int y, unsigned char ch, unsigned char attr) {
    AdCell *cell;
    if (x < 0 || x >= c->w || y < 0 || y >= AD_CANVAS_MAX_H) return;
    if (c->fixed && y >= c->h) return;  /* off the bottom of a fixed canvas */
    ad_canvas_touch_row(c, y);
    cell = &c->cells[(size_t)y * c->w + x];
    cell->ch = ch;
    cell->attr = attr;
}

int ad_canvas_resize(AdCanvas *c, int w, int h) {
    AdCell *cells;
    size_t i, n;
    int x, y, keep_w, keep_h;
    if (w < 1 || w > AD_CANVAS_MAX_W || h < 1 || h > AD_CANVAS_MAX_H) return 0;
    n = (size_t)w * AD_CANVAS_MAX_H;
    cells = (AdCell *)malloc(n * sizeof(AdCell));
    if (!cells) return 0;
    for (i = 0; i < n; i++) {
        cells[i].ch = ' ';
        cells[i].attr = AD_BLANK_ATTR;
    }
    /* only rows inside the new height carry over, so growing the canvas
       later (cursor moving down) never brings cropped art back */
    keep_w = w < c->w ? w : c->w;
    keep_h = h < c->h ? h : c->h;
    for (y = 0; y < keep_h; y++)
        for (x = 0; x < keep_w; x++)
            cells[(size_t)y * w + x] = c->cells[(size_t)y * c->w + x];
    free(c->cells);
    c->cells = cells;
    c->w = w;
    c->h = h;
    c->fixed = 1;
    return 1;
}

static int blank_cell(AdCell cell) {
    return (cell.ch == ' ' || cell.ch == 0 || cell.ch == 0xFF) && AD_ATTR_BG(cell.attr) == 0;
}

void ad_canvas_extent(const AdCanvas *c, int *w, int *h) {
    int x, y;
    *w = *h = 0;
    for (y = 0; y < c->h; y++) {
        for (x = 0; x < c->w; x++) {
            if (!blank_cell(c->cells[(size_t)y * c->w + x])) {
                if (x + 1 > *w) *w = x + 1;
                *h = y + 1;
            }
        }
    }
}
