/* The editor: cursor, tools, status bar, key dispatch.
 *
 * Keys are all terminal-safe: in the Draw tool printable characters
 * type onto the canvas, so every command is a control key, function
 * key, or cursor key. In the other tools printable keys pick the brush
 * glyph (or are Select/paste commands). ^A+digit is the fallback for
 * callers whose terminal doesn't pass F-keys through. ^C/^S/^Q are
 * deliberately unused (BBS/flow-control software along the way can
 * eat them). ^Z/^Y are undo/redo as in PabloDraw/Moebius -- safe even
 * in local mode, since OpenDoors clears ISIG when it puts stdio in raw
 * mode (ODCom.c), so ^Z never suspends the door.
 *
 * Shapes and pastes are previewed live through the tools.h overlay,
 * then committed with Space/Enter; Esc backs out one level at a time
 * (paste -> anchor -> tool -> main menu). */
#include "../include/editor.h"
#include "../include/fkeys.h"
#include "../include/input.h"
#include "../include/transfer.h"
#include "../include/gallery.h"
#include "../include/tdf.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define STATUS_ATTR   AD_ATTR(15, 1)
#define STATUS_DIM    AD_ATTR(7, 1)
#define PAST_END_ATTR AD_ATTR(8, 0)
#define POPUP_ATTR    AD_ATTR(15, 1)
#define POPUP_KEY     AD_ATTR(14, 1)
#define POPUP_FRAME   AD_ATTR(11, 1)

static const char *const TOOL_NAMES[AD_TOOL_COUNT] = {
    "Draw", "Line", "Box", "Ellipse", "Fill", "Shade", "Pixel", "Select", "Colorize"
};
static const int TOOL_OPT_COUNT[AD_TOOL_COUNT] = {
    2, 2, AD_BOX_STYLE_COUNT, AD_ELLIPSE_STYLE_COUNT, 2, 2, 2, 1, 3
};

/* The file-name part of a path. */
static const char *base_name(const char *path) {
    const char *a = strrchr(path, '/'), *b = strrchr(path, '\\');
    const char *p = (b && (!a || b > a)) ? b : a;
    return p ? p + 1 : path;
}

static void set_message(AdEditor *e, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(e->message, sizeof(e->message), fmt, ap);
    va_end(ap);
}

/* Wide screens get a sidebar when this many columns are spare. */
#define SIDEBAR_MIN 38

/* (Re)sizes the screen buffers and overlay for a cols x rows terminal
   and decides the layout: canvas view on the left, a sidebar in any
   spare columns, status bar on the bottom row. Returns 0 on allocation
   failure (the old layout is kept). */
static int layout(AdEditor *e, int cols, int rows) {
    AdScreen ns;
    AdCell *nov;
    unsigned char *nmask;
    int vh;

    if (cols < AD_MIN_COLS) cols = AD_MIN_COLS;
    if (cols > AD_MAX_COLS) cols = AD_MAX_COLS;
    if (rows < AD_MIN_ROWS) rows = AD_MIN_ROWS;
    if (rows > AD_MAX_ROWS) rows = AD_MAX_ROWS;
    vh = rows - 1;

    if (!ad_screen_init(&ns, cols, rows)) return 0;
    nov = (AdCell *)malloc((size_t)e->canvas.w * vh * sizeof(AdCell));
    nmask = (unsigned char *)calloc((size_t)e->canvas.w * vh, 1);
    if (!nov || !nmask) {
        free(nov);
        free(nmask);
        ad_screen_free(&ns);
        return 0;
    }
    ad_screen_free(&e->screen);
    free(e->ov);
    free(e->ov_mask);
    e->screen = ns;
    e->ov = nov;
    e->ov_mask = nmask;
    e->view_h = vh;

    e->view_w = cols < e->canvas.w ? cols : e->canvas.w;
    e->sb_room = (cols - e->canvas.w >= SIDEBAR_MIN + 1);
    if (e->sb_room && !e->sb_hidden) {
        /* the toolbox docks at the right edge at its own width; every
           column between it and the canvas belongs to the drawing area
           (a wider canvas uses them, see canvas_dialog()) */
        e->sb_w = SIDEBAR_MIN;
        e->sb_x = cols - e->sb_w;
        e->view_w = e->sb_x - 1;     /* then one divider column */
    } else if (e->sb_room) {
        /* collapsed: the drawing area runs to the right edge, the last
           column is the "<TOOLS" tab that brings the toolbox back */
        e->sb_x = e->sb_w = 0;
        e->view_w = cols - 1;
    } else {
        e->sb_x = e->sb_w = 0;
    }
    ad_screen_set_ice(&e->screen, e->canvas.ice);
    ad_screen_full_redraw(&e->screen);
    return 1;
}

int ad_editor_init(AdEditor *e, int cols, int rows, int canvas_w) {
    memset(e, 0, sizeof(*e));
    if (!ad_canvas_init(&e->canvas, canvas_w, AD_CANVAS_MIN_H)) return 0;
    if (!ad_undo_init(&e->undo) || !layout(e, cols, rows)) {
        ad_editor_free(e);
        return 0;
    }
    memcpy(e->fkeys, AD_FKEY_SETS, sizeof(e->fkeys));
    e->fg = 7;
    e->bg = 0;
    e->fset = AD_FKEY_DEFAULT_SET;
    e->brush = 0xDB;
    e->tool = AD_TOOL_DRAW;
    return 1;
}

void ad_editor_free(AdEditor *e) {
    ad_undo_free(&e->undo);
    ad_clip_free(&e->clip);
    free(e->ov);
    free(e->ov_mask);
    e->ov = NULL;
    e->ov_mask = NULL;
    ad_screen_free(&e->screen);
    ad_canvas_free(&e->canvas);
}

static unsigned char cur_attr(const AdEditor *e) {
    return AD_ATTR(e->fg, e->bg);
}

/* A surface over the canvas: commit = 1 edits the drawing, 0 draws
   into the preview overlay for the visible rows. */
static AdSurface surface(AdEditor *e, int commit) {
    AdSurface s;
    memset(&s, 0, sizeof(s));
    s.canvas = &e->canvas;
    s.ov = e->ov;
    s.ov_mask = e->ov_mask;
    s.ov_top = e->top;
    s.ov_h = e->view_h;
    s.commit = commit;
    s.mirror = e->mirror;
    if (commit) {
        s.hook = ad_undo_record;  /* every committed edit is undoable */
        s.hook_ctx = &e->undo;
    }
    return s;
}

/* Tools that work in half-block pixel space (y doubled). */
static int pixel_space(const AdEditor *e) {
    switch (e->tool) {
        case AD_TOOL_PIXEL:   return 1;
        case AD_TOOL_LINE:    return e->opt[AD_TOOL_LINE] == 1;
        case AD_TOOL_BOX:     return e->opt[AD_TOOL_BOX] >= AD_BOX_PIXEL;
        case AD_TOOL_ELLIPSE: return e->opt[AD_TOOL_ELLIPSE] >= AD_ELLIPSE_PIXEL;
        default:              return 0;
    }
}

static int cursor_py(const AdEditor *e) { return e->cy * 2 + e->half; }

/* Keeps the cursor on the canvas and the viewport around the cursor. */
static void clamp_cursor(AdEditor *e) {
    if (e->cx < 0) e->cx = 0;
    if (e->cx >= e->canvas.w) e->cx = e->canvas.w - 1;
    if (e->cy < 0) { e->cy = 0; e->half = 0; }
    if (e->cy >= AD_CANVAS_MAX_H) { e->cy = AD_CANVAS_MAX_H - 1; e->half = 1; }
    if (e->canvas.fixed && e->cy >= e->canvas.h) { e->cy = e->canvas.h - 1; e->half = 1; }
    if (!pixel_space(e)) e->half = 0;
    ad_canvas_touch_row(&e->canvas, e->cy);
    if (e->top < 0) e->top = 0;
    if (e->cy < e->top) e->top = e->cy;
    if (e->cy >= e->top + e->view_h) e->top = e->cy - e->view_h + 1;
    /* horizontal scrolling, for canvases wider than the screen */
    if (e->left > e->canvas.w - e->view_w) e->left = e->canvas.w - e->view_w;
    if (e->left < 0) e->left = 0;
    if (e->cx < e->left) e->left = e->cx;
    if (e->cx >= e->left + e->view_w) e->left = e->cx - e->view_w + 1;
}

/* ------------------------------------------------------------ editing */

static void place(AdEditor *e, unsigned char ch) {
    AdSurface s = surface(e, 1);
    if (e->insert) {
        /* shift the rest of the row right; the last column falls off */
        int x;
        s.mirror = 0;
        for (x = e->canvas.w - 1; x > e->cx; x--) {
            AdCell c = ad_canvas_get(&e->canvas, x - 1, e->cy);
            ad_surf_put(&s, x, e->cy, c.ch, c.attr);
        }
        s.mirror = e->mirror;
    }
    ad_surf_put(&s, e->cx, e->cy, ch, cur_attr(e));
    if (e->cx < e->canvas.w - 1) e->cx++;
}

static void backspace(AdEditor *e) {
    AdSurface s = surface(e, 1);
    if (e->cx == 0) return;
    e->cx--;
    if (e->insert) {
        int x;
        s.mirror = 0;
        for (x = e->cx; x < e->canvas.w - 1; x++) {
            AdCell c = ad_canvas_get(&e->canvas, x + 1, e->cy);
            ad_surf_put(&s, x, e->cy, c.ch, c.attr);
        }
        ad_surf_put(&s, e->canvas.w - 1, e->cy, ' ', AD_BLANK_ATTR);
    } else {
        ad_surf_put(&s, e->cx, e->cy, ' ', AD_BLANK_ATTR);
    }
}

/* The "pen down" action for tools that paint as the cursor moves. */
static void pen_apply(AdEditor *e) {
    AdSurface s = surface(e, 1);
    switch (e->tool) {
        case AD_TOOL_DRAW:  ad_surf_put(&s, e->cx, e->cy, e->brush, cur_attr(e)); break;
        case AD_TOOL_SHADE: ad_shade_step(&s, e->cx, e->cy, e->fg, +1); break;
        case AD_TOOL_PIXEL: ad_surf_pixel(&s, e->cx, cursor_py(e), e->fg); break;
        default: break;
    }
}

static int pen_on(const AdEditor *e) {
    return (e->tool == AD_TOOL_DRAW || e->tool == AD_TOOL_SHADE || e->tool == AD_TOOL_PIXEL) &&
           e->opt[e->tool] == 1 && !e->pasting;
}

/* Draws the current shape from the anchor to the cursor -- into the
   preview overlay or (commit) onto the canvas. */
static void draw_shape(AdEditor *e, AdSurface *s) {
    int px = pixel_space(e);
    int y0 = px ? e->apy : e->ay, y1 = px ? cursor_py(e) : e->cy;
    switch (e->tool) {
        case AD_TOOL_LINE:
            if (px) ad_draw_line_pixels(s, e->ax, y0, e->cx, y1, e->fg);
            else    ad_draw_line(s, e->ax, y0, e->cx, y1, e->brush, cur_attr(e));
            break;
        case AD_TOOL_BOX:
            ad_draw_box(s, e->ax, y0, e->cx, y1, (AdBoxStyle)e->opt[AD_TOOL_BOX],
                        e->brush, cur_attr(e), e->fg);
            break;
        case AD_TOOL_ELLIPSE:
            ad_draw_ellipse(s, e->ax, y0, e->cx, y1, (AdEllipseStyle)e->opt[AD_TOOL_ELLIPSE],
                            e->brush, cur_attr(e), e->fg);
            break;
        default: break;
    }
}

static void sel_rect(const AdEditor *e, int *x0, int *y0, int *x1, int *y1) {
    *x0 = e->ax < e->cx ? e->ax : e->cx;
    *x1 = e->ax < e->cx ? e->cx : e->ax;
    *y0 = e->ay < e->cy ? e->ay : e->cy;
    *y1 = e->ay < e->cy ? e->cy : e->ay;
}

static void select_command(AdEditor *e, char cmd) {
    int x0, y0, x1, y1, x, y;
    AdSurface s = surface(e, 1);
    sel_rect(e, &x0, &y0, &x1, &y1);
    s.mirror = 0;  /* block operations act on exactly the marked block */
    switch (cmd) {
        case 'C':
            if (ad_clip_copy(&e->clip, &e->canvas, x0, y0, x1, y1))
                set_message(e, "Copied %dx%d -- ^V to paste", x1 - x0 + 1, y1 - y0 + 1);
            break;
        case 'X':
        case 'M':
            if (!ad_clip_copy(&e->clip, &e->canvas, x0, y0, x1, y1)) break;
            for (y = y0; y <= y1; y++)
                for (x = x0; x <= x1; x++) ad_surf_put(&s, x, y, ' ', AD_BLANK_ATTR);
            if (cmd == 'M') {
                e->pasting = 1;
                e->cx = x0;
                e->cy = y0;
                set_message(e, "Moving -- arrows, Enter to drop");
            } else {
                set_message(e, "Cut %dx%d -- ^V to paste", x1 - x0 + 1, y1 - y0 + 1);
            }
            break;
        case 'D':
            for (y = y0; y <= y1; y++)
                for (x = x0; x <= x1; x++) ad_surf_put(&s, x, y, ' ', AD_BLANK_ATTR);
            set_message(e, "Deleted %dx%d", x1 - x0 + 1, y1 - y0 + 1);
            break;
        case 'F':
            for (y = y0; y <= y1; y++)
                for (x = x0; x <= x1; x++) ad_surf_put(&s, x, y, e->brush, cur_attr(e));
            set_message(e, "Filled %dx%d with the brush", x1 - x0 + 1, y1 - y0 + 1);
            break;
        case 'H':
        case 'V': {
            /* flip in place: glyphs are mirrored too (◄ becomes ►) */
            AdClipboard tmp = { 0, 0, NULL };
            if (!ad_clip_copy(&tmp, &e->canvas, x0, y0, x1, y1)) break;
            if (cmd == 'H') ad_clip_flip_h(&tmp); else ad_clip_flip_v(&tmp);
            ad_clip_paste(&s, &tmp, x0, y0, 0);
            ad_clip_free(&tmp);
            set_message(e, cmd == 'H' ? "Flipped left-right" : "Flipped upside-down");
            e->anchored = 1;  /* keep the block marked for more */
            return;
        }
        case 'S':
            for (y = y0; y <= y1; y++)
                for (x = x0; x <= x1; x++) {
                    AdCell c = ad_canvas_get(&e->canvas, x, y);
                    int f = AD_ATTR_FG(c.attr), b = AD_ATTR_BG(c.attr);
                    ad_surf_put(&s, x, y, c.ch, AD_ATTR(b, e->canvas.ice ? f : (f & 7)));
                }
            set_message(e, "Swapped foreground and background");
            e->anchored = 1;
            return;
        case 'N':
            /* center each row's content between the block's edges */
            for (y = y0; y <= y1; y++) {
                int first = -1, last = -1, len, dst;
                AdCell row[AD_CANVAS_MAX_W];
                for (x = x0; x <= x1; x++) {
                    AdCell c = ad_canvas_get(&e->canvas, x, y);
                    row[x - x0] = c;
                    if (!((c.ch == ' ' || c.ch == 0) && AD_ATTR_BG(c.attr) == 0)) {
                        if (first < 0) first = x;
                        last = x;
                    }
                }
                if (first < 0) continue;
                len = last - first + 1;
                dst = x0 + ((x1 - x0 + 1) - len) / 2;
                for (x = x0; x <= x1; x++) {
                    int src = x - dst + first;
                    if (src >= first && src <= last) ad_surf_put(&s, x, y, row[src - x0].ch, row[src - x0].attr);
                    else ad_surf_put(&s, x, y, ' ', AD_BLANK_ATTR);
                }
            }
            set_message(e, "Centered each row of the block");
            e->anchored = 1;
            return;
        case 'R':
            /* new colors, same drawing -- Colorize for a whole block */
            for (y = y0; y <= y1; y++)
                for (x = x0; x <= x1; x++) ad_surf_recolor(&s, x, y, e->fg, e->bg, e->opt[AD_TOOL_COLORIZE]);
            set_message(e, "Recolored %dx%d", x1 - x0 + 1, y1 - y0 + 1);
            break;
        default:
            return;
    }
    e->anchored = 0;
}

static void start_paste(AdEditor *e) {
    if (!e->clip.cells) {
        set_message(e, "Clipboard is empty -- mark a block with Select (^T K)");
        return;
    }
    e->pasting = 1;
    e->anchored = 0;
    set_message(e, "Paste: Enter=stamp H/V=flip T=transparent Esc=done");
}

/* Space / Enter in any tool but Draw. */
static void tool_action(AdEditor *e) {
    AdSurface s = surface(e, 1);
    if (e->pasting) {
        ad_clip_paste(&s, &e->clip, e->cx, e->cy, e->paste_transparent);
        return;
    }
    switch (e->tool) {
        case AD_TOOL_LINE:
        case AD_TOOL_BOX:
        case AD_TOOL_ELLIPSE:
            if (!e->anchored) {
                e->anchored = 1;
                e->ax = e->cx;
                e->ay = e->cy;
                e->apy = cursor_py(e);
            } else {
                draw_shape(e, &s);
                e->anchored = 0;
            }
            break;
        case AD_TOOL_FILL:
            if (ad_flood_fill(&s, e->cx, e->cy, e->brush, cur_attr(e), e->opt[AD_TOOL_FILL]) == 0)
                set_message(e, "Nothing to fill there");
            break;
        case AD_TOOL_SHADE:
            ad_shade_step(&s, e->cx, e->cy, e->fg, +1);
            break;
        case AD_TOOL_PIXEL:
            ad_surf_pixel(&s, e->cx, cursor_py(e), e->fg);
            break;
        case AD_TOOL_COLORIZE:
            ad_surf_recolor(&s, e->cx, e->cy, e->fg, e->bg, e->opt[AD_TOOL_COLORIZE]);
            break;
        case AD_TOOL_SELECT:
            if (!e->anchored) {
                e->anchored = 1;
                e->ax = e->cx;
                e->ay = e->cy;
                set_message(e, "Mark: move, then C=copy X=cut M=move D=del F=fill");
            } else {
                select_command(e, 'C');
            }
            break;
        default:
            break;
    }
}

static void set_tool(AdEditor *e, AdTool t) {
    e->tool = t;
    e->anchored = 0;
    e->pasting = 0;
    clamp_cursor(e);
    if (t == AD_TOOL_SELECT)
        set_message(e, "Select: drag to mark a block (or Space + arrows), right-click for Copy/Cut/Paste");
    if (t == AD_TOOL_COLORIZE)
        set_message(e, "Colorize: drag over the art to give it the current colors (Tab: FG+BG/FG/BG)");
}

/* Select tool: mark the whole canvas. */
static void select_all(AdEditor *e) {
    e->anchored = 1;
    e->ax = 0;
    e->ay = 0;
    e->cx = e->canvas.w - 1;
    e->cy = e->canvas.h - 1;
    set_message(e, "Marked everything: C copy  X cut  D delete  (right-click for more)");
}

/* ------------------------------------------------------------ drawing the screen */

