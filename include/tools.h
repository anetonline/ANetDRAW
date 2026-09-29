/* Drawing primitives shared by every tool.
 *
 * Everything draws through an AdSurface. With commit = 0 it writes into
 * a viewport-sized overlay (the live rubber-band preview while the
 * artist is still dragging a line/box/ellipse or floating a paste);
 * with commit = 1 it writes the canvas itself. Same shape code both
 * ways, so the preview can never disagree with what gets drawn. Mirror
 * mode is applied here too, so every tool mirrors for free, and all
 * committed writes funnel through one function (surf_put()) -- the
 * single place undo recording hooks in. */
#ifndef ANETDRAW_TOOLS_H
#define ANETDRAW_TOOLS_H

#include "canvas.h"

enum { AD_MIRROR_OFF = 0, AD_MIRROR_H = 1, AD_MIRROR_V = 2, AD_MIRROR_HV = 3 };

/* Called for every committed cell change, before it's applied -- the
   undo hook (Phase 3). NULL = no recording. */
typedef void (*AdCommitHook)(void *ctx, int x, int y, AdCell before, AdCell after);

typedef struct {
    AdCanvas *canvas;
    /* preview overlay: covers canvas rows ov_top .. ov_top+ov_h-1 */
    AdCell *ov;
    unsigned char *ov_mask;
    int ov_top, ov_h;
    int commit;
    int mirror;
    AdCommitHook hook;
    void *hook_ctx;
} AdSurface;

AdCell ad_surf_get(const AdSurface *s, int x, int y);
/* Puts a cell, plus its mirror image(s) with mirrored glyphs. */
void ad_surf_put(AdSurface *s, int x, int y, unsigned char ch, unsigned char attr);
/* Sets one half-block pixel (py = 2*row + half) to a color, re-encoding
   the cell as the best upper-half/lower-half/full-block/space glyph
   for its two pixels. Mirrors too. */
void ad_surf_pixel(AdSurface *s, int x, int py, int color);
/* Clears the overlay (start of each preview frame). */
void ad_surf_clear_overlay(AdSurface *s);

/* Mirror image of a glyph (e.g. left/right half block, box corners). */
unsigned char ad_mirror_glyph_h(unsigned char ch);
unsigned char ad_mirror_glyph_v(unsigned char ch);

/* Pixel cell encoding helpers. */
void ad_cell_to_pixels(AdCell c, int *top, int *bot);
AdCell ad_pixels_to_cell(int top, int bot, int ice);

/* ---- shapes. "Pixel" variants work in half-block space (y doubled). */
typedef enum {
    AD_BOX_BRUSH = 0, AD_BOX_BRUSH_FILLED, AD_BOX_SINGLE, AD_BOX_DOUBLE,
    AD_BOX_PIXEL, AD_BOX_PIXEL_FILLED, AD_BOX_STYLE_COUNT
} AdBoxStyle;
typedef enum {
    AD_ELLIPSE_BRUSH = 0, AD_ELLIPSE_BRUSH_FILLED,
    AD_ELLIPSE_PIXEL, AD_ELLIPSE_PIXEL_FILLED, AD_ELLIPSE_STYLE_COUNT
} AdEllipseStyle;

void ad_draw_line(AdSurface *s, int x0, int y0, int x1, int y1,
                  unsigned char ch, unsigned char attr);
void ad_draw_line_pixels(AdSurface *s, int x0, int py0, int x1, int py1, int color);
void ad_draw_box(AdSurface *s, int x0, int y0, int x1, int y1, AdBoxStyle style,
                 unsigned char ch, unsigned char attr, int color);
void ad_draw_ellipse(AdSurface *s, int x0, int y0, int x1, int y1, AdEllipseStyle style,
                     unsigned char ch, unsigned char attr, int color);

/* Flood fill (commit only). color_only = recolor the region, keep its
   glyphs. Region = 4-connected cells matching the start cell (glyph +
   attr, or just attr when color_only). Returns cells changed. */
int ad_flood_fill(AdSurface *s, int x, int y, unsigned char ch, unsigned char attr,
                  int color_only);

/* Shading brush: steps a cell along space, light/medium/dark shade,
   full block (CP437 B0 B1 B2 DB) in color fg.
   dir = +1 denser, -1 lighter. */
void ad_shade_step(AdSurface *s, int x, int y, int fg, int dir);

/* ---- colorize: new colors, same glyphs (mirror mode applies) */
#define AD_RECOLOR_BOTH 0
#define AD_RECOLOR_FG   1
#define AD_RECOLOR_BG   2
void ad_surf_recolor(AdSurface *s, int x, int y, int fg, int bg, int mode);
void ad_recolor_line(AdSurface *s, int x0, int y0, int x1, int y1, int fg, int bg, int mode);

/* ---- clipboard */
typedef struct {
    int w, h;
    AdCell *cells;   /* NULL = empty */
} AdClipboard;

void ad_clip_free(AdClipboard *cb);
int  ad_clip_copy(AdClipboard *cb, const AdCanvas *c, int x0, int y0, int x1, int y1);
void ad_clip_flip_h(AdClipboard *cb);
void ad_clip_flip_v(AdClipboard *cb);
/* Stamps the clipboard with its top-left at (x, y). transparent = skip
   blank cells (space on black) so the art underneath shows through. */
void ad_clip_paste(AdSurface *s, const AdClipboard *cb, int x, int y, int transparent);

#endif /* ANETDRAW_TOOLS_H */
