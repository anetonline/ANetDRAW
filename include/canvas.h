/* The drawing itself -- a grid of (CP437 glyph, PC text attribute)
 * cells, independent of anything on screen. The attribute byte is the
 * classic PC text-mode one (fg = low nibble, bg = high nibble, bit 7 =
 * blink or, with iCE colors on, bright background) -- the same meaning
 * .ANS/.BIN/XBin files and SAUCE's iCE flag already assume, so file
 * I/O (Phase 4) never has to translate. */
#ifndef ANETDRAW_CANVAS_H
#define ANETDRAW_CANVAS_H

#define AD_CANVAS_W      80    /* the classic ANSI art width -- default */
#define AD_CANVAS_MAX_W  400
#define AD_CANVAS_MIN_H  25
#define AD_CANVAS_MAX_H  2000

#define AD_ATTR(fg, bg)  ((unsigned char)((((bg) & 15) << 4) | ((fg) & 15)))
#define AD_ATTR_FG(a)    ((a) & 15)
#define AD_ATTR_BG(a)    (((a) >> 4) & 15)
#define AD_BLANK_ATTR    AD_ATTR(7, 0)

typedef struct {
    unsigned char ch;
    unsigned char attr;
} AdCell;

typedef struct {
    int w;
    /* Logical height -- the rows that are part of the drawing. Starts
       at AD_CANVAS_MIN_H and grows as the artist moves/paints further
       down; storage for AD_CANVAS_MAX_H rows is allocated up front so
       growing can never fail mid-edit. */
    int h;
    int ice;   /* 1 = bit 7 of attr is bright background, 0 = blink */
    /* 0: a new drawing grows downward as the artist moves/paints below
       the bottom. 1: the artist set the size explicitly (canvas size
       dialog), so it stays that size -- like other drawing programs. */
    int fixed;
    AdCell *cells;
} AdCanvas;

int  ad_canvas_init(AdCanvas *c, int w, int h); /* 0 on alloc failure */
void ad_canvas_free(AdCanvas *c);
void ad_canvas_clear(AdCanvas *c);
/* Out-of-range reads return a blank cell. */
AdCell ad_canvas_get(const AdCanvas *c, int x, int y);
/* Grows c->h to include row y if needed; out-of-range writes (x
   outside the width, y outside 0..AD_CANVAS_MAX_H-1) are ignored. */
void ad_canvas_set(AdCanvas *c, int x, int y, unsigned char ch, unsigned char attr);
/* Makes sure row y is part of the logical canvas (cursor moved there). */
void ad_canvas_touch_row(AdCanvas *c, int y);
/* Changes the drawing's size: keeps what fits, crops what doesn't, new
   area is blank. Returns 0 on allocation failure (canvas unchanged). */
int  ad_canvas_resize(AdCanvas *c, int w, int h);
/* Smallest width/height that still holds everything that isn't blank
   (0x0 for an empty canvas) -- what a resize would crop. */
void ad_canvas_extent(const AdCanvas *c, int *w, int *h);

#endif /* ANETDRAW_CANVAS_H */