static void tool_label(const AdEditor *e, char *buf, size_t n) {
    static const char *const BOX_OPT[] = { "Brush", "Solid", "Single", "Double", "Pixel", "PxSolid" };
    static const char *const ELL_OPT[] = { "Brush", "Solid", "Pixel", "PxSolid" };
    const char *opt = "";
    if (e->pasting) {
        snprintf(buf, n, "PASTE %dx%d%s", e->clip.w, e->clip.h, e->paste_transparent ? " T" : "");
        return;
    }
    switch (e->tool) {
        case AD_TOOL_DRAW:    opt = e->opt[e->tool] ? (e->insert ? "Pen INS" : "Pen") : (e->insert ? "INS" : ""); break;
        case AD_TOOL_LINE:    opt = e->opt[e->tool] ? "Pixel" : "Brush"; break;
        case AD_TOOL_BOX:     opt = BOX_OPT[e->opt[e->tool]]; break;
        case AD_TOOL_ELLIPSE: opt = ELL_OPT[e->opt[e->tool]]; break;
        case AD_TOOL_FILL:    opt = e->opt[e->tool] ? "Color" : "Glyph"; break;
        case AD_TOOL_SHADE:   opt = e->opt[e->tool] ? "Pen" : ""; break;
        case AD_TOOL_PIXEL:   opt = e->half ? (e->opt[e->tool] ? "Bot Pen" : "Bot")
                                            : (e->opt[e->tool] ? "Top Pen" : "Top"); break;
        case AD_TOOL_COLORIZE: {
            static const char *const CO[3] = { "FG+BG", "FG", "BG" };
            opt = CO[e->opt[e->tool] % 3];
            break;
        }
        case AD_TOOL_SELECT:
            if (e->anchored) {
                snprintf(buf, n, "Mark %dx%d", abs(e->cx - e->ax) + 1, abs(e->cy - e->ay) + 1);
                return;
            }
            break;
        default: break;
    }
    snprintf(buf, n, "%s%s%s%s", e->anchored && e->tool != AD_TOOL_SELECT ? "*" : "",
             TOOL_NAMES[e->tool], opt[0] ? " " : "", opt);
}

static void paint_view(AdEditor *e) {
    int row, col, sx0 = 0, sy0 = 0, sx1 = -1, sy1 = -1;
    AdSurface prev = surface(e, 0);

    ad_surf_clear_overlay(&prev);
    if (e->pasting) {
        ad_clip_paste(&prev, &e->clip, e->cx, e->cy, e->paste_transparent);
    } else if (e->anchored && e->tool != AD_TOOL_SELECT) {
        draw_shape(e, &prev);
    }
    if (e->anchored && e->tool == AD_TOOL_SELECT) sel_rect(e, &sx0, &sy0, &sx1, &sy1);

    for (row = 0; row < e->view_h; row++) {
        int cy = e->top + row;
        for (col = 0; col < e->view_w; col++) {
            int x = e->left + col;
            size_t i = (size_t)row * e->canvas.w + x;
            if (x >= e->canvas.w) {
                /* past the canvas's right edge: a dim dotted "desk",
                   like the gray around the canvas in PabloDraw/Moebius */
                ad_screen_put(&e->screen, col, row, 0xB0, AD_ATTR(8, 0));
                continue;
            }
            if (cy < e->canvas.h || e->ov_mask[i]) {
                AdCell c = e->ov_mask[i] ? e->ov[i] : ad_canvas_get(&e->canvas, x, cy);
                if (x >= sx0 && x <= sx1 && cy >= sy0 && cy <= sy1) {
                    /* marked block: show it inverted */
                    c.attr = AD_ATTR(AD_ATTR_BG(c.attr), AD_ATTR_FG(c.attr) & 7);
                }
                ad_screen_put(&e->screen, col, row, c.ch, c.attr);
            } else {
                /* Past the end of the drawing: a faint dot in column 1
                   so the artist can see where the canvas stops. */
                ad_screen_put(&e->screen, col, row, x == 0 ? 0xFA : ' ', PAST_END_ATTR);
            }
        }
        /* right of the canvas: a divider, then the sidebar (painted
           separately) or plain filler on mid-width screens */
        for (col = e->view_w; col < e->screen.w; col++)
            ad_screen_put(&e->screen, col, row, col == e->view_w ? 0xB3 : ' ', AD_ATTR(9, 0));
        /* collapsed toolbox: a vertical "<TOOLS" tab on the divider (the
           whole divider is clickable) to bring it back */
        if (e->sb_room && e->sb_hidden && e->view_w < e->screen.w && row < 6)
            ad_screen_put(&e->screen, e->view_w, row, (unsigned char)"\x11TOOLS"[row],
                          AD_ATTR(14, 1));
    }
}

/* Bottom row, 79 columns (column 80 of the last row is never written
   -- see render.h). Layout:
     0-8   " 80:0001 "     cursor position
     9-12  " Aa "          sample in the current colors
     13-15 " # "           brush glyph
     16-31 tool + option   (or the message, which runs to col 78)
     32-36 "iCE"/"BLK" + mirror flag
     38-57 F-key strip     digit + glyph x10
     59-78 "S06|^T Tools ^O Help"  */
static void paint_status(AdEditor *e) {
    int row = e->screen.h - 1;
    int col, i;
    char buf[80];

    for (col = 0; col < e->screen.w - 1; col++)
        ad_screen_put(&e->screen, col, row, ' ', STATUS_ATTR);

    /* 9 columns either way: " 80:0001 " or " 132:0001" */
    /* column,row -- "(23,6)" -- the old "23:0006" read like a clock */
    snprintf(buf, sizeof(buf), "(%d,%d)", e->cx + 1, e->cy + 1);
    if (strlen(buf) > 8) snprintf(buf, sizeof(buf), "%d,%d", e->cx + 1, e->cy + 1);
    {
        char pos[16];
        snprintf(pos, sizeof(pos), " %-8.8s", buf);
        snprintf(buf, sizeof(buf), "%s", pos);
    }
    ad_screen_puts(&e->screen, 0, row, buf, STATUS_ATTR);
    ad_screen_puts(&e->screen, 9, row, " Aa ", cur_attr(e));
    ad_screen_put(&e->screen, 13, row, ' ', STATUS_ATTR);
    ad_screen_put(&e->screen, 14, row, e->brush, cur_attr(e));
    ad_screen_put(&e->screen, 15, row, ' ', STATUS_ATTR);

    if (e->message[0]) {
        snprintf(buf, sizeof(buf), " %-62.62s", e->message);
        ad_screen_puts(&e->screen, 16, row, buf, AD_ATTR(14, 1));
        return;
    }
    tool_label(e, buf, sizeof(buf));
    {
        char field[32];
        snprintf(field, sizeof(field), " %-15.15s", buf);
        ad_screen_puts(&e->screen, 16, row, field, AD_ATTR(14, 1));
    }
    ad_screen_puts(&e->screen, 32, row, e->canvas.ice ? "iCE" : "BLK", STATUS_DIM);
    {
        static const char *const MIR[4] = { "  ", "MH", "MV", "M+" };
        ad_screen_puts(&e->screen, 35, row, MIR[e->mirror & 3], STATUS_ATTR);
    }
    for (i = 0; i < AD_FKEY_PER_SET; i++) {
        int c = 38 + i * 2;
        ad_screen_put(&e->screen, c, row, (unsigned char)('0' + (i + 1) % 10), STATUS_DIM);
        ad_screen_put(&e->screen, c + 1, row, e->fkeys[e->fset][i], cur_attr(e));
    }
    snprintf(buf, sizeof(buf), "S%02d\xB3^T Tools ^O Help", e->fset + 1);
    ad_screen_puts(&e->screen, 59, row, buf, STATUS_ATTR);
}

/* ------------------------------------------------------------ sidebar */

enum { HIT_FG = 1, HIT_BG, HIT_TOOL, HIT_FKEY, HIT_SET_PREV, HIT_SET_NEXT, HIT_SEL, HIT_TEXT, HIT_MENU, HIT_UNDO,
       HIT_OPTION, HIT_MIRROR, HIT_ICE, HIT_COLORS, HIT_CHARS, HIT_HELP, HIT_HIDE };

#define SB_ATTR   AD_ATTR(7, 0)
#define SB_HEAD   AD_ATTR(11, 0)
#define SB_KEY    AD_ATTR(14, 0)
#define SB_DIM    AD_ATTR(8, 0)
#define SB_SEL    AD_ATTR(0, 7)

static void sb_hit(AdEditor *e, int x0, int x1, int y, int action, int arg) {
    if (e->nhits < (int)(sizeof(e->hits) / sizeof(e->hits[0])) && y < e->view_h) {
        e->hits[e->nhits].x0 = (short)x0;
        e->hits[e->nhits].x1 = (short)x1;
        e->hits[e->nhits].y = (short)y;
        e->hits[e->nhits].action = (short)action;
        e->hits[e->nhits].arg = (short)arg;
        e->nhits++;
    }
}

/* Clipped to the sidebar and the rows above the status bar. */
static void sb_puts(AdEditor *e, int x, int y, const char *str, unsigned char attr) {
    for (; *str && x < e->screen.w; ++str, ++x)
        if (y < e->view_h) ad_screen_put(&e->screen, x, y, (unsigned char)*str, attr);
}

static void sb_put(AdEditor *e, int x, int y, unsigned char ch, unsigned char attr) {
    if (y < e->view_h && x < e->screen.w) ad_screen_put(&e->screen, x, y, ch, attr);
}

static void swatch(AdEditor *e, int x, int y, int color) {
    /* color 0 (black) drawn as a faint shade so it's visible */
    unsigned char g = color ? 0xDB : 0xB0;
    unsigned char a = color ? AD_ATTR(color, 0) : AD_ATTR(8, 0);
    sb_put(e, x, y, g, a);
    sb_put(e, x + 1, y, g, a);
}

/* PabloDraw-style toolbox in the spare columns of a wide screen.
   Everything on it is clickable (see sidebar_click()). Laid out to fit
   23 rows; taller screens just have room to spare. */
static void paint_sidebar(AdEditor *e) {
    static const char TOOL_KEYS[AD_TOOL_COUNT] = { 'D', 'L', 'B', 'E', 'F', 'S', 'P', 'K', 'C' };
    int x = e->sb_x + 1, w = e->sb_w - 1, y, i, half;
    char buf[64];

    e->nhits = 0;
    if (!e->sb_w) return;
    /* second column of the tool / F-key lists: close enough to read as
       one panel even when the sidebar is very wide */
    half = w / 2 < 16 ? w / 2 : 16;

    snprintf(buf, sizeof(buf), "ANetDRAW v%s", AD_VERSION);
    sb_puts(e, x, 0, "ANetDRAW", AD_ATTR(15, 0));
    sb_puts(e, x + 9, 0, buf + 9, SB_DIM);
    sb_puts(e, x + w - 8, 0, "hide \x10", SB_KEY);
    sb_hit(e, x + w - 8, x + w - 2, 0, HIT_HIDE, 0);
    for (i = 0; i < w - 1; i++) sb_put(e, x + i, 1, 0xC4, SB_DIM);

    /* colors: left-click = foreground, right-click = background */
    sb_puts(e, x, 2, "Colors", SB_HEAD);
    sb_puts(e, x + 7, 2, e->canvas.ice ? "(iCE)" : "(blink)", SB_DIM);
    sb_hit(e, x + 7, x + 13, 2, HIT_ICE, 0);
    sb_puts(e, x + 15, 2, "^K", SB_KEY);
    sb_hit(e, x + 15, x + 16, 2, HIT_COLORS, 0);
    sb_puts(e, x, 3, "FG", SB_ATTR);
    sb_puts(e, x, 5, "BG", SB_ATTR);
    for (i = 0; i < 16; i++) {
        int sx = x + 3 + i * 2;
        swatch(e, sx, 3, i);
        sb_hit(e, sx, sx + 1, 3, HIT_FG, i);
        if (i == e->fg) sb_put(e, sx, 4, 0x1E, AD_ATTR(15, 0));
        if (i < (e->canvas.ice ? 16 : 8)) {
            swatch(e, sx, 5, i);
            sb_hit(e, sx, sx + 1, 5, HIT_BG, i);
            if (i == e->bg) sb_put(e, sx, 6, 0x1E, AD_ATTR(15, 0));
        }
    }
    sb_puts(e, x, 7, "Now", SB_ATTR);
    sb_puts(e, x + 4, 7, " Aa ", cur_attr(e));
    sb_puts(e, x + 9, 7, "Brush", SB_ATTR);
    sb_put(e, x + 15, 7, e->brush, cur_attr(e));
    sb_hit(e, x + 9, x + 15, 7, HIT_CHARS, 0);
    sb_puts(e, x + 17, 7, "^G", SB_KEY);
    sb_hit(e, x + 17, x + 18, 7, HIT_CHARS, 0);

    {
        char fn[64];
        snprintf(fn, sizeof(fn), "%.*s%s", w - 4, e->path[0] ? base_name(e->path) : "Untitled",
                 e->dirty ? " *" : "");
        sb_puts(e, x, 8, fn, e->dirty ? AD_ATTR(14, 0) : SB_DIM);
    }

    /* tools, two columns of five; the tenth is Text (TheDraw fonts) */
    sb_puts(e, x, 9, "Tools", SB_HEAD);
    sb_puts(e, x + 6, 9, "^T", SB_KEY);
    for (i = 0; i <= AD_TOOL_COUNT; i++) {
        int tx = x + (i / 5) * half, ty = 10 + i % 5;
        int sel = (i < AD_TOOL_COUNT && i == (int)e->tool);
        char key = i < AD_TOOL_COUNT ? TOOL_KEYS[i] : 'T';
        snprintf(buf, sizeof(buf), " %c %-8s", key, i < AD_TOOL_COUNT ? TOOL_NAMES[i] : "Text");
        buf[half - 1 < (int)sizeof(buf) ? half - 1 : (int)sizeof(buf) - 1] = 0;
        sb_puts(e, tx, ty, buf, sel ? SB_SEL : SB_ATTR);
        if (!sel) sb_put(e, tx + 1, ty, (unsigned char)key, SB_KEY);
        if (i < AD_TOOL_COUNT) sb_hit(e, tx, tx + half - 2, ty, HIT_TOOL, i);
        else sb_hit(e, tx, tx + half - 2, ty, HIT_TEXT, 0);
    }
    {
        char label[40];
        tool_label(e, label, sizeof(label));
        snprintf(buf, sizeof(buf), "%-*.*s", w - 10, w - 10, label);
        sb_puts(e, x, 15, "Tab", SB_KEY);
        sb_puts(e, x + 4, 15, buf, AD_ATTR(15, 0));
        sb_hit(e, x, x + w - 2, 15, HIT_OPTION, 0);
    }

    /* F-key glyphs of the current set, two columns */
    snprintf(buf, sizeof(buf), "F-keys  \x11 Set %02d \x10", e->fset + 1);
    sb_puts(e, x, 16, buf, SB_HEAD);
    sb_hit(e, x + 8, x + 9, 16, HIT_SET_PREV, 0);
    sb_hit(e, x + 17, x + 18, 16, HIT_SET_NEXT, 0);
    for (i = 0; i < AD_FKEY_PER_SET; i++) {
        int fx = x + (i / 5) * half, fy = 17 + i % 5;
        snprintf(buf, sizeof(buf), "F%-2d", i + 1);
        sb_puts(e, fx, fy, buf, SB_KEY);
        sb_put(e, fx + 4, fy, e->fkeys[e->fset][i], cur_attr(e));
        sb_hit(e, fx, fx + 5, fy, HIT_FKEY, i);
    }

    {
        static const char *const MIR[4] = { "off", "left/right", "top/bottom", "both" };
        snprintf(buf, sizeof(buf), "Mirror %-11s", MIR[e->mirror & 3]);
        sb_puts(e, x, 22, buf, SB_ATTR);
        sb_hit(e, x, x + 16, 22, HIT_MIRROR, 0);
        snprintf(buf, sizeof(buf), "Undo %d  Redo %d", e->undo.applied, e->undo.nacts - e->undo.applied);
        sb_puts(e, x + 19, 22, buf, SB_DIM);
    }
    /* block actions: what Select marked, or what's on the clipboard */
    {
        static const struct { char cmd; const char *label; } ACT[] = {
            { 'C', "Copy" }, { 'X', "Cut" }, { 'D', "Del" }, { 'F', "Fill" }, { 'M', "Move" },
            { 'R', "Recolor" } };
        int bx = x;
        size_t a;
        if (e->tool == AD_TOOL_SELECT && e->anchored && !e->pasting) {
            int x0, y0, x1, y1;
            sel_rect(e, &x0, &y0, &x1, &y1);
            snprintf(buf, sizeof(buf), "%dx%d", x1 - x0 + 1, y1 - y0 + 1);
            sb_puts(e, bx, 23, buf, SB_HEAD);
            bx += (int)strlen(buf) + 1;
            for (a = 0; a < sizeof(ACT) / sizeof(ACT[0]); a++) {
                int l = (int)strlen(ACT[a].label);
                if (bx + l > x + w - 1) break;
                sb_puts(e, bx, 23, ACT[a].label, SB_SEL);
                sb_hit(e, bx, bx + l - 1, 23, HIT_SEL, ACT[a].cmd);
                bx += l + 1;
            }
        } else if (!e->pasting && (e->clip.cells || e->tool == AD_TOOL_SELECT)) {
            if (e->clip.cells) {
                snprintf(buf, sizeof(buf), "Clip %dx%d", e->clip.w, e->clip.h);
                sb_puts(e, bx, 23, buf, SB_DIM);
                bx += (int)strlen(buf) + 1;
                sb_puts(e, bx, 23, "Paste", SB_SEL);
                sb_hit(e, bx, bx + 4, 23, HIT_SEL, 'P');
                bx += 6;
            }
            if (e->tool == AD_TOOL_SELECT) {
                sb_puts(e, bx, 23, "All", SB_SEL);
                sb_hit(e, bx, bx + 2, 23, HIT_SEL, 'A');
            }
        }
    }
    /* taller screens: file and canvas buttons, then a key reminder on
       the bottom line */
    if (e->view_h >= 32) {
        static const struct { char cmd; const char *label; int row; } BTN[] = {
            { 'N', "New", 26 }, { 'O', "Open", 26 }, { 'S', "Save", 26 }, { 'A', "Save as", 26 },
            { 'B', "Gallery", 27 }, { 'P', "Publish", 27 }, { 'D', "Download", 27 } };
        int bx = x, brow = 26;
        size_t b;
        sb_puts(e, x, 25, "File", SB_HEAD);
        sb_puts(e, x + 5, 25, "Esc=menu", SB_DIM);
        for (b = 0; b < sizeof(BTN) / sizeof(BTN[0]); b++) {
            int l = (int)strlen(BTN[b].label);
            if (BTN[b].row != brow) { brow = BTN[b].row; bx = x; }
            if (bx + l > x + w - 1) continue;
            sb_puts(e, bx, brow, BTN[b].label, SB_SEL);
            sb_hit(e, bx, bx + l - 1, brow, HIT_MENU, BTN[b].cmd);
            bx += l + 1;
        }
        snprintf(buf, sizeof(buf), "%dx%d %s", e->canvas.w, e->canvas.h, e->canvas.ice ? "iCE" : "blink");
        sb_puts(e, x, 29, "Canvas", SB_HEAD);
        sb_puts(e, x + 7, 29, buf, SB_ATTR);
        sb_puts(e, x + 8 + (int)strlen(buf), 29, "Size", SB_SEL);
        sb_hit(e, x + 8 + (int)strlen(buf), x + 11 + (int)strlen(buf), 29, HIT_MENU, 'C');
        sb_puts(e, x, 30, "Undo", SB_SEL);
        sb_hit(e, x, x + 3, 30, HIT_UNDO, 0);
        sb_puts(e, x + 5, 30, "Redo", SB_SEL);
        sb_hit(e, x + 5, x + 8, 30, HIT_UNDO, 1);
        sb_puts(e, x + 10, 30, "Fonts", SB_SEL);
        sb_hit(e, x + 10, x + 14, 30, HIT_TEXT, 0);
        sb_puts(e, x + 16, 30, "Colors", SB_SEL);
        sb_hit(e, x + 16, x + 21, 30, HIT_COLORS, 0);
        sb_puts(e, x + 23, 30, "Chars", SB_SEL);
        sb_hit(e, x + 23, x + 27, 30, HIT_CHARS, 0);
        sb_puts(e, x, e->view_h - 1, "^O help   ^Z/^Y undo/redo", SB_DIM);
        sb_hit(e, x, x + 6, e->view_h - 1, HIT_HELP, 0);
    } else {
        for (y = 23; y < e->view_h; y++)
            if (y == 24) sb_puts(e, x, y, "^O help   ^Z/^Y undo/redo", SB_DIM);
    }
}

