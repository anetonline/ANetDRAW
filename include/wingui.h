#ifndef ANETDRAW_WINGUI_H
#define ANETDRAW_WINGUI_H

/* Windows local mode (-L): ANetDRAW's own window instead of OpenDoors'
 * console. OpenDoors' local screen is a fixed 80x25 with no mouse; this
 * window is resizable (the editor re-lays out live, like -L in a Linux
 * terminal), goes full screen with Alt+Enter, zooms with Ctrl+Plus /
 * Ctrl+Minus / Ctrl+wheel, and passes the mouse through for drawing.
 *
 * The door's output is the same ANSI it sends callers; wingui.c keeps a
 * small ANSI screen (cursor moves, SGR, clears, iCE/blink mode, mouse
 * mode) and draws it with the IBM VGA 8x16 font (font8x16.c). Input
 * comes back as ready-made AdKeys (keys, mouse, resize, window closed),
 * so input.c hands them straight to the editor. The window runs on its
 * own thread, so it stays responsive while the door is busy.
 *
 * --console on the command line keeps the old OpenDoors console. */

#ifdef _WIN32
#include <stddef.h>
#include "input.h"

/* Opens the window with room for cols x rows cells. Returns 1 and the
   actual size, or 0 if the window couldn't be created. */
int  ad_gui_start(int cols, int rows, int *out_cols, int *out_rows);
int  ad_gui_active(void);
void ad_gui_size(int *cols, int *rows);
void ad_gui_write(const char *buf, size_t len);
/* Next key within ms milliseconds: 1 = got one, 0 = timeout. */
int  ad_gui_get_key(AdKey *k, int ms);
int  ad_gui_pending(void);
void ad_gui_flush_input(void);
#endif

#endif
