/* The editing session: canvas + cursor + current colors/tool, and the
 * keyboard loop that drives them. */
#ifndef ANETDRAW_EDITOR_H
#define ANETDRAW_EDITOR_H

#include "anetdraw.h"
#include "canvas.h"
#include "render.h"
#include "tools.h"
#include "undo.h"
#include "fkeys.h"
#include "fileio.h"
#include "files.h"
#include "wall.h"

typedef enum {
    AD_TOOL_DRAW = 0,   /* type text / place glyphs; option: pen */
    AD_TOOL_LINE,       /* option: brush glyph / half-block pixels */
    AD_TOOL_BOX,        /* option: AdBoxStyle */
    AD_TOOL_ELLIPSE,    /* option: AdEllipseStyle */
    AD_TOOL_FILL,       /* option: glyph+color / color only */
    AD_TOOL_SHADE,      /* option: pen */
    AD_TOOL_PIXEL,      /* option: pen */
    AD_TOOL_SELECT,     /* copy / cut / delete / fill / move a block */
    AD_TOOL_COLORIZE,   /* recolor what's there; option: fg+bg / fg / bg */
    AD_TOOL_COUNT
} AdTool;

typedef struct {
    AdCanvas canvas;
    AdScreen screen;
    int cx, cy;        /* cursor, canvas coordinates */
    int half;          /* 0 = top, 1 = bottom half-block pixel (pixel tools) */
    int top;           /* first canvas row shown on screen */
    int left;          /* first canvas column shown on screen */
    int view_h;        /* canvas rows visible (screen rows minus status bar) */
    int view_w;        /* canvas columns visible */
    int sb_x, sb_w;    /* sidebar on wide screens (sb_w = 0: none) */
    int sb_room;       /* the screen has room for the sidebar */
    int sb_hidden;     /* artist collapsed it (^W, or the arrow buttons) */
    const AdDoor *door;
    int fg, bg;        /* current drawing colors (bg 0-15 with iCE, else 0-7) */
    int fset;          /* current F-key set, 0-based */
    unsigned char brush;  /* glyph the shape/pen tools draw with */
    int ctrl_a;        /* 1 = ^A pressed, next digit places an F-key glyph */

    AdTool tool;
    int opt[AD_TOOL_COUNT];
    int anchored;      /* shape/select anchor set */
    int ax, ay, apy;   /* anchor: cell x/y, pixel y */
    int mirror;        /* AD_MIRROR_* */
    int insert;        /* Draw tool insert mode */

    int pasting;       /* floating clipboard follows the cursor */
    int paste_transparent;
    AdClipboard clip;

    AdUndo undo;
    /* F-key glyph sets -- start as the PabloDraw/Moebius defaults, and
       the character picker (^G) can put any glyph in any slot */
    unsigned char fkeys[AD_FKEY_NUM_SETS][AD_FKEY_PER_SET];

    /* preview overlay for the visible rows (see tools.h) */
    AdCell *ov;
    unsigned char *ov_mask;

    int mouse_drag;    /* 0 none, 1 left-button stroke/gesture, 2 right (Shade lighten) */
    int mlx, mly, mlpy;  /* last drag position (cell x/y, pixel y) */

    /* the file being edited (empty: not saved yet), its SAUCE info,
       unsaved-changes flag, and where the sysop's browser last was */
    char path[AD_PATH_MAX];
    AdSauce sauce;
    int dirty;
    int close_pending;
    int block_menu;
    int pending_undo;   /* 1 undo / 2 redo, from a toolbox button */
    /* the shared wall (wall.h): on it, the canvas is everyone's */
    int wall;
    AdWall wallst;
    AdCanvas wall_shadow;   /* the canvas as last shared, to find our changes */
    AdWallPeer peers[AD_WALL_MAX_PEERS];
    int npeers;
    long wall_here_at;      /* when we last said where we are (time()) */     /* a right-click asked for the Select menu */  /* the local window's close button was clicked */
    char browse_dir[AD_PATH_MAX];

    /* clickable sidebar regions, rebuilt every paint */
    struct { short x0, x1, y, action, arg; } hits[128];
    int nhits;

    char message[80];  /* one-shot status message, shown until next key */
} AdEditor;

/* Returns 0 on allocation failure. cols/rows: the caller's terminal
   size; canvas_w: width of the new drawing. */
int  ad_editor_init(AdEditor *e, int cols, int rows, int canvas_w);
void ad_editor_free(AdEditor *e);
/* Runs until the artist quits or hangs up. */
void ad_editor_run(AdEditor *e, const AdDoor *door);

#endif /* ANETDRAW_EDITOR_H */