static int help_screen(AdEditor *e, int local);
static int color_picker(AdEditor *e, int local);
static int char_picker(AdEditor *e, int local);
static void tool_menu(AdEditor *e, int local);
static void handle_ctrl(AdEditor *e, char c, int local);
static void toggle_sidebar(AdEditor *e);
static void font_tool(AdEditor *e, int local);
static int menu_command(AdEditor *e, int local, char c);

/* One Select-tool block command, from a key, the toolbox or the
   right-click menu: C copy, X cut, D delete, F fill with the brush,
   M move, P paste, A mark everything. */
static void replace_color_dialog(AdEditor *e, int local);

static void block_command(AdEditor *e, int local, char cmd) {
    if (cmd == 'P') { start_paste(e); return; }
    if (cmd == 'L') { if (e->tool == AD_TOOL_SELECT && e->anchored) replace_color_dialog(e, local); return; }
    if (cmd == 'A') {
        if (e->tool != AD_TOOL_SELECT) set_tool(e, AD_TOOL_SELECT);
        select_all(e);
        return;
    }
    if (e->tool == AD_TOOL_SELECT && e->anchored) select_command(e, cmd);
}

/* Returns 0 when the session should end. */
static int sidebar_click(AdEditor *e, int x, int y, int button, int local) {
    int i;
    for (i = 0; i < e->nhits; i++) {
        if (e->hits[i].y != y || x < e->hits[i].x0 || x > e->hits[i].x1) continue;
        switch (e->hits[i].action) {
            case HIT_FG:
                /* right-click a FG swatch = set the background (if allowed) */
                if (button == 2) {
                    if (e->hits[i].arg < (e->canvas.ice ? 16 : 8)) e->bg = e->hits[i].arg;
                } else {
                    e->fg = e->hits[i].arg;
                }
                break;
            case HIT_BG:       e->bg = e->hits[i].arg; break;
            case HIT_TOOL:     set_tool(e, (AdTool)e->hits[i].arg); break;
            case HIT_FKEY:
                e->brush = e->fkeys[e->fset][e->hits[i].arg];
                set_message(e, "Brush is F%d's glyph", e->hits[i].arg + 1);
                break;
            case HIT_SET_PREV: handle_ctrl(e, 'P', local); break;
            case HIT_SET_NEXT: handle_ctrl(e, 'N', local); break;
            case HIT_OPTION:
                if (!e->pasting && TOOL_OPT_COUNT[e->tool] > 1) {
                    e->opt[e->tool] = (e->opt[e->tool] + 1) % TOOL_OPT_COUNT[e->tool];
                    e->anchored = 0;
                }
                break;
            case HIT_MIRROR:   handle_ctrl(e, 'R', local); break;
            case HIT_ICE:      handle_ctrl(e, 'E', local); break;
            case HIT_COLORS:   return color_picker(e, local);
            case HIT_CHARS:    return char_picker(e, local);
            case HIT_HELP:     return help_screen(e, local);
            case HIT_HIDE:     toggle_sidebar(e); break;
            case HIT_TEXT:     font_tool(e, local); break;
            case HIT_SEL:      block_command(e, local, (char)e->hits[i].arg); break;
            case HIT_MENU:     return menu_command(e, local, (char)e->hits[i].arg);
            case HIT_UNDO:     e->pending_undo = e->hits[i].arg ? 2 : 1; break;  /* run by the main loop */
            default: break;
        }
        return 1;
    }
    return 1;
}

/* Canvas view + sidebar + status bar into the screen buffer. */
static void paint_all(AdEditor *e) {
    paint_view(e);
    paint_sidebar(e);
    paint_status(e);
}

static void flush_at_cursor(AdEditor *e) {
    ad_screen_flush(&e->screen, e->cx - e->left, e->cy - e->top);
}

static void refresh(AdEditor *e) {
    paint_all(e);
    flush_at_cursor(e);
}

/* Collapses / restores the wide-screen toolbox. */
static void toggle_sidebar(AdEditor *e) {
    if (!e->sb_room) {
        set_message(e, "The screen is too narrow for the toolbox");
        return;
    }
    e->sb_hidden = !e->sb_hidden;
    layout(e, e->screen.w, e->screen.h);
    clamp_cursor(e);
}

/* The terminal changed size (local window resize, or ^L found a new
   size): lay everything out again. */
static void apply_resize(AdEditor *e) {
    int cols = e->screen.w, rows = e->screen.h;
    ad_door_query_size(e->door, &cols, &rows);
    if (cols != e->screen.w || rows != e->screen.h) {
        if (layout(e, cols, rows)) set_message(e, "Screen is now %dx%d", cols, rows);
    } else {
        ad_screen_full_redraw(&e->screen);
    }
    clamp_cursor(e);
}

/* A centered popup box drawn over the current view. lines[0] is the
   title. Returns the top-left screen position. */
static void popup(AdEditor *e, const char *const *lines, int n, int width) {
    int h = n + 1, row0, col0, r, c;
    if (h > e->view_h) h = e->view_h;
    row0 = (e->view_h - h) / 2;
    col0 = (e->screen.w - width) / 2;
    for (r = 0; r <= h; r++) {
        for (c = 0; c < width; c++) {
            unsigned char g = ' ';
            unsigned char a = POPUP_ATTR;
            if (r == 0 || r == h) {
                g = (c == 0) ? (r == 0 ? 0xC9 : 0xC8) : (c == width - 1) ? (r == 0 ? 0xBB : 0xBC) : 0xCD;
                a = POPUP_FRAME;
            } else if (c == 0 || c == width - 1) {
                g = 0xBA;
                a = POPUP_FRAME;
            }
            ad_screen_put(&e->screen, col0 + c, row0 + r, g, a);
        }
    }
    /* title inset in the top border */
    {
        char t[64];
        snprintf(t, sizeof(t), " %.60s ", lines[0]);
        ad_screen_puts(&e->screen, col0 + 2, row0, t, POPUP_KEY);
    }
    for (r = 1; r < n && r < h; r++) {
        const char *s = lines[r];
        /* a leading "X " is a hotkey, shown highlighted */
        if (s[0] && s[1] == ' ' && s[0] != ' ') {
            ad_screen_put(&e->screen, col0 + 2, row0 + r, (unsigned char)s[0], POPUP_KEY);
            ad_screen_puts(&e->screen, col0 + 3, row0 + r, s + 1, POPUP_ATTR);
        } else {
            ad_screen_puts(&e->screen, col0 + 2, row0 + r, s, POPUP_ATTR);
        }
    }
}

/* Next key for a popup. Mouse releases, drags and wheel turns are
   skipped -- otherwise the release of the very click that opened a
   popup would close it again. */
static AdKey popup_key(AdEditor *e, int local) {
    for (;;) {
        AdKey k = ad_input_get(local);
        if (k.kind == AD_KEY_RESIZE) {
            /* re-layout now; the popup closes (menus) or redraws
               itself at the new size (pickers) */
            apply_resize(e);
            return k;
        }
        if (k.kind == AD_KEY_MOUSE && (k.mrelease || k.mdrag || k.mwheel || k.mbutton == 3))
            continue;
        if (k.kind == AD_KEY_CLOSE) {
            /* the window's close button: back out of this popup like
               Esc; the main loop then runs the quit flow */
            e->close_pending = 1;
            k.kind = AD_KEY_ESCAPE;
        }
        return k;
    }
}

/* Screen row of popup line 0 -- same formula popup() uses. */
static int popup_row0(const AdEditor *e, int n) {
    int h = n + 1;
    if (h > e->view_h) h = e->view_h;
    return (e->view_h - h) / 2;
}

static void tool_menu(AdEditor *e, int local) {
    static const char *const LINES[] = {
        "Tools",
        "D Draw     type text, place glyphs",
        "L Line",
        "B Box      brush/solid/single/double/pixel",
        "E Ellipse  brush/solid/pixel",
        "F Fill     flood fill glyph or color",
        "S Shade    step through \xB0\xB1\xB2\xDB",
        "P Pixel    half-block painting",
        "K Select   copy/cut/move/paste blocks",
        "C Colorize recolor, keep the drawing",
        "T Text     big letters in TheDraw fonts",
        "",
        "  Tab = tool options   Esc = cancel",
    };
    AdKey k;
    paint_view(e);
    paint_status(e);
    int n = (int)(sizeof(LINES) / sizeof(LINES[0]));
    popup(e, LINES, n, 48);
    flush_at_cursor(e);
    k = popup_key(e, local);
    ad_screen_full_redraw(&e->screen);
    if (k.kind == AD_KEY_MOUSE && k.mbutton == 0) {
        /* click on a tool's line picks it */
        int r = k.my - popup_row0(e, n);
        if (r >= 1 && r < n && LINES[r][0] != ' ' && LINES[r][0]) {
            k.kind = AD_KEY_CHAR;
            k.ch = LINES[r][0];
        }
    }
    if (k.kind != AD_KEY_CHAR) return;
    switch (k.ch) {
        case 'd': case 'D': set_tool(e, AD_TOOL_DRAW); break;
        case 'l': case 'L': set_tool(e, AD_TOOL_LINE); break;
        case 'b': case 'B': set_tool(e, AD_TOOL_BOX); break;
        case 'e': case 'E': set_tool(e, AD_TOOL_ELLIPSE); break;
        case 'f': case 'F': set_tool(e, AD_TOOL_FILL); break;
        case 's': case 'S': set_tool(e, AD_TOOL_SHADE); break;
        case 'p': case 'P': set_tool(e, AD_TOOL_PIXEL); break;
        case 'k': case 'K': set_tool(e, AD_TOOL_SELECT); break;
        case 'c': case 'C': set_tool(e, AD_TOOL_COLORIZE); break;
        case 't': case 'T': font_tool(e, local); break;
        default: break;
    }
}

static int help_screen(AdEditor *e, int local) {
    static const char *const LINES[] = {
        "ANetDRAW keys",
        "Arrows Home End PgUp PgDn  move (canvas grows down)",
        "F1-F10 or ^A 1-0   place / pick an F-key glyph",
        "^N ^P (F12 F11)    next / previous glyph set",
        "^F ^B   next fg / bg color     ^U  pick up colors",
        "^E      iCE colors on/off      ^R  mirror off/H/V/both",
        "^T      tools menu             ^V  paste clipboard",
        "^T T    big text in TheDraw fonts (then place it)",
        "^T C    Colorize: drag to recolor, keeps the art",
        "^D      insert / delete a line or column",
        "^K      color picker           ^G  character picker",
        "^Z ^Y   undo / redo",
        "Tab     tool option (pen, box style, fill mode...)",
        "Space   Draw: type space; other tools: anchor/apply",
        "Enter   Draw: new line;  other tools: same as Space",
        "Ins     Draw: insert mode   Bksp  erase / lighten",
        "Esc     back out: paste > mark > tool > main menu",
        "^W      show / hide the toolbox (wide screens)",
        "^L      redraw screen",
        "Mouse   left: draw / drag shapes   right: pick up",
        "        wheel: scroll   click the status bar items",
        "",
        "  press any key",
    };
    AdKey k;
    paint_all(e);
    popup(e, LINES, (int)(sizeof(LINES) / sizeof(LINES[0])), 58);
    flush_at_cursor(e);
    k = popup_key(e, local);
    ad_screen_full_redraw(&e->screen);
    return k.kind != AD_KEY_HANGUP;
}

static int confirm_quit(AdEditor *e, int local) {
    AdKey k;
    set_message(e, "Quit ANetDRAW? Unsaved work is lost. (Y/N)");
    paint_all(e);
    ad_screen_flush(&e->screen, 17 + 43, e->screen.h - 1);
    k = popup_key(e, local);
    e->message[0] = 0;
    if (k.kind == AD_KEY_HANGUP) return 1;
    return k.kind == AD_KEY_CHAR && (k.ch == 'y' || k.ch == 'Y');
}

/* ------------------------------------------------------------ pickers */

/* Double-line frame with a title inset in the top border. */
static void frame(AdEditor *e, int col0, int row0, int width, int height, const char *title) {
    int r, c;
    char t[64];
    for (r = 0; r < height; r++) {
        for (c = 0; c < width; c++) {
            unsigned char g = ' ', a = POPUP_ATTR;
            if (r == 0 || r == height - 1) {
                g = (c == 0) ? (r == 0 ? 0xC9 : 0xC8)
                  : (c == width - 1) ? (r == 0 ? 0xBB : 0xBC) : 0xCD;
                a = POPUP_FRAME;
            } else if (c == 0 || c == width - 1) {
                g = 0xBA;
                a = POPUP_FRAME;
            }
            ad_screen_put(&e->screen, col0 + c, row0 + r, g, a);
        }
    }
    snprintf(t, sizeof(t), " %s ", title);
    ad_screen_puts(&e->screen, col0 + 2, row0, t, POPUP_KEY);
}

/* Right-click menu for the Select tool, opened at the pointer. Returns
   the chosen command letter, or 0. Greyed items can't be picked. */
static char block_menu(AdEditor *e, int local, int mx, int my) {
    static const struct { char key; const char *label; } ITEMS[] = {
        { 'C', "C  Copy" }, { 'X', "X  Cut" }, { 'P', "P  Paste" }, { 'D', "D  Delete" },
        { 'F', "F  Fill with brush" }, { 'R', "R  Recolor (keep art)" }, { 'M', "M  Move" },
        { 'H', "H  Flip left-right" }, { 'V', "V  Flip upside-down" }, { 'S', "S  Swap FG / BG" },
        { 'L', "L  Replace a color..." }, { 'N', "N  Center each row" }, { 'A', "A  Select all" } };
    int n = (int)(sizeof(ITEMS) / sizeof(ITEMS[0])), width = 24, height = n + 2, sel = 0, i;
    int col0 = mx + 1, row0 = my;
    int marked = e->anchored && !e->pasting;
    if (col0 + width > e->screen.w) col0 = mx - width;
    if (col0 < 0) col0 = 0;
    if (row0 + height > e->view_h) row0 = e->view_h - height;
    if (row0 < 0) row0 = 0;
    for (;;) {
        AdKey k;
        paint_all(e);
        frame(e, col0, row0, width, height, "Block");
        for (i = 0; i < n; i++) {
            int ok = strchr("CXDFMRHVSLN", ITEMS[i].key) ? marked : ITEMS[i].key == 'P' ? e->clip.cells != NULL : 1;
            char line[32];
            snprintf(line, sizeof(line), " %-*s", width - 3, ITEMS[i].label);
            ad_screen_puts(&e->screen, col0 + 1, row0 + 1 + i, line,
                           i == sel ? AD_ATTR(0, 3) : ok ? POPUP_ATTR : AD_ATTR(8, 1));
            if (ok && i != sel) ad_screen_put(&e->screen, col0 + 2, row0 + 1 + i, (unsigned char)ITEMS[i].key, POPUP_KEY);
        }
        ad_screen_flush(&e->screen, col0 + 2, row0 + 1 + sel);
        k = popup_key(e, local);
        if (k.kind == AD_KEY_HANGUP || k.kind == AD_KEY_ESCAPE) break;
        if (k.kind == AD_KEY_UP) sel = (sel + n - 1) % n;
        else if (k.kind == AD_KEY_DOWN) sel = (sel + 1) % n;
        else if (k.kind == AD_KEY_ENTER) k.kind = AD_KEY_CHAR, k.ch = ITEMS[sel].key;
        else if (k.kind == AD_KEY_MOUSE) {
            if (k.mbutton != 0 || k.mx < col0 || k.mx >= col0 + width || k.my <= row0 || k.my > row0 + n) break;
            k.kind = AD_KEY_CHAR;
            k.ch = ITEMS[k.my - row0 - 1].key;
        }
        if (k.kind == AD_KEY_CHAR) {
            char c = (char)(k.ch & ~0x20);
            for (i = 0; i < n; i++) {
                if (ITEMS[i].key != c) continue;
                if (strchr("CXDFMRHVSLN", c) && !marked) break;
                if (c == 'P' && !e->clip.cells) break;
                ad_screen_full_redraw(&e->screen);
                return c;
            }
        }
    }
    ad_screen_full_redraw(&e->screen);
    return 0;
}

/* Select tool, L: every cell in the marked block that has one color gets
   another -- in the foreground, the background, or both. */
