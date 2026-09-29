/* Diff-buffered screen renderer. Holds what the caller's terminal is
 * showing right now (prev) and what it should show (curr); a flush
 * only sends the cells that changed. Same idea as ANetCRAFT's
 * renderer, but cells carry a PC attribute byte instead of SGR codes,
 * and the SGR/iCE translation lives here in one place.
 *
 * Terminal facts this is built on (checked against SyncTERM's own
 * cterm.adoc, not assumed):
 * - SyncTERM wraps the instant a glyph lands in the last column (no
 *   VT "pending wrap"), so writing column 80 of the bottom row scrolls
 *   the whole screen. The renderer never writes the bottom-right cell,
 *   and forgets its tracked cursor position after writing any last
 *   column.
 * - SyncTERM treats only 0x00/07/08/09/0A/0D/1B as controls -- every
 *   other byte below 0x20 draws its CP437 glyph. Those seven get a
 *   look-alike substitute on screen (never in the canvas itself).
 * - Bright backgrounds: SyncTERM implements SGR 100-107 by setting the
 *   blink bit, so they only show as bright (not blinking) after
 *   CSI ?33h. xterm-family terminals ignore ?33h and honor 100-107
 *   directly. So iCE mode sends CSI ?33h once and uses 100-107, which
 *   is correct on both; non-iCE mode sends CSI ?33l and SGR 5 (real
 *   blink, which is what bit 7 means without iCE). */
#ifndef ANETDRAW_RENDER_H
#define ANETDRAW_RENDER_H

#include <stddef.h>
#include "canvas.h"

typedef struct {
    int w, h;
    AdCell *curr;
    AdCell *prev;
    int full_redraw;
    int ice;          /* current bright-background mode (see above) */
    int ice_sent;     /* mode last sent to the terminal, -1 = never */
    int cur_col, cur_row;  /* where the last flush parked the cursor, -1 = unknown */
} AdScreen;

int  ad_screen_init(AdScreen *s, int w, int h); /* 0 on alloc failure */
void ad_screen_free(AdScreen *s);
void ad_screen_full_redraw(AdScreen *s);
void ad_screen_set_ice(AdScreen *s, int ice);
/* col/row are 0-based screen coordinates. */
void ad_screen_put(AdScreen *s, int col, int row, unsigned char ch, unsigned char attr);
/* Writes a string starting at col/row, clipped at the right edge. */
void ad_screen_puts(AdScreen *s, int col, int row, const char *str, unsigned char attr);
/* Sends every changed cell, then parks the terminal cursor at
   (cur_col, cur_row) -- 0-based -- or leaves it wherever it ended if
   cur_col < 0. */
void ad_screen_flush(AdScreen *s, int cur_col, int cur_row);
/* Restores the terminal to a sane state before the door exits. */
void ad_screen_restore_terminal(void);

/* Builds the SGR sequence for a PC attribute into buf (>= 32 bytes). */
void ad_sgr(unsigned char attr, int ice, char *buf, size_t bufsz);
/* The byte actually sent to the terminal for a canvas glyph. */
unsigned char ad_display_glyph(unsigned char ch);

#endif /* ANETDRAW_RENDER_H */