static void replace_color_dialog(AdEditor *e, int local) {
    int from = e->fg, to = e->fg, where = 0, field = 0;  /* where: 0 fg, 1 bg, 2 both */
    int width = 44, height = 10;
    static const char *const WHERE[3] = { "foreground", "background", "fg and bg" };
    if (!e->anchored) return;
    for (;;) {
        AdKey k;
        int col0 = (e->screen.w - width) / 2, row0 = (e->view_h - height) / 2, i, r;
        char buf[64];
        if (row0 < 0) row0 = 0;
        paint_all(e);
        frame(e, col0, row0, width, height, "Replace a color");
        for (r = 0; r < 2; r++) {
            int cur = r ? to : from;
            ad_screen_puts(&e->screen, col0 + 2, row0 + 2 + r * 2, r ? "Change to" : "Find     ",
                           field == r ? AD_ATTR(0, 3) : POPUP_ATTR);
            for (i = 0; i < 16; i++) {
                ad_screen_put(&e->screen, col0 + 13 + i * 2, row0 + 2 + r * 2, 0xDB, AD_ATTR(i, 1));
                ad_screen_put(&e->screen, col0 + 14 + i * 2, row0 + 2 + r * 2, 0xDB, AD_ATTR(i, 1));
            }
            ad_screen_put(&e->screen, col0 + 13 + cur * 2, row0 + 3 + r * 2, 0x1E, AD_ATTR(15, 1));
        }
        snprintf(buf, sizeof(buf), "In the %-10s  (W to switch)", WHERE[where]);
        ad_screen_puts(&e->screen, col0 + 2, row0 + 6, buf, field == 2 ? AD_ATTR(0, 3) : POPUP_ATTR);
        ad_screen_puts(&e->screen, col0 + 2, row0 + height - 1,
                       " \x1b\x1a pick  Tab=next  Enter=replace  Esc ", POPUP_KEY);
        ad_screen_flush(&e->screen, col0 + 13 + (field == 1 ? to : from) * 2, row0 + 2 + (field == 1 ? 2 : 0));
        k = popup_key(e, local);
        switch (k.kind) {
            case AD_KEY_HANGUP: case AD_KEY_ESCAPE: ad_screen_full_redraw(&e->screen); return;
            case AD_KEY_TAB: case AD_KEY_DOWN: field = (field + 1) % 3; break;
            case AD_KEY_UP: field = (field + 2) % 3; break;
            case AD_KEY_LEFT:
                if (field == 0) from = (from + 15) % 16;
                else if (field == 1) to = (to + 15) % 16;
                else where = (where + 2) % 3;
                break;
            case AD_KEY_RIGHT:
                if (field == 0) from = (from + 1) % 16;
                else if (field == 1) to = (to + 1) % 16;
                else where = (where + 1) % 3;
                break;
            case AD_KEY_CHAR:
                if ((k.ch & ~0x20) == 'W') where = (where + 1) % 3;
                break;
            case AD_KEY_MOUSE:
                if (k.mbutton == 2) { ad_screen_full_redraw(&e->screen); return; }
                if (k.mbutton == 0 && k.mx >= col0 + 13 && k.mx < col0 + 45) {
                    int c = (k.mx - col0 - 13) / 2;
                    if (k.my == row0 + 2) { from = c; field = 0; }
                    else if (k.my == row0 + 4) { to = c; field = 1; }
                }
                if (k.mbutton == 0 && k.my == row0 + 6) { where = (where + 1) % 3; field = 2; }
                break;
            case AD_KEY_ENTER: {
                AdSurface s = surface(e, 1);
                int x0, y0, x1, y1, x, y, n = 0;
                sel_rect(e, &x0, &y0, &x1, &y1);
                s.mirror = 0;
                for (y = y0; y <= y1; y++)
                    for (x = x0; x <= x1; x++) {
                        AdCell c = ad_canvas_get(&e->canvas, x, y);
                        int f = AD_ATTR_FG(c.attr), b = AD_ATTR_BG(c.attr);
                        if (where != 1 && f == from) f = to;
                        if (where != 0 && b == from) b = e->canvas.ice ? to : (to & 7);
                        if (AD_ATTR(f, b) != c.attr) { ad_surf_put(&s, x, y, c.ch, AD_ATTR(f, b)); n++; }
                    }
                ad_screen_full_redraw(&e->screen);
                set_message(e, "Replaced color %d with %d in %d cell%s", from, to, n, n == 1 ? "" : "s");
                return;
            }
            default: break;
        }
    }
}

/* ------------------------------------------------------------ rows & columns */

/* Last row the drawing uses: a sized canvas's bottom, else the lowest
   row with anything on it (at least the cursor's). */
static int used_bottom(const AdEditor *e) {
    int ew, eh;
    if (e->canvas.fixed) return e->canvas.h - 1;
    ad_canvas_extent(&e->canvas, &ew, &eh);
    return (eh > e->cy + 1 ? eh : e->cy + 1) - 1;
}

/* TheDraw's insert / delete line and column, at the cursor. Everything
   goes through a committing surface, so it's one undo step. What moves
   past a sized canvas's edge is gone; a growing canvas grows a row. */
static void rows_cols(AdEditor *e, char op) {
    AdSurface s = surface(e, 1);
    int x, y, bottom = used_bottom(e), w = e->canvas.w;
    s.mirror = 0;
    switch (op) {
        case 'I':  /* insert a line: rows move down */
            if (!e->canvas.fixed && bottom + 1 < AD_CANVAS_MAX_H) bottom++;
            for (y = bottom; y > e->cy; y--)
                for (x = 0; x < w; x++) {
                    AdCell c = ad_canvas_get(&e->canvas, x, y - 1);
                    ad_surf_put(&s, x, y, c.ch, c.attr);
                }
            for (x = 0; x < w; x++) ad_surf_put(&s, x, e->cy, ' ', AD_BLANK_ATTR);
            set_message(e, "Inserted a line at row %d", e->cy + 1);
            break;
        case 'Y':  /* delete the line: rows move up */
            for (y = e->cy; y < bottom; y++)
                for (x = 0; x < w; x++) {
                    AdCell c = ad_canvas_get(&e->canvas, x, y + 1);
                    ad_surf_put(&s, x, y, c.ch, c.attr);
                }
            for (x = 0; x < w; x++) ad_surf_put(&s, x, bottom, ' ', AD_BLANK_ATTR);
            set_message(e, "Deleted row %d", e->cy + 1);
            break;
        case 'C':  /* insert a column: the rest moves right */
            for (y = 0; y <= bottom; y++) {
                for (x = w - 1; x > e->cx; x--) {
                    AdCell c = ad_canvas_get(&e->canvas, x - 1, y);
                    ad_surf_put(&s, x, y, c.ch, c.attr);
                }
                ad_surf_put(&s, e->cx, y, ' ', AD_BLANK_ATTR);
            }
            set_message(e, "Inserted a column at column %d", e->cx + 1);
            break;
        case 'X':  /* delete the column: the rest moves left */
            for (y = 0; y <= bottom; y++) {
                for (x = e->cx; x < w - 1; x++) {
                    AdCell c = ad_canvas_get(&e->canvas, x + 1, y);
                    ad_surf_put(&s, x, y, c.ch, c.attr);
                }
                ad_surf_put(&s, w - 1, y, ' ', AD_BLANK_ATTR);
            }
            set_message(e, "Deleted column %d", e->cx + 1);
            break;
        default:
            break;
    }
}

/* ^D: the rows & columns menu. */
static void rows_cols_menu(AdEditor *e, int local) {
    static const char *const LINES[] = {
        "Lines & columns (at the cursor)",
        "I Insert a line",
        "Y Delete this line",
        "C Insert a column",
        "X Delete this column",
        "",
        "  Esc = cancel",
    };
    int n = (int)(sizeof(LINES) / sizeof(LINES[0]));
    AdKey k;
    paint_all(e);
    popup(e, LINES, n, 38);
    flush_at_cursor(e);
    k = popup_key(e, local);
    ad_screen_full_redraw(&e->screen);
    if (k.kind == AD_KEY_MOUSE && k.mbutton == 0) {
        int r = k.my - popup_row0(e, n);
        if (r >= 1 && r <= 4) { k.kind = AD_KEY_CHAR; k.ch = LINES[r][0]; }
    }
    if (k.kind == AD_KEY_CHAR && strchr("IYCX", k.ch & ~0x20)) rows_cols(e, (char)(k.ch & ~0x20));
}

/* TheDraw-style color grid: foreground across, background down, each
   swatch drawn in its own colors -- one pick sets both. */
static int color_picker(AdEditor *e, int local) {
    int fg = e->fg, bg = e->bg;
    int nbg = e->canvas.ice ? 16 : 8;
    int width = 16 * 3 + 4, height = nbg + 3;

    for (;;) {
        AdKey k;
        int col0 = (e->screen.w - width) / 2;
        int row0 = (e->view_h - height) / 2;
        if (row0 < 0) row0 = 0;
        int f, b;
        char foot[64];
        paint_all(e);
        frame(e, col0, row0, width, height, e->canvas.ice ? "Colors (iCE)" : "Colors");
        for (b = 0; b < nbg; b++) {
            for (f = 0; f < 16; f++) {
                int x = col0 + 2 + f * 3, y = row0 + 1 + b;
                unsigned char a = AD_ATTR(f, b);
                int sel = (f == fg && b == bg);
                /* the selected swatch gets brackets in a color that
                   shows up against its own background */
                unsigned char ba = AD_ATTR((b == 7 || b >= 8) ? 0 : 15, b);
                ad_screen_put(&e->screen, x, y, sel ? '[' : ' ', sel ? ba : a);
                ad_screen_put(&e->screen, x + 1, y, 0xFE, a);
                ad_screen_put(&e->screen, x + 2, y, sel ? ']' : ' ', sel ? ba : a);
            }
        }
        snprintf(foot, sizeof(foot), " Arrows  Enter=pick  Esc   F%02d/B%02d ", fg, bg);
        ad_screen_puts(&e->screen, col0 + 2, row0 + height - 1, foot, POPUP_KEY);
        ad_screen_flush(&e->screen, col0 + 3 + fg * 3, row0 + 1 + bg);

        k = popup_key(e, local);
        if (k.kind == AD_KEY_MOUSE) {
            int cf = (k.mx - col0 - 2) / 3, cb = k.my - row0 - 1;
            if (k.mbutton == 0 && k.mx >= col0 + 2 && cf >= 0 && cf < 16 && cb >= 0 && cb < nbg) {
                fg = cf;
                bg = cb;
                k.kind = AD_KEY_ENTER;  /* a click picks it */
            } else if (k.mbutton == 2) {
                k.kind = AD_KEY_ESCAPE;
            }
        }
        switch (k.kind) {
            case AD_KEY_HANGUP: ad_screen_full_redraw(&e->screen); return 0;
            case AD_KEY_LEFT:   fg = (fg + 15) % 16; break;
            case AD_KEY_RIGHT:  fg = (fg + 1) % 16; break;
            case AD_KEY_UP:     bg = (bg + nbg - 1) % nbg; break;
            case AD_KEY_DOWN:   bg = (bg + 1) % nbg; break;
            case AD_KEY_ENTER:
                e->fg = fg;
                e->bg = bg;
                ad_screen_full_redraw(&e->screen);
                return 1;
            case AD_KEY_ESCAPE:
                ad_screen_full_redraw(&e->screen);
                return 1;
            default: break;
        }
    }
}

/* All 256 CP437 glyphs, 32 x 8. Enter makes the glyph the brush (and
   places it in the Draw tool); F1-F10 put it in that slot of the
   current F-key set. */
static int char_picker(AdEditor *e, int local) {
    static int sel = 0xDB;
    int width = 32 * 2 + 3, height = 8 + 3;

    for (;;) {
        AdKey k;
        int col0 = (e->screen.w - width) / 2;
        int row0 = (e->view_h - height) / 2;
        if (row0 < 0) row0 = 0;
        int i;
        char foot[72];
        paint_all(e);
        frame(e, col0, row0, width, height, "Characters");
        for (i = 0; i < 256; i++) {
            int x = col0 + 2 + (i % 32) * 2, y = row0 + 1 + i / 32;
            ad_screen_put(&e->screen, x, y, (unsigned char)i,
                          i == sel ? AD_ATTR(0, 7) : AD_ATTR(15, 1));
        }
        snprintf(foot, sizeof(foot), " Enter=brush  F1-F10=put in set %02d  Esc   #%03d ",
                 e->fset + 1, sel);
        ad_screen_puts(&e->screen, col0 + 2, row0 + height - 1, foot, POPUP_KEY);
        ad_screen_flush(&e->screen, col0 + 2 + (sel % 32) * 2, row0 + 1 + sel / 32);

        k = popup_key(e, local);
        if (k.kind == AD_KEY_MOUSE) {
            int cx = (k.mx - col0 - 2) / 2, cy = k.my - row0 - 1;
            if (k.mbutton == 0 && k.mx >= col0 + 2 && cx >= 0 && cx < 32 && cy >= 0 && cy < 8) {
                sel = cy * 32 + cx;
                k.kind = AD_KEY_ENTER;
            } else if (k.mbutton == 2) {
                k.kind = AD_KEY_ESCAPE;
            }
        }
        switch (k.kind) {
            case AD_KEY_HANGUP: ad_screen_full_redraw(&e->screen); return 0;
            case AD_KEY_LEFT:   sel = (sel + 255) % 256; break;
            case AD_KEY_RIGHT:  sel = (sel + 1) % 256; break;
            case AD_KEY_UP:     sel = (sel + 256 - 32) % 256; break;
            case AD_KEY_DOWN:   sel = (sel + 32) % 256; break;
            case AD_KEY_FN:
                if (k.fn >= 1 && k.fn <= 10) {
                    e->fkeys[e->fset][k.fn - 1] = (unsigned char)sel;
                    set_message(e, "Set %02d F%d is now glyph #%03d", e->fset + 1, k.fn, sel);
                }
                break;
            case AD_KEY_ENTER:
                ad_screen_full_redraw(&e->screen);
                e->brush = (unsigned char)sel;
                if (e->tool == AD_TOOL_DRAW && !e->pasting) place(e, e->brush);
                return 1;
            case AD_KEY_ESCAPE:
                ad_screen_full_redraw(&e->screen);
                return 1;
            default: break;
        }
    }
}

/* ------------------------------------------------------------ canvas size */

/* Width x height dialog, like the "canvas size" box in other drawing
   programs. Type digits into the fields (Tab / arrows switch), or pick
   a preset. Shrinking below the art asks for a second Enter first. */
static int canvas_dialog(AdEditor *e, int local) {
    static const struct { char key; int w, h; } PRESETS[] = {
        { 'A', 80, 25 }, { 'B', 80, 50 }, { 'C', 132, 37 }, { 'D', 160, 50 }
    };
    int vals[2], field = 0, fresh = 1, confirm = 0, i;
    int width = 52, height = 11;
    /* E: the canvas fills the drawing area -- all the columns beside the
       toolbox on a wide screen (all of them on a narrower one) */
    int fit_w = e->screen.w - (SIDEBAR_MIN + 1) >= AD_MIN_COLS ? e->screen.w - (SIDEBAR_MIN + 1) : e->screen.w;
    int fit_h = e->view_h;
    char err[64] = "";
    if (fit_w > AD_CANVAS_MAX_W) fit_w = AD_CANVAS_MAX_W;
    vals[0] = e->canvas.w;
    vals[1] = e->canvas.h;

    for (;;) {
        AdKey k;
        char buf[80];
        int col0 = (e->screen.w - width) / 2;
        int row0 = (e->view_h - height) / 2;
        int ew, eh;
        if (row0 < 0) row0 = 0;
        paint_all(e);
        frame(e, col0, row0, width, height, "Canvas size");
        snprintf(buf, sizeof(buf), "Width  [%4d]  columns, 1-%d", vals[0], AD_CANVAS_MAX_W);
        ad_screen_puts(&e->screen, col0 + 2, row0 + 2, buf, POPUP_ATTR);
        snprintf(buf, sizeof(buf), "Height [%4d]  rows, 1-%d", vals[1], AD_CANVAS_MAX_H);
        ad_screen_puts(&e->screen, col0 + 2, row0 + 3, buf, POPUP_ATTR);
        /* the active field's value highlighted */
        snprintf(buf, sizeof(buf), "%4d", vals[field]);
        ad_screen_puts(&e->screen, col0 + 10, row0 + 2 + field, buf, AD_ATTR(0, 7));
        ad_screen_puts(&e->screen, col0 + 2, row0 + 5, "Presets:", POPUP_ATTR);
        for (i = 0; i < 4; i++) {
            snprintf(buf, sizeof(buf), "%c %dx%d", PRESETS[i].key, PRESETS[i].w, PRESETS[i].h);
            ad_screen_put(&e->screen, col0 + 11 + i * 10, row0 + 5, (unsigned char)PRESETS[i].key, POPUP_KEY);
            ad_screen_puts(&e->screen, col0 + 12 + i * 10, row0 + 5, buf + 1, POPUP_ATTR);
        }
        snprintf(buf, sizeof(buf), "E Fit the screen: %dx%d", fit_w, fit_h);
        ad_screen_puts(&e->screen, col0 + 11, row0 + 6, buf, POPUP_ATTR);
        ad_screen_put(&e->screen, col0 + 11, row0 + 6, 'E', POPUP_KEY);
        ad_canvas_extent(&e->canvas, &ew, &eh);
        snprintf(buf, sizeof(buf), "Your art uses %dx%d.", ew, eh);
        ad_screen_puts(&e->screen, col0 + 2, row0 + 7, buf, AD_ATTR(7, 1));
        if (err[0])
            ad_screen_puts(&e->screen, col0 + 2, row0 + 8, err, AD_ATTR(14, 1));
        else if (!e->canvas.fixed)
            ad_screen_puts(&e->screen, col0 + 2, row0 + 8,
                           "(It grows as you draw until you set a size.)", AD_ATTR(7, 1));
        ad_screen_puts(&e->screen, col0 + 2, row0 + height - 1,
                       " Tab=field  Enter=OK  Esc=cancel ", POPUP_KEY);
        ad_screen_flush(&e->screen, col0 + 13, row0 + 2 + field);

        k = popup_key(e, local);
        if (k.kind != AD_KEY_ENTER) confirm = 0;
        switch (k.kind) {
            case AD_KEY_HANGUP:
                ad_screen_full_redraw(&e->screen);
                return 0;
            case AD_KEY_ESCAPE:
                ad_screen_full_redraw(&e->screen);
                return 1;
            case AD_KEY_TAB: case AD_KEY_UP: case AD_KEY_DOWN:
                field = !field;
                fresh = 1;
                break;
            case AD_KEY_BACKSPACE:
                vals[field] /= 10;
                fresh = 0;
                break;
            case AD_KEY_CHAR:
                if (k.ch >= '0' && k.ch <= '9') {
                    int v = (fresh ? 0 : vals[field]) * 10 + (k.ch - '0');
                    if (v <= (field ? AD_CANVAS_MAX_H : AD_CANVAS_MAX_W)) vals[field] = v;
                    fresh = 0;
                } else if ((k.ch & ~0x20) == 'E') {
                    vals[0] = fit_w;
                    vals[1] = fit_h;
                    fresh = 1;
                } else {
                    for (i = 0; i < 4; i++)
                        if ((k.ch & ~0x20) == PRESETS[i].key) {
                            vals[0] = PRESETS[i].w;
                            vals[1] = PRESETS[i].h;
                            fresh = 1;
                        }
                }
                err[0] = 0;
                break;
            case AD_KEY_MOUSE:
                if (k.mbutton == 2) { ad_screen_full_redraw(&e->screen); return 1; }
                if (k.mbutton == 0 && k.my == row0 + 5 && k.mx >= col0 + 11) {
                    i = (k.mx - col0 - 11) / 10;
                    if (i >= 0 && i < 4) { vals[0] = PRESETS[i].w; vals[1] = PRESETS[i].h; fresh = 1; }
                } else if (k.mbutton == 0 && k.my == row0 + 6 && k.mx >= col0 + 11) {
                    vals[0] = fit_w;
                    vals[1] = fit_h;
                    fresh = 1;
                } else if (k.mbutton == 0 && (k.my == row0 + 2 || k.my == row0 + 3)) {
                    field = k.my - row0 - 2;
                    fresh = 1;
                }
                break;
            case AD_KEY_ENTER:
                if (vals[0] < 1 || vals[1] < 1) {
                    snprintf(err, sizeof(err), "Width and height must be at least 1.");
                    break;
                }
                if ((vals[0] < ew || vals[1] < eh) && !confirm) {
                    snprintf(err, sizeof(err), "That crops your art! Enter again to crop.");
                    confirm = 1;
                    break;
                }
                if (vals[0] == e->canvas.w && vals[1] == e->canvas.h) {
                    ad_screen_full_redraw(&e->screen);
                    return 1;
                }
                if (!ad_canvas_resize(&e->canvas, vals[0], vals[1])) {
                    snprintf(err, sizeof(err), "Not enough memory for that size.");
                    break;
                }
                /* recorded undo steps point at old positions */
                ad_undo_clear(&e->undo);
                e->anchored = 0;
                e->pasting = 0;
                layout(e, e->screen.w, e->screen.h);
                clamp_cursor(e);
                e->dirty = 1;
                set_message(e, "Canvas is now %dx%d (undo history cleared)", vals[0], vals[1]);
                return 1;
            default:
                break;
        }
    }
}

/* ------------------------------------------------------------ TheDraw fonts */

/* The font list is read once per session, the first time it's needed. */
static AdTdfIndex g_fonts;
static int g_fonts_scanned = 0, g_font_sel = 0;
static char g_font_text[48] = "ANetDRAW";
static char g_font_find[16] = "";

static int contains_nocase(const char *n, const char *f) {
    size_t nl = strlen(n), fl = strlen(f), k, j;
    for (k = 0; k + fl <= nl; k++) {
        for (j = 0; j < fl && tolower((unsigned char)n[k + j]) == tolower((unsigned char)f[j]); j++) {}
        if (j == fl) return 1;
    }
    return 0;
}

/* Find matches the font's name or its file's name. */
static int font_matches(int i) {
    const char *file = g_fonts.files[g_fonts.e[i].file];
    const char *base = strrchr(file, '/'), *bs = strrchr(file, '\\');
    if (bs && (!base || bs > base)) base = bs;
    if (!g_font_find[0]) return 1;
    return contains_nocase(g_fonts.e[i].name, g_font_find) ||
           contains_nocase(base ? base + 1 : file, g_font_find);
}

/* ^T T: pick a TheDraw font, type the text, then place it like a paste. */
static void font_tool(AdEditor *e, int local) {
    static const char *const TYPE[] = { "outline", "block", "color" };
    int *vis = NULL, nvis = 0, sel_pos = 0, scroll = 0, focus_find = 0, loaded = -1, done = 0;
    AdTdfFont font;
    AdClipboard preview = { 0, 0, NULL };
    int preview_ok = 0;

    memset(&font, 0, sizeof(font));
    if (!g_fonts_scanned) {
        set_message(e, "Loading fonts...");
        paint_status(e);
        flush_at_cursor(e);
        ad_tdf_scan(e->door->fonts_dir, &g_fonts);
        g_fonts_scanned = 1;
        e->message[0] = 0;
    }
    if (g_fonts.n == 0) {
        set_message(e, "No TheDraw fonts found -- install the ANetDRAW font pack (see README)");
        return;
    }
    vis = (int *)malloc((size_t)g_fonts.n * sizeof(int));
    if (!vis) return;

    while (!done) {
        int width = e->screen.w - 2 > 132 ? 132 : e->screen.w - 2;
        int height = e->view_h;
        int col0 = (e->screen.w - width) / 2, row0 = 0;
        int prev_rows = height - 9 < 12 ? height - 9 : 12;
        int list_rows, i, cur, fx;
        char line[200];
        AdKey k;

        if (prev_rows < 3) prev_rows = 3;
        list_rows = height - 6 - prev_rows;
        if (list_rows < 2) list_rows = 2;

        /* the fonts matching Find, keeping the selection if it still matches */
        nvis = 0;
        cur = g_font_sel;
        for (i = 0; i < g_fonts.n; i++)
            if (font_matches(i)) vis[nvis++] = i;
        sel_pos = 0;
        for (i = 0; i < nvis; i++)
            if (vis[i] == cur) { sel_pos = i; break; }
        if (nvis) g_font_sel = vis[sel_pos];
        if (sel_pos < scroll) scroll = sel_pos;
        if (sel_pos >= scroll + list_rows) scroll = sel_pos - list_rows + 1;

        /* preview in the selected font */
        if (nvis && loaded != g_font_sel) {
            ad_tdf_free(&font);
            loaded = ad_tdf_load(&g_fonts, g_font_sel, &font) ? g_font_sel : -2;
        }
        preview_ok = nvis && loaded >= 0 && ad_tdf_render(&font, g_font_text, cur_attr(e), &preview);

        paint_all(e);
        frame(e, col0, row0, width, height, "TheDraw fonts");
        snprintf(line, sizeof(line), "Text:");
        ad_screen_puts(&e->screen, col0 + 2, row0 + 1, line, POPUP_ATTR);
        fx = col0 + 8;
        snprintf(line, sizeof(line), "%-*.*s", 34, 34, g_font_text);
        ad_screen_puts(&e->screen, fx, row0 + 1, line, focus_find ? AD_ATTR(15, 0) : AD_ATTR(0, 7));
        ad_screen_puts(&e->screen, col0 + 44, row0 + 1, "Find:", POPUP_ATTR);
        snprintf(line, sizeof(line), "%-*.*s", 14, 14, g_font_find);
        ad_screen_puts(&e->screen, col0 + 50, row0 + 1, line, focus_find ? AD_ATTR(0, 7) : AD_ATTR(15, 0));
        snprintf(line, sizeof(line), "%d font%s", nvis, nvis == 1 ? "" : "s");
        ad_screen_puts(&e->screen, col0 + width - 2 - (int)strlen(line), row0 + 1, line, AD_ATTR(7, 1));

        for (i = 0; i < list_rows; i++) {
            int idx = scroll + i;
            if (idx >= nvis) {
                snprintf(line, sizeof(line), "%-*s", width - 4, (i == 0 && nvis == 0) ? "  No font name matches" : "");
                ad_screen_puts(&e->screen, col0 + 2, row0 + 2 + i, line, AD_ATTR(7, 1));
                continue;
            }
            {
                const AdTdfEntry *en = &g_fonts.e[vis[idx]];
                const char *file = g_fonts.files[en->file];
                const char *slash = strrchr(file, '/'), *bslash = strrchr(file, '\\');
                if (bslash && (!slash || bslash > slash)) slash = bslash;
                snprintf(line, sizeof(line), " %-13s %-8s %-*.*s", en->name, en->type < 3 ? TYPE[en->type] : "?",
                         width - 29, width - 29, slash ? slash + 1 : file);
            }
            ad_screen_puts(&e->screen, col0 + 2, row0 + 2 + i, line, idx == sel_pos ? AD_ATTR(0, 3) : AD_ATTR(15, 1));
        }
        /* preview box */
        {
            int py0 = row0 + 2 + list_rows, x, y;
            for (x = 1; x < width - 1; x++) ad_screen_put(&e->screen, col0 + x, py0, 0xC4, POPUP_FRAME);
            ad_screen_puts(&e->screen, col0 + 3, py0, " Preview ", POPUP_KEY);
            for (y = 0; y < prev_rows; y++)
                for (x = 1; x < width - 1; x++) {
                    AdCell c = { ' ', AD_ATTR(7, 0) };
                    if (preview_ok && y < preview.h && x - 2 < preview.w && x >= 2) {
                        c = preview.cells[(size_t)y * preview.w + (x - 2)];
                        if (c.ch == 0) { c.ch = ' '; c.attr = AD_ATTR(7, 0); }
                    }
                    ad_screen_put(&e->screen, col0 + x, py0 + 1 + y, c.ch, c.attr);
                }
            if (!preview_ok && nvis)
                ad_screen_puts(&e->screen, col0 + 3, py0 + 1, "(this font has none of those letters)", AD_ATTR(7, 0));
        }
        ad_screen_puts(&e->screen, col0 + 2, row0 + height - 2,
                       "Type the text  Tab=find a font  \x18\x19=pick  Enter=place it  Esc=cancel", AD_ATTR(7, 1));
        if (focus_find)
            ad_screen_flush(&e->screen, col0 + 50 + (int)strlen(g_font_find), row0 + 1);
        else
            ad_screen_flush(&e->screen, fx + ((int)strlen(g_font_text) < 34 ? (int)strlen(g_font_text) : 33), row0 + 1);

        k = popup_key(e, local);
        switch (k.kind) {
            case AD_KEY_HANGUP: case AD_KEY_ESCAPE: done = 1; preview_ok = 0; break;
            case AD_KEY_UP: sel_pos--; break;
            case AD_KEY_DOWN: sel_pos++; break;
            case AD_KEY_PGUP: sel_pos -= list_rows; break;
            case AD_KEY_PGDN: sel_pos += list_rows; break;
            case AD_KEY_HOME: sel_pos = 0; break;
            case AD_KEY_END: sel_pos = nvis - 1; break;
            case AD_KEY_TAB: focus_find = !focus_find; break;
            case AD_KEY_BACKSPACE: {
                char *s = focus_find ? g_font_find : g_font_text;
                size_t n = strlen(s);
                if (n) s[n - 1] = 0;
                break;
            }
            case AD_KEY_CHAR: {
                char *s = focus_find ? g_font_find : g_font_text;
                size_t cap = focus_find ? sizeof(g_font_find) : sizeof(g_font_text), n = strlen(s);
                if (n + 1 < cap) { s[n] = k.ch; s[n + 1] = 0; }
                if (focus_find) scroll = 0;
                break;
            }
            case AD_KEY_MOUSE:
                if (k.mbutton == 2) { done = 1; preview_ok = 0; break; }
                if (k.mwheel) { sel_pos += 3 * k.mwheel; break; }
                if (k.mbutton == 0 && k.my >= row0 + 2 && k.my < row0 + 2 + list_rows && scroll + k.my - row0 - 2 < nvis) {
                    int idx = scroll + (k.my - row0 - 2);
                    if (idx != sel_pos) { sel_pos = idx; break; }
                    /* a second click on the same font: use it */
                } else {
                    break;
                }
                /* FALLTHROUGH */
            case AD_KEY_ENTER:
                if (preview_ok) done = 2;
                break;
            default: break;
        }
        if (sel_pos >= nvis) sel_pos = nvis - 1;
        if (sel_pos < 0) sel_pos = 0;
        if (nvis) g_font_sel = vis[sel_pos];
    }
    free(vis);
    ad_tdf_free(&font);
    ad_screen_full_redraw(&e->screen);
    if (done == 2 && preview_ok) {
        /* the text becomes the clipboard, placed like a paste */
        ad_clip_free(&e->clip);
        e->clip = preview;
        e->paste_transparent = 1;
        start_paste(e);
        set_message(e, "Place it: arrows/mouse move  Enter=stamp  T=transparent  Esc=done");
    } else {
        ad_clip_free(&preview);
    }
}

/* ------------------------------------------------------------ files */

static int filter_printable(int c) { return c >= 0x20 && c < 0x7F; }
static int filter_sysop_name(int c) { return c >= 0x20 && c < 0x7F && c != '/' && c != '\\'; }
static int filter_caller_name(int c) { return isalnum(c) || c == ' ' || c == '_' || c == '-' || c == '.'; }

/* A small form: nfields labelled text fields. Tab / arrows move between
   fields, Enter accepts, Esc cancels. Returns 1 ok, 0 cancel, -1 hangup. */
static int form_dialog(AdEditor *e, int local, const char *title, int nfields,
                       const char *const *labels, char **bufs, const int *maxlens,
                       int (*filter)(int), const char *note) {
    int field = 0, i, width = 64, height = nfields + 5;
    for (;;) {
        AdKey k;
        int col0 = (e->screen.w - width) / 2, row0 = (e->view_h - height) / 2;
        int fx = col0 + 12, fw = width - 15;
        char line[80];
        if (row0 < 0) row0 = 0;
        paint_all(e);
        frame(e, col0, row0, width, height, title);
        for (i = 0; i < nfields; i++) {
            size_t n = strlen(bufs[i]);
            const char *shown = n > (size_t)(fw - 1) ? bufs[i] + n - (fw - 1) : bufs[i];
            ad_screen_puts(&e->screen, col0 + 2, row0 + 2 + i, labels[i], POPUP_ATTR);
            snprintf(line, sizeof(line), "%-*.*s", fw, fw, shown);
            ad_screen_puts(&e->screen, fx, row0 + 2 + i, line, i == field ? AD_ATTR(0, 7) : AD_ATTR(15, 0));
        }
        if (note) ad_screen_puts(&e->screen, col0 + 2, row0 + height - 2, note, AD_ATTR(7, 1));
        ad_screen_puts(&e->screen, col0 + 2, row0 + height - 1, " Tab=next field  Enter=OK  Esc=cancel ", POPUP_KEY);
        {
            size_t n = strlen(bufs[field]);
            int cx = fx + (n > (size_t)(fw - 1) ? fw - 1 : (int)n);
            ad_screen_flush(&e->screen, cx, row0 + 2 + field);
        }
        k = popup_key(e, local);
        switch (k.kind) {
            case AD_KEY_HANGUP: ad_screen_full_redraw(&e->screen); return -1;
            case AD_KEY_ESCAPE: ad_screen_full_redraw(&e->screen); return 0;
            case AD_KEY_ENTER:  ad_screen_full_redraw(&e->screen); return 1;
            case AD_KEY_TAB: case AD_KEY_DOWN: field = (field + 1) % nfields; break;
            case AD_KEY_UP: field = (field + nfields - 1) % nfields; break;
            case AD_KEY_BACKSPACE: {
                size_t n = strlen(bufs[field]);
                if (n) bufs[field][n - 1] = 0;
                break;
            }
            case AD_KEY_CHAR: {
                size_t n = strlen(bufs[field]);
                if (filter((unsigned char)k.ch) && (int)n < maxlens[field]) {
                    bufs[field][n] = k.ch;
                    bufs[field][n + 1] = 0;
                }
                break;
            }
            case AD_KEY_MOUSE:
                if (k.mbutton == 2) { ad_screen_full_redraw(&e->screen); return 0; }
                if (k.mbutton == 0 && k.my >= row0 + 2 && k.my < row0 + 2 + nfields) field = k.my - row0 - 2;
                break;
            default: break;
        }
    }
}

/* One-line question on the status bar. Returns 'Y', 'N', 0 (Esc), -1 hangup. */
static int ask_ync(AdEditor *e, int local, const char *question) {
    for (;;) {
        AdKey k;
        set_message(e, "%s", question);
        paint_all(e);
        ad_screen_flush(&e->screen, 17 + (int)strlen(question) + 1, e->screen.h - 1);
        k = popup_key(e, local);
        e->message[0] = 0;
        if (k.kind == AD_KEY_HANGUP) return -1;
        if (k.kind == AD_KEY_ESCAPE) return 0;
        if (k.kind == AD_KEY_CHAR && (k.ch == 'y' || k.ch == 'Y')) return 'Y';
        if (k.kind == AD_KEY_CHAR && (k.ch == 'n' || k.ch == 'N')) return 'N';
    }
}

/* The file browser. The sysop walks the real filesystem (sub-folders,
   ".." to go up); a caller only ever sees the files in their own folder
   and can only type a plain file name. save_mode adds a name field.
   Returns 1 with the chosen path in out, 0 cancel, -1 hangup. */
static int browser(AdEditor *e, int local, int save_mode, char *out, size_t outsz) {
    const AdDoor *d = e->door;
    int sysop = d->sysop;
    /* name holds any listed file's full name (up to 255) -- truncating
       it would save to a different file than the one picked */
    char dir[AD_PATH_MAX], name[256];
    AdDirList list = { NULL, 0 };
    int sel = 0, scroll = 0, have_up = 0, list_focus = 1, reload = 1;

    snprintf(dir, sizeof(dir), "%s", sysop ? (e->browse_dir[0] ? e->browse_dir : d->data_dir) : d->user_dir);
    snprintf(name, sizeof(name), "%.255s", e->path[0] ? base_name(e->path) : "");
    if (save_mode && !name[0]) list_focus = 0;

    for (;;) {
        AdKey k;
        int width = e->screen.w - 8 < 72 ? e->screen.w - 8 : 72;
        int height = e->view_h - 2 < 22 ? e->view_h - 2 : 22;
        int col0 = (e->screen.w - width) / 2, row0 = (e->view_h - height) / 2;
        int list_rows = height - (save_mode ? 6 : 5), total, i;
        char line[128];

        if (reload) {
            ad_free_dir(&list);
            if (!ad_list_dir(dir, sysop, &list) && sysop) {
                /* unreadable folder: fall back to the data folder */
                snprintf(dir, sizeof(dir), "%s", d->data_dir);
                ad_list_dir(dir, sysop, &list);
            }
            have_up = sysop && strcmp(dir, "/") != 0 && !(strlen(dir) == 3 && dir[1] == ':');
            sel = scroll = 0;
            reload = 0;
        }
        total = list.n + have_up;
        if (row0 < 0) row0 = 0;
        if (list_rows < 3) list_rows = 3;
        if (sel >= total) sel = total - 1;
        if (sel < 0) sel = 0;
        if (sel < scroll) scroll = sel;
        if (sel >= scroll + list_rows) scroll = sel - list_rows + 1;

        paint_all(e);
        frame(e, col0, row0, width, height, save_mode ? "Save drawing" : "Open drawing");
        if (sysop) {
            size_t n = strlen(dir);
            snprintf(line, sizeof(line), "%.*s", width - 4, n > (size_t)(width - 4) ? dir + n - (width - 4) : dir);
        } else {
            snprintf(line, sizeof(line), "Your drawings");
        }
        ad_screen_puts(&e->screen, col0 + 2, row0 + 1, line, AD_ATTR(11, 1));
        for (i = 0; i < list_rows; i++) {
            int idx = scroll + i, y = row0 + 2 + i;
            char item[128];
            unsigned char a;
            if (idx >= total) {
                snprintf(item, sizeof(item), "%-*s", width - 4, (i == 0 && total == 0) ? "  (no drawings here yet)" : "");
                ad_screen_puts(&e->screen, col0 + 2, y, item, AD_ATTR(7, 1));
                continue;
            }
            if (have_up && idx == 0) {
                snprintf(item, sizeof(item), " %-*s", width - 5, "\x18 .. (up a folder)");
            } else {
                const AdDirEntry *de = &list.e[idx - have_up];
                if (de->is_dir) snprintf(item, sizeof(item), " \xFE %-*.*s", width - 7, width - 7, de->name);
                else snprintf(item, sizeof(item), "   %-*.*s%7ldK", width - 15, width - 15, de->name, (de->size + 1023) / 1024);
            }
            a = (idx == sel) ? (list_focus ? AD_ATTR(0, 3) : AD_ATTR(0, 7)) : AD_ATTR(15, 1);
            ad_screen_puts(&e->screen, col0 + 2, y, item, a);
        }
        if (save_mode) {
            /* the field runs from col0+13 to just inside the frame; a
               long name shows its tail */
            int fw = width - 16;
            size_t nl = strlen(name);
            snprintf(line, sizeof(line), "%-*.*s", fw, fw, nl > (size_t)(fw - 1) ? name + nl - (fw - 1) : name);
            ad_screen_puts(&e->screen, col0 + 2, row0 + height - 3, "File name:", POPUP_ATTR);
            ad_screen_puts(&e->screen, col0 + 13, row0 + height - 3, line, list_focus ? AD_ATTR(15, 0) : AD_ATTR(0, 7));
        }
        ad_screen_puts(&e->screen, col0 + 2, row0 + height - 2,
                       save_mode ? "Type a name, Enter=save  \x18\x19=pick  Esc=cancel"
                                 : "\x18\x19 PgUp PgDn=pick  Enter=open  Esc=cancel", AD_ATTR(7, 1));
        if (save_mode && !list_focus)
            ad_screen_flush(&e->screen, col0 + 13 + ((int)strlen(name) < width - 17 ? (int)strlen(name) : width - 17),
                            row0 + height - 3);
        else
            ad_screen_flush(&e->screen, col0 + 3, row0 + 2 + (sel - scroll));

        k = popup_key(e, local);
        switch (k.kind) {
            case AD_KEY_HANGUP:
                ad_free_dir(&list);
                ad_screen_full_redraw(&e->screen);
                return -1;
            case AD_KEY_ESCAPE:
                ad_free_dir(&list);
                ad_screen_full_redraw(&e->screen);
                return 0;
            case AD_KEY_UP:   sel--; list_focus = 1; break;
            case AD_KEY_DOWN: sel++; list_focus = 1; break;
            case AD_KEY_PGUP: sel -= list_rows; list_focus = 1; break;
            case AD_KEY_PGDN: sel += list_rows; list_focus = 1; break;
            case AD_KEY_HOME: sel = 0; list_focus = 1; break;
            case AD_KEY_END:  sel = total - 1; list_focus = 1; break;
            case AD_KEY_TAB:  if (save_mode) list_focus = !list_focus; break;
            case AD_KEY_BACKSPACE:
                if (save_mode) { size_t n = strlen(name); if (n) name[n - 1] = 0; list_focus = 0; }
                break;
            case AD_KEY_CHAR:
                if (save_mode && (sysop ? filter_sysop_name((unsigned char)k.ch) : filter_caller_name((unsigned char)k.ch))) {
                    size_t n = strlen(name);
                    if (n < AD_NAME_MAX) { name[n] = k.ch; name[n + 1] = 0; }
                    list_focus = 0;
                }
                break;
            case AD_KEY_MOUSE:
                if (k.mbutton == 2) { ad_free_dir(&list); ad_screen_full_redraw(&e->screen); return 0; }
                if (k.mwheel) { sel += 3 * k.mwheel; break; }
                if (k.mbutton == 0 && k.my >= row0 + 2 && k.my < row0 + 2 + list_rows) {
                    int idx = scroll + (k.my - row0 - 2);
                    if (idx < total) {
                        int again = (idx == sel && list_focus);
                        sel = idx;
                        list_focus = 1;
                        if (!again && !(have_up && idx == 0) && !list.e[idx - have_up].is_dir) {
                            if (save_mode) snprintf(name, sizeof(name), "%s", list.e[idx - have_up].name);
                            break;
                        }
                        k.kind = AD_KEY_ENTER;  /* folders open on one click, files on the second */
                    }
                }
                if (k.kind != AD_KEY_ENTER) break;
                /* FALLTHROUGH */
            case AD_KEY_ENTER:
                if (list_focus && total > 0) {
                    if (have_up && sel == 0) {
                        char up[AD_PATH_MAX];
                        ad_parent_dir(dir, up, sizeof(up));
                        snprintf(dir, sizeof(dir), "%s", up);
                        reload = 1;
                        break;
                    }
                    if (list.e[sel - have_up].is_dir) {
                        char sub[AD_PATH_MAX];
                        ad_path_join(dir, list.e[sel - have_up].name, sub, sizeof(sub));
                        snprintf(dir, sizeof(dir), "%s", sub);
                        reload = 1;
                        break;
                    }
                    if (!save_mode) {
                        ad_path_join(dir, list.e[sel - have_up].name, out, outsz);
                        goto chosen;
                    }
                    snprintf(name, sizeof(name), "%s", list.e[sel - have_up].name);
                }
                if (save_mode) {
                    char clean[256 + 8];
                    if (sysop) {
                        /* the sysop may use any plain name; .ans if there's no extension */
                        size_t n;
                        snprintf(clean, sizeof(clean), "%s", name);
                        n = strlen(clean);
                        while (n && clean[n - 1] == ' ') clean[--n] = 0;
                        if (!n) break;
                        if (!strchr(clean, '.') && n + 4 < sizeof(clean)) strcat(clean, ".ans");
                    } else if (!ad_safe_filename(name, clean, sizeof(clean))) {
                        break;
                    }
                    ad_path_join(dir, clean, out, outsz);
                    goto chosen;
                }
                break;
            default: break;
        }
    }
chosen:
    if (sysop) snprintf(e->browse_dir, sizeof(e->browse_dir), "%s", dir);
    ad_free_dir(&list);
    ad_screen_full_redraw(&e->screen);
    return 1;
}

/* Returns 1 saved, 0 not saved (cancelled / failed), -1 hangup. */
/* TheDraw-style save options (see AdSauce): clear the screen first, and
   a display speed. Returns 1 save, 0 cancel, -1 hangup. */
static int save_options(AdEditor *e, int local) {
    int clear = e->sauce.clear_screen, speed = e->sauce.speed, row = 0;
    int width = 58, height = 9;
    if (speed < 0 || speed >= AD_SPEED_COUNT) speed = 0;
    for (;;) {
        AdKey k;
        char buf[80], sp[24];
        int col0 = (e->screen.w - width) / 2, row0 = (e->view_h - height) / 2;
        if (row0 < 0) row0 = 0;
        if (speed) snprintf(sp, sizeof(sp), "%ld bps", AD_SPEEDS[speed]);
        else snprintf(sp, sizeof(sp), "full speed");
        paint_all(e);
        frame(e, col0, row0, width, height, "Save options");
        snprintf(buf, sizeof(buf), " C  Clear the screen first      %-3s ", clear ? "Yes" : "No");
        ad_screen_puts(&e->screen, col0 + 2, row0 + 2, buf, row == 0 ? AD_ATTR(0, 3) : POPUP_ATTR);
        snprintf(buf, sizeof(buf), " S  Display speed      \x11 %-12s \x10 ", sp);
        ad_screen_puts(&e->screen, col0 + 2, row0 + 3, buf, row == 1 ? AD_ATTR(0, 3) : POPUP_ATTR);
        if (row != 0) ad_screen_put(&e->screen, col0 + 3, row0 + 2, 'C', POPUP_KEY);
        if (row != 1) ad_screen_put(&e->screen, col0 + 3, row0 + 3, 'S', POPUP_KEY);
        ad_screen_puts(&e->screen, col0 + 2, row0 + 5, "A speed draws it like a modem would. SyncTERM and most", AD_ATTR(7, 1));
        ad_screen_puts(&e->screen, col0 + 2, row0 + 6, "BBS terminals honor it; the rest just show it at once.", AD_ATTR(7, 1));
        ad_screen_puts(&e->screen, col0 + 2, row0 + height - 1, " Enter=save  Esc=cancel ", POPUP_KEY);
        ad_screen_flush(&e->screen, col0 + 3, row0 + 2 + row);
        k = popup_key(e, local);
        switch (k.kind) {
            case AD_KEY_HANGUP: ad_screen_full_redraw(&e->screen); return -1;
            case AD_KEY_ESCAPE: ad_screen_full_redraw(&e->screen); return 0;
            case AD_KEY_ENTER:
                e->sauce.clear_screen = clear;
                e->sauce.speed = speed;
                ad_screen_full_redraw(&e->screen);
                return 1;
            case AD_KEY_UP: case AD_KEY_DOWN: case AD_KEY_TAB: row = !row; break;
            case AD_KEY_LEFT: if (row) speed = (speed + AD_SPEED_COUNT - 1) % AD_SPEED_COUNT; else clear = !clear; break;
            case AD_KEY_RIGHT: if (row) speed = (speed + 1) % AD_SPEED_COUNT; else clear = !clear; break;
            case AD_KEY_CHAR:
                if ((k.ch & ~0x20) == 'C' || (k.ch == ' ' && row == 0)) { clear = !clear; row = 0; }
                else if ((k.ch & ~0x20) == 'S' || (k.ch == ' ' && row == 1)) { speed = (speed + 1) % AD_SPEED_COUNT; row = 1; }
                else if ((k.ch & ~0x20) == 'Y' && row == 0) clear = 1;
                else if ((k.ch & ~0x20) == 'N' && row == 0) clear = 0;
                break;
            case AD_KEY_MOUSE:
                if (k.mbutton == 2) { ad_screen_full_redraw(&e->screen); return 0; }
                if (k.mbutton == 0 && k.my == row0 + 2) { row = 0; clear = !clear; }
                else if (k.mbutton == 0 && k.my == row0 + 3) {
                    row = 1;
                    /* left arrow half steps down, the rest steps up */
                    speed = (speed + (k.mx < col0 + 27 ? AD_SPEED_COUNT - 1 : 1)) % AD_SPEED_COUNT;
                }
                break;
            default: break;
        }
    }
}

/* SAUCE defaults: the artist, and the file name as the title */
static void sauce_defaults(AdEditor *e, const char *path) {
    if (!e->sauce.author[0])
        snprintf(e->sauce.author, sizeof(e->sauce.author), "%.20s", e->door->user_name);
    if (!e->sauce.title[0]) {
        char t[64];
        char *dot;
        snprintf(t, sizeof(t), "%.63s", base_name(path));
        dot = strrchr(t, '.');
        if (dot) *dot = 0;
        snprintf(e->sauce.title, sizeof(e->sauce.title), "%.35s", t);
    }
}

static int do_save(AdEditor *e, int local, int save_as) {
    char path[AD_PATH_MAX], err[160];
    if (save_as || !e->path[0]) {
        int r = browser(e, local, 1, path, sizeof(path));
        if (r <= 0) return r;
        if (strcmp(path, e->path) != 0 && ad_file_exists(path)) {
            char q[120];
            snprintf(q, sizeof(q), "%.60s exists. Replace it? (Y/N)", base_name(path));
            r = ask_ync(e, local, q);
            if (r < 0) return -1;
            if (r != 'Y') return 0;
        }
        /* as TheDraw did: how should it display? (kept with the drawing,
           so a plain Save doesn't ask again) */
        r = save_options(e, local);
        if (r <= 0) return r;
    } else {
        snprintf(path, sizeof(path), "%s", e->path);
    }
    sauce_defaults(e, path);
    if (!ad_ans_save(path, &e->canvas, &e->sauce, err, sizeof(err))) {
        set_message(e, "Couldn't save: %.50s", err);
        return 0;
    }
    snprintf(e->path, sizeof(e->path), "%s", path);
    e->dirty = 0;
    set_message(e, "Saved %.50s", base_name(path));
    return 1;
}

/* Before throwing the drawing away: offer to save. Returns 1 go ahead,
   0 stay, -1 hangup. */
static int ok_to_discard(AdEditor *e, int local, const char *verb) {
    char q[100];
    int r;
    if (!e->dirty) return 1;
    snprintf(q, sizeof(q), "Save your drawing before you %s? (Y/N, Esc=cancel)", verb);
    r = ask_ync(e, local, q);
    if (r < 0) return -1;
    if (r == 0) return 0;
    if (r == 'N') return 1;
    r = do_save(e, local, 0);
    return r < 0 ? -1 : r;
}

/* Swaps a freshly prepared canvas in: new layout, cursor home, no undo. */
static void adopt_canvas(AdEditor *e, AdCanvas *c) {
    ad_canvas_free(&e->canvas);
    e->canvas = *c;
    ad_undo_clear(&e->undo);
    e->cx = e->cy = e->top = e->left = e->half = 0;
    e->anchored = e->pasting = 0;
    ad_screen_set_ice(&e->screen, e->canvas.ice);
    layout(e, e->screen.w, e->screen.h);
    clamp_cursor(e);
}

static int do_open(AdEditor *e, int local) {
    char path[AD_PATH_MAX], err[160];
    AdCanvas c;
    AdSauce meta;
    int r = ok_to_discard(e, local, "open another");
    if (r <= 0) return r < 0 ? 0 : 1;
    r = browser(e, local, 0, path, sizeof(path));
    if (r < 0) return 0;
    if (r == 0) return 1;
    /* load into a spare canvas: a bad file never costs the current one */
    if (!ad_canvas_init(&c, AD_CANVAS_W, AD_CANVAS_MIN_H)) {
        set_message(e, "Out of memory");
        return 1;
    }
    if (!ad_ans_load(path, &c, &meta, err, sizeof(err))) {
        ad_canvas_free(&c);
        set_message(e, "Couldn't open it: %.50s", err);
        return 1;
    }
    adopt_canvas(e, &c);
    e->sauce = meta;
    snprintf(e->path, sizeof(e->path), "%s", path);
    e->dirty = 0;
    set_message(e, "Opened %.40s (%dx%d%s)", base_name(path), e->canvas.w, e->canvas.h,
                e->canvas.ice ? ", iCE" : "");
    return 1;
}

/* On a wide screen a new drawing starts with a choice of canvas: the
   classic 80x25, the space beside the toolbox, or the whole screen.
   Returns 0 on hangup. (Skipped when the sysop set --width, or when the
   screen has no room to spare.) */
static int canvas_choice(AdEditor *e, int local) {
    int fit_w = e->screen.w - (SIDEBAR_MIN + 1), full_w = e->screen.w, h = e->screen.h;
    int width = 62, height = 9, sel = 0, n, i;
    const char *labels[3];
    int ws[3], hs[3];
    char l0[80], l1[80], l2[80];
    if (e->door->canvas_w != AD_CANVAS_W || fit_w < AD_MIN_COLS) return 1;
    if (full_w > AD_CANVAS_MAX_W) full_w = AD_CANVAS_MAX_W;
    if (fit_w > AD_CANVAS_MAX_W) fit_w = AD_CANVAS_MAX_W;
    snprintf(l0, sizeof(l0), "A  %3dx%-3d  Classic -- every caller's screen shows it", 80, 25);
    snprintf(l1, sizeof(l1), "B  %3dx%-3d  Fill the space beside the toolbox", fit_w, h);
    snprintf(l2, sizeof(l2), "C  %3dx%-3d  Your whole screen (the toolbox tucks away)", full_w, h);
    labels[0] = l0; labels[1] = l1; labels[2] = l2;
    ws[0] = 80; hs[0] = 25; ws[1] = fit_w; hs[1] = h; ws[2] = full_w; hs[2] = h;
    n = fit_w == full_w ? 2 : 3;
    for (;;) {
        AdKey k;
        int col0 = (e->screen.w - width) / 2, row0 = (e->view_h - height) / 2, pick = -1;
        paint_all(e);
        frame(e, col0, row0, width, height, "New drawing: canvas size");
        for (i = 0; i < n; i++) {
            char line[80];
            snprintf(line, sizeof(line), " %-*s", width - 5, labels[i]);
            ad_screen_puts(&e->screen, col0 + 2, row0 + 2 + i, line, i == sel ? AD_ATTR(0, 3) : POPUP_ATTR);
            if (i != sel) ad_screen_put(&e->screen, col0 + 3, row0 + 2 + i, (unsigned char)('A' + i), POPUP_KEY);
        }
        ad_screen_puts(&e->screen, col0 + 2, row0 + 6, "Change it any time: Esc, C Canvas size.", AD_ATTR(7, 1));
        ad_screen_puts(&e->screen, col0 + 2, row0 + height - 1, " Enter=OK  Esc=classic ", POPUP_KEY);
        ad_screen_flush(&e->screen, col0 + 3, row0 + 2 + sel);
        k = popup_key(e, local);
        if (k.kind == AD_KEY_HANGUP) { ad_screen_full_redraw(&e->screen); return 0; }
        if (k.kind == AD_KEY_ESCAPE) pick = 0;
        else if (k.kind == AD_KEY_UP) sel = (sel + n - 1) % n;
        else if (k.kind == AD_KEY_DOWN || k.kind == AD_KEY_TAB) sel = (sel + 1) % n;
        else if (k.kind == AD_KEY_ENTER) pick = sel;
        else if (k.kind == AD_KEY_CHAR && (k.ch & ~0x20) >= 'A' && (k.ch & ~0x20) < 'A' + n) pick = (k.ch & ~0x20) - 'A';
        else if (k.kind == AD_KEY_MOUSE && k.mbutton == 0 && k.my >= row0 + 2 && k.my < row0 + 2 + n) pick = k.my - row0 - 2;
        else if (k.kind == AD_KEY_RESIZE) return 1;
        if (pick < 0) continue;
        ad_screen_full_redraw(&e->screen);
        if (pick == 0) return 1;  /* the classic growing 80x25 */
        if (!ad_canvas_resize(&e->canvas, ws[pick], hs[pick])) {
            set_message(e, "Not enough memory for that size");
            return 1;
        }
        ad_undo_clear(&e->undo);
        if (pick == 2) e->sb_hidden = 1;
        layout(e, e->screen.w, e->screen.h);
        clamp_cursor(e);
        set_message(e, pick == 2 ? "Canvas %dx%d -- ^W shows the toolbox" : "Canvas %dx%d", ws[pick], hs[pick]);
        return 1;
    }
}

static int do_new(AdEditor *e, int local) {
    AdCanvas c;
    int r = ok_to_discard(e, local, "start a new one");
    if (r <= 0) return r < 0 ? 0 : 1;
    if (!ad_canvas_init(&c, e->door->canvas_w, AD_CANVAS_MIN_H)) {
        set_message(e, "Out of memory");
        return 1;
    }
    adopt_canvas(e, &c);
    memset(&e->sauce, 0, sizeof(e->sauce));
    e->path[0] = 0;
    e->dirty = 0;
    set_message(e, "New drawing");
    return canvas_choice(e, local);
}

static int sauce_dialog(AdEditor *e, int local) {
    static const char *const LABELS[] = { "Title", "Author", "Group" };
    static const int MAX[] = { AD_SAUCE_TITLE_LEN, AD_SAUCE_AUTHOR_LEN, AD_SAUCE_GROUP_LEN };
    char t[AD_SAUCE_TITLE_LEN + 1], a[AD_SAUCE_AUTHOR_LEN + 1], g[AD_SAUCE_GROUP_LEN + 1];
    char *bufs[3];
    int r;
    snprintf(t, sizeof(t), "%s", e->sauce.title);
    snprintf(a, sizeof(a), "%.20s", e->sauce.author[0] ? e->sauce.author : e->door->user_name);
    snprintf(g, sizeof(g), "%s", e->sauce.group);
    bufs[0] = t; bufs[1] = a; bufs[2] = g;
    r = form_dialog(e, local, "SAUCE info", 3, LABELS, bufs, MAX, filter_printable,
                    "Saved inside the .ans file; viewers show it.");
    if (r < 0) return 0;
    if (r == 1) {
        if (strcmp(t, e->sauce.title) || strcmp(a, e->sauce.author) || strcmp(g, e->sauce.group))
            e->dirty = 1;
        snprintf(e->sauce.title, sizeof(e->sauce.title), "%s", t);
        snprintf(e->sauce.author, sizeof(e->sauce.author), "%s", a);
        snprintf(e->sauce.group, sizeof(e->sauce.group), "%s", g);
    }
    return 1;
}

/* Sends files to the caller's terminal by ZMODEM, with a note first for
   terminals that can't take it. Returns 0 on hangup. */
static int send_to_caller(AdEditor *e, const AdZmFile *files, int n) {
    int rc, done = 0;
    if (e->door->local) {
        set_message(e, "Downloads go to a caller's terminal -- use Save as here");
        return 1;
    }
    set_message(e, "Sending %.30s by ZMODEM... (Esc cancels if nothing happens)", files[0].name);
    paint_all(e);
    flush_at_cursor(e);
    rc = ad_xfer_send(e->door, files, n, &done);
    ad_dout("\x1b[0m");
    ad_screen_full_redraw(&e->screen);
    if (rc == AD_ZM_HANGUP) return 0;
    if (rc == AD_ZM_OK && n == 1) set_message(e, "Downloaded %.40s", files[0].name);
    else set_message(e, "%s", ad_zm_result_text(rc));
    return 1;
}

static int do_download(AdEditor *e) {
    char name[AD_NAME_MAX + 8];
    unsigned char *data = NULL;
    size_t len;
    AdSauce meta;
    AdZmFile f;
    int r;
    if (e->path[0]) snprintf(name, sizeof(name), "%.55s", base_name(e->path));
    else snprintf(name, sizeof(name), "anetdraw.ans");
    /* the file carries the same SAUCE a save would, without touching e->sauce */
    {
        AdSauce keep = e->sauce;
        sauce_defaults(e, name);
        meta = e->sauce;
        e->sauce = keep;
    }
    len = ad_ans_encode(&e->canvas, &meta, &data);
    if (!data) {
        set_message(e, "Out of memory");
        return 1;
    }
    f.name = name;
    f.data = data;
    f.len = (long)len;
    f.mtime = (long)time(NULL);
    r = send_to_caller(e, &f, 1);
    free(data);
    return r;
}

/* ------------------------------------------------------------ gallery */

static int blank_canvas(const AdCanvas *c) {
    int w = 0, h = 0;
    ad_canvas_extent(c, &w, &h);
    return w == 0 || h == 0;
}

/* Publish the current drawing to the shared gallery, under a title the
   artist confirms. Returns 0 on hangup. */
static int do_publish(AdEditor *e, int local) {
    static const char *const LABELS[] = { "Title", "Artist", "Group" };
    static const int MAX[] = { AD_SAUCE_TITLE_LEN, AD_SAUCE_AUTHOR_LEN, AD_SAUCE_GROUP_LEN };
    char t[AD_SAUCE_TITLE_LEN + 1], a[AD_SAUCE_AUTHOR_LEN + 1], g[AD_SAUCE_GROUP_LEN + 1];
    char file[256], err[120];
    char *bufs[3];
    unsigned char *data = NULL;
    size_t len;
    int r;

    if (blank_canvas(&e->canvas)) {
        set_message(e, "Draw something first -- the canvas is empty");
        return 1;
    }
    sauce_defaults(e, e->path[0] ? e->path : "untitled");
    snprintf(t, sizeof(t), "%s", e->sauce.title);
    snprintf(a, sizeof(a), "%s", e->sauce.author);
    snprintf(g, sizeof(g), "%s", e->sauce.group);
    bufs[0] = t; bufs[1] = a; bufs[2] = g;
    r = form_dialog(e, local, "Publish to the gallery", 3, LABELS, bufs, MAX, filter_printable,
                    "Everyone on the BBS can view and download it.");
    if (r < 0) return 0;
    if (r == 0) return 1;
    if (strcmp(t, e->sauce.title) || strcmp(a, e->sauce.author) || strcmp(g, e->sauce.group))
        e->dirty = 1;
    snprintf(e->sauce.title, sizeof(e->sauce.title), "%s", t);
    snprintf(e->sauce.author, sizeof(e->sauce.author), "%s", a);
    snprintf(e->sauce.group, sizeof(e->sauce.group), "%s", g);

    if (!ad_gallery_file_name(e->door->user_key, t, file, sizeof(file)) &&
        !ad_gallery_file_name(e->door->user_key, "untitled", file, sizeof(file))) {
        set_message(e, "Couldn't make a file name for that title");
        return 1;
    }
    {
        char path[AD_PATH_MAX];
        ad_path_join(e->door->gallery_dir, file, path, sizeof(path));
        if (ad_file_exists(path)) {
            char q[120];
            snprintf(q, sizeof(q), "You already published \"%.35s\". Replace it? (Y/N)", t);
            r = ask_ync(e, local, q);
            if (r < 0) return 0;
            if (r != 'Y') return 1;
        }
    }
    len = ad_ans_encode(&e->canvas, &e->sauce, &data);
    if (!data) {
        set_message(e, "Out of memory");
        return 1;
    }
    if (ad_gallery_write(e->door->gallery_dir, file, data, len, err, sizeof(err)))
        set_message(e, "Published \"%.35s\" to the gallery", t);
    else
        set_message(e, "Couldn't publish: %.50s", err);
    free(data);
    return 1;
}

static int may_remove(const AdEditor *e, const AdGalleryItem *it) {
    return e->door->sysop || (it->owner[0] && strcmp(it->owner, e->door->user_key) == 0);
}

static int gallery_download(AdEditor *e, const AdGalleryItem *it) {
    size_t len;
    unsigned char *data = ad_gallery_read(e->door->gallery_dir, it->file, &len);
    AdZmFile f;
    int r;
    if (!data) {
        set_message(e, "Couldn't read that piece");
        return 1;
    }
    f.name = ad_gallery_plain_name(it->file);
    f.data = data;
    f.len = (long)len;
    f.mtime = it->mtime;
    r = send_to_caller(e, &f, 1);
    free(data);
    return r;
}

/* Loads a gallery piece into c (initialized here). 0 on failure. */
static int gallery_load(AdEditor *e, const AdGalleryItem *it, AdCanvas *c, AdSauce *meta) {
    size_t len;
    char err[120];
    unsigned char *data = ad_gallery_read(e->door->gallery_dir, it->file, &len);
    int ok;
    if (!data) return 0;
    if (!ad_canvas_init(c, AD_CANVAS_W, AD_CANVAS_MIN_H)) {
        free(data);
        return 0;
    }
    ok = ad_ans_decode(data, len, c, meta, err, sizeof(err));
    free(data);
    if (!ok) ad_canvas_free(c);
    return ok;
}

/* Swaps a gallery piece in as a new, unsaved drawing. */
static int gallery_open_copy(AdEditor *e, int local, const AdGalleryItem *it) {
    AdCanvas c;
    AdSauce meta;
    int r = ok_to_discard(e, local, "open another");
    if (r <= 0) return r < 0 ? -1 : 0;
    if (!gallery_load(e, it, &c, &meta)) {
        set_message(e, "Couldn't open that piece");
        return 0;
    }
    adopt_canvas(e, &c);
    e->sauce = meta;
    e->path[0] = 0;  /* a copy: Save puts it in the caller's own folder */
    e->dirty = 0;
    set_message(e, "Opened a copy of \"%.30s\" -- Save as to keep it", it->title);
    return 1;
}

enum { VIEW_BACK, VIEW_DOWNLOAD, VIEW_OPEN, VIEW_HANGUP };

/* Full-screen viewer for one piece. */
static int view_piece(AdEditor *e, int local, const AdGalleryItem *it) {
    AdCanvas c;
    AdSauce meta;
    int top = 0, left = 0, result = VIEW_BACK, done = 0;
    int rows = e->screen.h - 1, cols = e->screen.w;
    if (!gallery_load(e, it, &c, &meta)) {
        set_message(e, "Couldn't read that piece");
        return VIEW_BACK;
    }
    ad_screen_set_ice(&e->screen, c.ice);
    while (!done) {
        int x, y, maxtop = c.h - rows, maxleft = c.w - cols;
        char status[200];
        AdKey k;
        if (maxtop < 0) maxtop = 0;
        if (maxleft < 0) maxleft = 0;
        if (top > maxtop) top = maxtop;
        if (top < 0) top = 0;
        if (left > maxleft) left = maxleft;
        if (left < 0) left = 0;
        for (y = 0; y < rows; y++)
            for (x = 0; x < cols; x++) {
                if (y + top < c.h && x + left < c.w) {
                    AdCell cell = ad_canvas_get(&c, x + left, y + top);
                    ad_screen_put(&e->screen, x, y, cell.ch, cell.attr);
                } else {
                    ad_screen_put(&e->screen, x, y, ' ', AD_ATTR(7, 0));
                }
            }
        snprintf(status, sizeof(status), " %.35s by %.20s  %dx%d  %s D=download O=open a copy Esc=back",
                 it->title, it->author[0] ? it->author : "?", c.w, c.h,
                 c.h > rows || c.w > cols ? "\x18\x19 scroll " : "");
        for (x = 0; x < cols - 1; x++) ad_screen_put(&e->screen, x, rows, ' ', STATUS_ATTR);
        status[cols - 1 < (int)sizeof(status) ? cols - 1 : (int)sizeof(status) - 1] = 0;
        ad_screen_puts(&e->screen, 0, rows, status, STATUS_ATTR);
        ad_screen_flush(&e->screen, cols - 2, rows);
        k = popup_key(e, local);
        switch (k.kind) {
            case AD_KEY_HANGUP: result = VIEW_HANGUP; done = 1; break;
            case AD_KEY_ESCAPE: case AD_KEY_ENTER: done = 1; break;
            case AD_KEY_UP: top--; break;
            case AD_KEY_DOWN: top++; break;
            case AD_KEY_LEFT: left -= 8; break;
            case AD_KEY_RIGHT: left += 8; break;
            case AD_KEY_PGUP: top -= rows - 1; break;
            case AD_KEY_PGDN: top += rows - 1; break;
            case AD_KEY_HOME: top = left = 0; break;
            case AD_KEY_END: top = maxtop; break;
            case AD_KEY_RESIZE: rows = e->screen.h - 1; cols = e->screen.w; break;
            case AD_KEY_CHAR:
                if ((k.ch & ~0x20) == 'D') { result = VIEW_DOWNLOAD; done = 1; }
                else if ((k.ch & ~0x20) == 'O') { result = VIEW_OPEN; done = 1; }
                else if (k.ch == ' ') top += rows - 1;
                break;
            case AD_KEY_MOUSE:
                if (k.mwheel) top += 3 * k.mwheel;
                else if (k.mbutton == 2) done = 1;
                break;
            default: break;
        }
    }
    ad_canvas_free(&c);
    ad_screen_set_ice(&e->screen, e->canvas.ice);
    ad_screen_full_redraw(&e->screen);
    return result;
}

static void gallery_date(const char *d, char *out, size_t outsz) {
    if (strlen(d) == 8) snprintf(out, outsz, "%.4s-%.2s-%.2s", d, d + 4, d + 6);
    else snprintf(out, outsz, "%s", "");
}

/* The gallery: everyone's published pieces, newest first. Returns 0 on
   hangup. */
static int gallery_screen(AdEditor *e, int local) {
    AdGalleryItem *items = NULL;
    int n = -1, sel = 0, scroll = 0;
    for (;;) {
        AdKey k;
        int width = e->screen.w - 4 < 78 ? e->screen.w - 4 : 78;
        int height = e->view_h - 1 < 23 ? e->view_h - 1 : 23;
        int col0 = (e->screen.w - width) / 2, row0 = (e->view_h - height) / 2;
        int list_rows = height - 5, i, tw = width - 42;
        char line[160];
        const AdGalleryItem *cur;

        if (n < 0) {
            free(items);
            n = ad_gallery_list(e->door->gallery_dir, &items);
        }
        if (row0 < 0) row0 = 0;
        if (list_rows < 3) list_rows = 3;
        if (tw < 10) tw = 10;
        if (sel >= n) sel = n - 1;
        if (sel < 0) sel = 0;
        if (sel < scroll) scroll = sel;
        if (sel >= scroll + list_rows) scroll = sel - list_rows + 1;
        cur = n > 0 ? &items[sel] : NULL;

        paint_all(e);
        frame(e, col0, row0, width, height, "Gallery");
        snprintf(line, sizeof(line), "%-*s %-20s %-7s %s", tw, "Title", "Artist", "Size", "Date");
        ad_screen_puts(&e->screen, col0 + 3, row0 + 1, line, AD_ATTR(11, 1));
        for (i = 0; i < list_rows; i++) {
            int idx = scroll + i;
            char size[32], date[16];
            if (idx >= n) {
                snprintf(line, sizeof(line), "%-*s", width - 4,
                         (i == 0 && n == 0) ? "  Nothing here yet -- publish a drawing with P from the menu" : "");
                ad_screen_puts(&e->screen, col0 + 2, row0 + 2 + i, line, AD_ATTR(7, 1));
                continue;
            }
            if (items[idx].w > 0) snprintf(size, sizeof(size), "%dx%d", items[idx].w, items[idx].h);
            else snprintf(size, sizeof(size), "%ldK", (items[idx].size + 1023) / 1024);
            gallery_date(items[idx].date, date, sizeof(date));
            snprintf(line, sizeof(line), " %-*.*s %-20.20s %-7.7s %-10.10s", tw, tw, items[idx].title,
                     items[idx].author, size, date);
            snprintf(line + strlen(line), sizeof(line) - strlen(line), "%*s",
                     width - 4 - (int)strlen(line) > 0 ? width - 4 - (int)strlen(line) : 0, "");
            ad_screen_puts(&e->screen, col0 + 2, row0 + 2 + i, line,
                           idx == sel ? AD_ATTR(0, 3) : AD_ATTR(15, 1));
        }
        snprintf(line, sizeof(line), "%d piece%s", n, n == 1 ? "" : "s");
        ad_screen_puts(&e->screen, col0 + width - 3 - (int)strlen(line), row0 + height - 3, line, AD_ATTR(7, 1));
        ad_screen_puts(&e->screen, col0 + 2, row0 + height - 2,
                       cur && may_remove(e, cur) ? "Enter=view  D=download  O=open a copy  R=remove  Esc=back"
                                                 : "Enter=view  D=download  O=open a copy  Esc=back",
                       AD_ATTR(7, 1));
        ad_screen_flush(&e->screen, col0 + 3, row0 + 2 + (sel - scroll));

        k = popup_key(e, local);
        if (k.kind == AD_KEY_MOUSE) {
            if (k.mbutton == 2) k.kind = AD_KEY_ESCAPE;
            else if (k.mwheel) { sel += 3 * k.mwheel; continue; }
            else if (k.mbutton == 0 && k.my >= row0 + 2 && k.my < row0 + 2 + list_rows &&
                     scroll + (k.my - row0 - 2) < n) {
                int idx = scroll + (k.my - row0 - 2);
                if (idx != sel) { sel = idx; continue; }
                k.kind = AD_KEY_ENTER;  /* a second click views it */
            } else {
                continue;
            }
        }
        switch (k.kind) {
            case AD_KEY_HANGUP: free(items); ad_screen_full_redraw(&e->screen); return 0;
            case AD_KEY_ESCAPE: free(items); ad_screen_full_redraw(&e->screen); return 1;
            case AD_KEY_UP: sel--; break;
            case AD_KEY_DOWN: sel++; break;
            case AD_KEY_PGUP: sel -= list_rows; break;
            case AD_KEY_PGDN: sel += list_rows; break;
            case AD_KEY_HOME: sel = 0; break;
            case AD_KEY_END: sel = n - 1; break;
            case AD_KEY_ENTER:
            case AD_KEY_CHAR: {
                int cmd = k.kind == AD_KEY_ENTER ? 'V' : (k.ch & ~0x20);
                if (!cur) break;
                if (cmd == 'V') {
                    int v = view_piece(e, local, cur);
                    if (v == VIEW_HANGUP) { free(items); return 0; }
                    if (v == VIEW_DOWNLOAD) cmd = 'D';
                    else if (v == VIEW_OPEN) cmd = 'O';
                }
                if (cmd == 'D') {
                    if (!gallery_download(e, cur)) { free(items); return 0; }
                    /* the result shows on the status bar under the list */
                    paint_status(e);
                } else if (cmd == 'O') {
                    int r = gallery_open_copy(e, local, cur);
                    if (r < 0) { free(items); return 0; }
                    if (r > 0) { free(items); ad_screen_full_redraw(&e->screen); return 1; }
                } else if (cmd == 'R' && may_remove(e, cur)) {
                    char q[120];
                    int r;
                    snprintf(q, sizeof(q), "Remove \"%.35s\" from the gallery? (Y/N)", cur->title);
                    r = ask_ync(e, local, q);
                    if (r < 0) { free(items); return 0; }
                    if (r == 'Y') {
                        if (ad_gallery_remove(e->door->gallery_dir, cur->file))
                            set_message(e, "Removed \"%.35s\"", cur->title);
                        else
                            set_message(e, "Couldn't remove it");
                        n = -1;  /* list again */
                    }
                }
                break;
            }
            default: break;
        }
    }
}

/* Esc's last stop: the main menu. Returns 0 when the artist quits. */
static int main_menu(AdEditor *e, int local) {
    static const char *const LINES[] = {
        "ANetDRAW",
        "N New drawing     O Open...",
        "S Save            A Save as...",
        "I SAUCE info...   C Canvas size...",
        "B Browse gallery  P Publish to gallery",
        "D Download this drawing (ZMODEM)",
        "K Colors          G Characters",
        "T Tools           H Help",
        "W Toolbox on/off  Q Quit",
        "",
        "  Esc = back to drawing",
    };
    int n = (int)(sizeof(LINES) / sizeof(LINES[0]));
    int width = 42;
    char title[64];
    const char *lines[sizeof(LINES) / sizeof(LINES[0])];
    AdKey k;

    /* the title bar names the file, with * if it has unsaved changes */
    snprintf(title, sizeof(title), "%.28s%s", e->path[0] ? base_name(e->path) : "Untitled",
             e->dirty ? " *" : "");
    memcpy(lines, LINES, sizeof(lines));
    lines[0] = title;
    paint_all(e);
    popup(e, lines, n, width);
    flush_at_cursor(e);
    k = popup_key(e, local);
    ad_screen_full_redraw(&e->screen);
    if (k.kind == AD_KEY_HANGUP) return 0;
    if (k.kind == AD_KEY_MOUSE && k.mbutton == 0) {
        int row = k.my - popup_row0(e, n);
        if (row >= 1 && row < n && LINES[row][0] != ' ' && LINES[row][0]) {
            const char *second = strchr(LINES[row] + 1, ' ');
            k.kind = AD_KEY_CHAR;
            k.ch = LINES[row][0];
            /* two-item lines: the right half picks the second item */
            while (second && *second == ' ') second++;
            if (second && *second && second[1] == ' ' && k.mx >= (e->screen.w - width) / 2 + 20)
                k.ch = *second;
        }
    }
    if (k.kind != AD_KEY_CHAR) return 1;
    return menu_command(e, local, k.ch);
}

/* One main-menu command by its letter (from the menu or a toolbox
   button). Returns 0 when the session should end. */
static int menu_command(AdEditor *e, int local, char c) {
    int r;
    switch (c & ~0x20) {
        case 'N': return do_new(e, local);
        case 'O': return do_open(e, local);
        case 'S': return do_save(e, local, 0) >= 0;
        case 'A': return do_save(e, local, 1) >= 0;
        case 'I': return sauce_dialog(e, local);
        case 'C': return canvas_dialog(e, local);
        case 'W': toggle_sidebar(e); return 1;
        case 'D': return do_download(e);
        case 'B': return gallery_screen(e, local);
        case 'P': return do_publish(e, local);
        case 'K': return color_picker(e, local);
        case 'G': return char_picker(e, local);
        case 'T': tool_menu(e, local); return 1;
        case 'H': return help_screen(e, local);
        case 'Q':
            if (e->dirty) {
                r = ok_to_discard(e, local, "quit");
                if (r < 0) return 0;
                return r == 0;  /* staying: 1 keeps the session going */
            }
            return !confirm_quit(e, local);
        default:  return 1;
    }
}

/* ------------------------------------------------------------ keys */

/* F-key / ^A glyph: placed in the Draw tool, and always becomes the brush. */
static void fkey_glyph(AdEditor *e, int idx) {
    unsigned char g = e->fkeys[e->fset][idx];
    e->brush = g;
    if (e->tool == AD_TOOL_DRAW && !e->pasting) place(e, g);
}

static void handle_ctrl(AdEditor *e, char c, int local) {
    switch (c) {
        case 'A':
            e->ctrl_a = 1;
            set_message(e, "^A: press 1-0 for the F1-F10 glyph");
            break;
        case 'F':
            e->fg = (e->fg + 1) % 16;
            break;
        case 'B':
            e->bg = (e->bg + 1) % (e->canvas.ice ? 16 : 8);
            break;
        case 'E':
            e->canvas.ice = !e->canvas.ice;
            e->dirty = 1;  /* saved into the file's SAUCE flags */
            if (!e->canvas.ice) e->bg &= 7;
            ad_screen_set_ice(&e->screen, e->canvas.ice);
            set_message(e, e->canvas.ice ? "iCE colors ON (16 backgrounds)"
                                         : "iCE colors OFF (bright bg = blink)");
            break;
        case 'N':
            e->fset = (e->fset + 1) % AD_FKEY_NUM_SETS;
            break;
        case 'P':
            e->fset = (e->fset + AD_FKEY_NUM_SETS - 1) % AD_FKEY_NUM_SETS;
            break;
        case 'U': {
            AdCell c2 = ad_canvas_get(&e->canvas, e->cx, e->cy);
            e->fg = AD_ATTR_FG(c2.attr);
            e->bg = AD_ATTR_BG(c2.attr);
            if (!e->canvas.ice) e->bg &= 7;
            set_message(e, "Picked up colors F%02d/B%02d", e->fg, e->bg);
            break;
        }
        case 'R': {
            static const char *const NAMES[4] = { "off", "left/right", "top/bottom", "both ways" };
            e->mirror = (e->mirror + 1) & 3;
            set_message(e, "Mirror %s", NAMES[e->mirror]);
            break;
        }
        case 'T':
            tool_menu(e, local);
            break;
        case 'V':
            start_paste(e);
            break;
        case 'W':
            toggle_sidebar(e);
            break;
        case 'D':
            rows_cols_menu(e, local);
            break;
        case 'L':
            /* redraw -- and re-check the terminal size, e.g. after
               SyncTERM switched to 132x37 mid-session */
            apply_resize(e);
            break;
        default:
            break;
    }
}

static void undo_redo(AdEditor *e, int redo) {
    int ok = redo ? ad_undo_redo(&e->undo, &e->canvas) : ad_undo_undo(&e->undo, &e->canvas);
    if (ok) e->dirty = 1;
    if (!ok) {
        set_message(e, redo ? "Nothing to redo" : "Nothing to undo");
        return;
    }
    set_message(e, "%s -- %d undo, %d redo left", redo ? "Redone" : "Undone",
                e->undo.applied, e->undo.nacts - e->undo.applied);
}

static void move_cursor(AdEditor *e, AdKeyKind k) {
    int px = pixel_space(e);
    switch (k) {
        case AD_KEY_UP:
            if (px && e->half == 1) e->half = 0;
            else { e->cy--; if (px) e->half = 1; }
            break;
        case AD_KEY_DOWN:
            if (px && e->half == 0) e->half = 1;
            else { e->cy++; if (px) e->half = 0; }
            break;
        case AD_KEY_LEFT:  e->cx--; break;
        case AD_KEY_RIGHT: e->cx++; break;
        default: break;
    }
}

/* ------------------------------------------------------------ mouse */

static int is_shape_tool(const AdEditor *e) {
    return e->tool == AD_TOOL_LINE || e->tool == AD_TOOL_BOX || e->tool == AD_TOOL_ELLIPSE;
}

/* Left button released: finish whatever the drag was doing. */
static void mouse_up(AdEditor *e) {
    if (e->mouse_drag == 1 && e->anchored && is_shape_tool(e)) {
        AdSurface s = surface(e, 1);
        draw_shape(e, &s);
        e->anchored = 0;
    }
    if (e->mouse_drag == 1 && e->anchored && e->tool == AD_TOOL_SELECT) {
        int x0, y0, x1, y1;
        sel_rect(e, &x0, &y0, &x1, &y1);
        set_message(e, "Marked %dx%d: C copy  X cut  D delete  F fill  R recolor  M move  (right-click: menu)",
                    x1 - x0 + 1, y1 - y0 + 1);
    }
    e->mouse_drag = 0;
}

static void mouse_press(AdEditor *e, int button) {
    AdSurface s = surface(e, 1);
    e->mlx = e->cx;
    e->mly = e->cy;
    e->mlpy = cursor_py(e);

    if (button == 2) {
        if (e->tool == AD_TOOL_SELECT && !e->pasting) {
            e->block_menu = 1;  /* opened by handle_mouse() once the press is done */
        } else if (e->tool == AD_TOOL_SHADE && !e->pasting) {
            ad_shade_step(&s, e->cx, e->cy, e->fg, -1);
            e->mouse_drag = 2;
        } else {
            /* eyedropper: colors, and the glyph as the brush */
            AdCell c = ad_canvas_get(&e->canvas, e->cx, e->cy);
            e->fg = AD_ATTR_FG(c.attr);
            e->bg = AD_ATTR_BG(c.attr);
            if (!e->canvas.ice) e->bg &= 7;
            if (c.ch != ' ' && c.ch != 0) e->brush = c.ch;
            set_message(e, "Picked up F%02d/B%02d and the glyph", e->fg, e->bg);
        }
        return;
    }
    if (button != 0) return;

    if (e->pasting) {
        ad_clip_paste(&s, &e->clip, e->cx, e->cy, e->paste_transparent);
        return;
    }
    switch (e->tool) {
        case AD_TOOL_DRAW:
            ad_surf_put(&s, e->cx, e->cy, e->brush, cur_attr(e));
            e->mouse_drag = 1;
            break;
        case AD_TOOL_PIXEL:
            ad_surf_pixel(&s, e->cx, cursor_py(e), e->fg);
            e->mouse_drag = 1;
            break;
        case AD_TOOL_SHADE:
            ad_shade_step(&s, e->cx, e->cy, e->fg, +1);
            e->mouse_drag = 1;
            break;
        case AD_TOOL_LINE:
        case AD_TOOL_BOX:
        case AD_TOOL_ELLIPSE:
        case AD_TOOL_SELECT:
            e->anchored = 1;
            e->ax = e->cx;
            e->ay = e->cy;
            e->apy = cursor_py(e);
            e->mouse_drag = 1;
            break;
        case AD_TOOL_FILL:
            if (ad_flood_fill(&s, e->cx, e->cy, e->brush, cur_attr(e), e->opt[AD_TOOL_FILL]) == 0)
                set_message(e, "Nothing to fill there");
            break;
        case AD_TOOL_COLORIZE:
            ad_surf_recolor(&s, e->cx, e->cy, e->fg, e->bg, e->opt[AD_TOOL_COLORIZE]);
            e->mouse_drag = 1;
            break;
        default:
            break;
    }
}

/* Button held and the pointer moved to a new cell (cursor already
   there). Strokes are joined with a line so fast drags leave no gaps. */
static void mouse_drag_to(AdEditor *e) {
    AdSurface s = surface(e, 1);
    if (e->mouse_drag == 1) {
        switch (e->tool) {
            case AD_TOOL_DRAW:
                ad_draw_line(&s, e->mlx, e->mly, e->cx, e->cy, e->brush, cur_attr(e));
                break;
            case AD_TOOL_PIXEL:
                ad_draw_line_pixels(&s, e->mlx, e->mlpy, e->cx, cursor_py(e), e->fg);
                break;
            case AD_TOOL_SHADE:
                ad_shade_step(&s, e->cx, e->cy, e->fg, +1);
                break;
            case AD_TOOL_COLORIZE:
                ad_recolor_line(&s, e->mlx, e->mly, e->cx, e->cy, e->fg, e->bg, e->opt[AD_TOOL_COLORIZE]);
                break;
            default:
                break;  /* shapes / select: the cursor move is the preview */
        }
    } else if (e->mouse_drag == 2) {
        ad_shade_step(&s, e->cx, e->cy, e->fg, -1);
    }
    e->mlx = e->cx;
    e->mly = e->cy;
    e->mlpy = cursor_py(e);
}

/* Clicks on the status bar (layout in paint_status()). */
static int status_click(AdEditor *e, int col, int local) {
    if (col >= 9 && col <= 12) return color_picker(e, local);
    if (col == 14) return char_picker(e, local);
    if ((col >= 16 && col <= 31) || (col >= 63 && col <= 70)) { tool_menu(e, local); return 1; }
    if (col >= 38 && col <= 57) {
        e->brush = e->fkeys[e->fset][(col - 38) / 2];
        set_message(e, "Brush is F%d's glyph", (col - 38) / 2 + 1);
        return 1;
    }
    if (col >= 59 && col <= 61) { e->fset = (e->fset + 1) % AD_FKEY_NUM_SETS; return 1; }
    if (col >= 72 && col <= 78) return help_screen(e, local);
    return 1;
}

/* Returns 0 when the session should end. */
static int handle_mouse(AdEditor *e, AdKey k, int local) {
    if (k.mwheel) {
        int d = 3 * k.mwheel;
        e->top += d;
        e->cy += d;
        if (e->top < 0) { e->cy -= e->top; e->top = 0; }
        return 1;
    }
    if (k.my == e->screen.h - 1) {
        if (!k.mrelease && !k.mdrag && k.mbutton == 0) return status_click(e, k.mx, local);
        if (k.mrelease) mouse_up(e);
        return 1;
    }
    if (e->sb_room && k.mx == e->view_w && k.my >= 0 && k.my < e->view_h) {
        /* the divider column: the collapse / expand tab */
        if (!k.mrelease && !k.mdrag && k.mbutton == 0) toggle_sidebar(e);
        if (k.mrelease) mouse_up(e);
        return 1;
    }
    if (e->sb_w && k.mx >= e->sb_x && k.my >= 0 && k.my < e->view_h) {
        if (!k.mrelease && !k.mdrag && (k.mbutton == 0 || k.mbutton == 2))
            return sidebar_click(e, k.mx, k.my, k.mbutton, local);
        if (k.mrelease) mouse_up(e);
        return 1;
    }
    if (k.my < 0 || k.my >= e->view_h || k.mx < 0 || k.mx >= e->view_w) {
        if (k.mrelease) mouse_up(e);
        return 1;
    }
    k.mx += e->left;  /* screen column -> canvas column */
    if (k.mx >= e->canvas.w && !k.mdrag && !k.mrelease) return 1;  /* a click past the canvas edge */
    if (k.mrelease) {
        mouse_up(e);
        return 1;
    }
    if (k.mdrag) {
        int ny = e->top + k.my;
        if (!e->mouse_drag && !e->pasting && !(e->anchored && is_shape_tool(e))) return 1;
        if (k.mx == e->cx && ny == e->cy) return 1;
        e->cx = k.mx;
        e->cy = ny;
        clamp_cursor(e);
        if (!e->pasting) mouse_drag_to(e);
        return 1;
    }
    /* a press */
    if (e->mouse_drag) mouse_up(e);  /* lost release: finish it now */
    if (k.mbutton == 2 && e->tool == AD_TOOL_SELECT && e->anchored) {
        /* right-click keeps the block: the menu acts on it */
        mouse_press(e, 2);
    } else {
        e->cx = k.mx;
        e->cy = e->top + k.my;
        clamp_cursor(e);
        mouse_press(e, k.mbutton);
    }
    if (e->block_menu) {
        char c;
        e->block_menu = 0;
        c = block_menu(e, local, k.mx - e->left, k.my);
        if (c) block_command(e, local, c);
    }
    return 1;
}

/* Returns 0 when the session should end. */
static int handle_key(AdEditor *e, AdKey k, int local) {
    if (e->ctrl_a) {
        e->ctrl_a = 0;
        if (k.kind == AD_KEY_CHAR && k.ch >= '0' && k.ch <= '9')
            fkey_glyph(e, (k.ch == '0') ? 9 : k.ch - '1');
        return 1;
    }

    switch (k.kind) {
        case AD_KEY_UP: case AD_KEY_DOWN: case AD_KEY_LEFT: case AD_KEY_RIGHT: {
            int ox = e->cx, oy = e->cy, oh = e->half;
            move_cursor(e, k.kind);
            clamp_cursor(e);
            if (pen_on(e) && (e->cx != ox || e->cy != oy || e->half != oh)) {
                /* Draw/Pixel stroke both ends, so the cell an F-key
                   just stepped off of is never skipped; Shade only
                   touches the arrival cell (it would darken twice). */
                if (e->tool != AD_TOOL_SHADE) {
                    int nx = e->cx, ny = e->cy, nh = e->half;
                    e->cx = ox; e->cy = oy; e->half = oh;
                    pen_apply(e);
                    e->cx = nx; e->cy = ny; e->half = nh;
                }
                pen_apply(e);
            }
            break;
        }
        case AD_KEY_HOME: e->cx = 0; break;
        case AD_KEY_END:  e->cx = e->canvas.w - 1; break;
        case AD_KEY_PGUP:
            e->cy -= e->view_h;
            e->top -= e->view_h;
            break;
        case AD_KEY_PGDN:
            e->cy += e->view_h;
            e->top += e->view_h;
            break;
        case AD_KEY_INSERT:
            if (e->tool == AD_TOOL_DRAW) e->insert = !e->insert;
            break;
        case AD_KEY_TAB:
            if (!e->pasting && TOOL_OPT_COUNT[e->tool] > 1) {
                e->opt[e->tool] = (e->opt[e->tool] + 1) % TOOL_OPT_COUNT[e->tool];
                e->anchored = 0;  /* char vs pixel anchors don't mix */
                /* pen down touches the paper right where the cursor is */
                if (pen_on(e) && e->tool != AD_TOOL_SHADE) pen_apply(e);
            }
            break;
        case AD_KEY_ENTER:
            if (e->tool == AD_TOOL_DRAW && !e->pasting) { e->cx = 0; e->cy++; }
            else tool_action(e);
            break;
        case AD_KEY_CHAR:
            if (e->pasting) {
                switch (k.ch) {
                    case ' ': tool_action(e); break;
                    case 'h': case 'H': ad_clip_flip_h(&e->clip); break;
                    case 'v': case 'V': ad_clip_flip_v(&e->clip); break;
                    case 't': case 'T': e->paste_transparent = !e->paste_transparent; break;
                    default: break;
                }
            } else if (e->tool == AD_TOOL_DRAW) {
                place(e, (unsigned char)k.ch);
            } else if (k.ch == ' ') {
                tool_action(e);
            } else if (e->tool == AD_TOOL_SELECT) {
                char c = (char)(k.ch & ~0x20);
                if (e->anchored && strchr("CXMDFRHVSN", c)) select_command(e, c);
                else if (c == 'P' || c == 'A' || c == 'L') block_command(e, local, c);
            } else {
                e->brush = (unsigned char)k.ch;
                set_message(e, "Brush: %c", k.ch);
            }
            break;
        case AD_KEY_BACKSPACE:
            if (e->pasting) break;
            if (e->tool == AD_TOOL_SELECT && e->anchored) select_command(e, 'D');  /* Delete / Bksp */
            else if (e->tool == AD_TOOL_DRAW) backspace(e);
            else if (e->tool == AD_TOOL_SHADE) {
                AdSurface s = surface(e, 1);
                ad_shade_step(&s, e->cx, e->cy, e->fg, -1);
            } else if (e->tool == AD_TOOL_PIXEL) {
                AdSurface s = surface(e, 1);
                ad_surf_pixel(&s, e->cx, cursor_py(e), e->bg);
            }
            break;
        case AD_KEY_FN:
            if (k.fn >= 1 && k.fn <= 10) fkey_glyph(e, k.fn - 1);
            else if (k.fn == 11) handle_ctrl(e, 'P', local);
            else if (k.fn == 12) handle_ctrl(e, 'N', local);
            break;
        case AD_KEY_MOUSE:
            return handle_mouse(e, k, local);
        case AD_KEY_CTRL:
            if (k.ch == 'O') return help_screen(e, local);
            if (k.ch == 'K') return color_picker(e, local);
            if (k.ch == 'G') return char_picker(e, local);
            handle_ctrl(e, k.ch, local);
            break;
        case AD_KEY_ESCAPE:
            if (e->pasting) e->pasting = 0;
            else if (e->anchored) e->anchored = 0;
            else if (e->tool != AD_TOOL_DRAW) set_tool(e, AD_TOOL_DRAW);
            else return main_menu(e, local);
            break;
        default:
            break;
    }
    return 1;
}

void ad_editor_run(AdEditor *e, const AdDoor *door) {
    int skipped = 0;  /* redraws skipped in the current input burst */
    e->door = door;
    /* xterm button-event mouse tracking + SGR coordinates -- terminals
       without mouse support just ignore these; see input.h */
    if (door->mouse) ad_dout("\x1b[?1002h\x1b[?1006h");
    /* steady (non-blinking) underline cursor -- DECSCUSR 4, supported by
       SyncTERM (cterm.adoc) and xterm-family terminals; a steady block
       would hide the glyph under the cursor */
    ad_dout("\x1b[4 q");
    ad_screen_full_redraw(&e->screen);
    set_message(e, "Welcome, %.30s! Type to draw -- ^O for help.", door->user_name);
    if (!canvas_choice(e, door->local)) return;
    if (!e->message[0] || strncmp(e->message, "Canvas", 6) != 0)
        set_message(e, "Welcome, %.30s! Type to draw -- ^O for help.", door->user_name);
    refresh(e);

    for (;;) {
        AdKey k = ad_input_get(door->local);
        int skipped_before = skipped;
        /* a message lasts until the next key or click -- not until the
           release of the click that produced it */
        if (!(k.kind == AD_KEY_MOUSE && k.mrelease)) e->message[0] = 0;
        if (k.kind == AD_KEY_HANGUP) return;
        if (k.kind == AD_KEY_CLOSE) {
            e->close_pending = 1;
        } else if (k.kind == AD_KEY_RESIZE) {
            apply_resize(e);
        } else if (k.kind == AD_KEY_CTRL && (k.ch == 'Z' || k.ch == 'Y')) {
            /* outside any undo action, so undo/redo never truncate
               their own history; a mouse stroke in progress ends first */
            e->ctrl_a = 0;
            if (e->mouse_drag) {
                mouse_up(e);
                ad_undo_end(&e->undo);
            }
            undo_redo(e, k.ch == 'Y');
        } else {
            int keep;
            /* one keypress = one undo step (a fill, a paste, a shape);
               a whole mouse drag stays one step until the button is
               released (ad_undo_begin() is a no-op while it's open) */
            ad_undo_begin(&e->undo);
            keep = handle_key(e, k, door->local);
            if (!e->mouse_drag) {
                int r = ad_undo_end(&e->undo);
                if (r != 0) e->dirty = 1;  /* the drawing changed */
                if (r < 0) set_message(e, "That edit was too big to undo -- undo history cleared");
            }
            if (!keep) return;
            if (e->pending_undo) {
                /* an Undo / Redo button: outside the click's undo action */
                int redo = e->pending_undo == 2;
                e->pending_undo = 0;
                undo_redo(e, redo);
            }
        }
        if (e->close_pending) {
            /* close button: quit, offering to save unsaved work first */
            int r = 1;
            e->close_pending = 0;
            if (e->mouse_drag) {
                mouse_up(e);
                ad_undo_end(&e->undo);
            }
            if (e->dirty) r = ok_to_discard(e, door->local, "quit");
            e->close_pending = 0;  /* a second click while it asked */
            if (r != 0) return;
        }
        clamp_cursor(e);
        /* Mouse drags (and fast typing) arrive in bursts over a real
           network link. When more input is already waiting, apply it
           before drawing: one screen update for the whole burst instead
           of one per event, so the caller's screen catches up in a
           single step. Still draws at least every 32 events, so a long
           drag keeps showing progress. */
        if (skipped_before < 32 && ad_input_pending()) {
            skipped = skipped_before + 1;
            continue;
        }
        skipped = 0;
        refresh(e);
    }
}
