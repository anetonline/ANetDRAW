/* Blocking key input for the editor, on top of OpenDoors'
 * od_get_input(). Unlike ANetCRAFT (a real-time tick loop that must
 * never block), an editor only acts on keypresses, so OpenDoors' own
 * escape-sequence table (arrows, Home/End, PgUp/PgDn, F1-F12 in all
 * the xterm/VT/ANSI-BBS variants) and its 250ms lone-ESC timeout are
 * used as-is instead of reimplemented.
 *
 * GETIN_RAWCTRL, not GETIN_NORMAL: in normal mode OpenDoors turns
 * ^E/^X/^S/^D into arrow keys (WordStar diamond) and ^G/^V into
 * Delete/Insert, which would steal those keys from the editor. 0x7F is
 * mapped to OD_KEY_DELETE even in RAWCTRL mode -- and 0x7F is what
 * PuTTY/xterm send for the Backspace key -- so Delete is treated as
 * Backspace (see ad_input_get()).
 *
 * Mouse: with xterm button-event tracking + SGR coordinates turned on
 * (CSI ?1002h CSI ?1006h -- SyncTERM, xterm, PuTTY and most modern
 * terminals), clicks and drags arrive as CSI < b ; x ; y M/m. OpenDoors
 * doesn't know that sequence, so input.c reassembles it after an ESC
 * and hands it over as AD_KEY_MOUSE. */
#ifndef ANETDRAW_INPUT_H
#define ANETDRAW_INPUT_H

typedef enum {
    AD_KEY_NONE = 0,
    AD_KEY_CHAR,       /* printable 0x20-0x7E, in .ch */
    AD_KEY_CTRL,       /* control letter, .ch = 'A'..'Z' */
    AD_KEY_UP, AD_KEY_DOWN, AD_KEY_LEFT, AD_KEY_RIGHT,
    AD_KEY_HOME, AD_KEY_END, AD_KEY_PGUP, AD_KEY_PGDN,
    AD_KEY_INSERT,
    AD_KEY_ENTER,
    AD_KEY_BACKSPACE,  /* 0x08, 0x7F, or the Delete key */
    AD_KEY_TAB,
    AD_KEY_ESCAPE,
    AD_KEY_FN,         /* F1-F12, .fn = 1..12 */
    AD_KEY_MOUSE,      /* SGR mouse report, see the m* fields */
    AD_KEY_RESIZE,     /* local terminal window changed size */
    AD_KEY_HANGUP,     /* carrier lost -- caller must exit */
    AD_KEY_CLOSE       /* Windows local window's close button: quit, offering to save */
} AdKeyKind;

typedef struct {
    AdKeyKind kind;
    char ch;
    int fn;
    /* AD_KEY_MOUSE: 0-based screen cell, button 0 left / 1 middle /
       2 right / 3 none (motion) / -1 wheel, mwheel -1 up / +1 down */
    int mx, my, mbutton, mdrag, mrelease, mwheel;
} AdKey;

/* Blocks until a key arrives or the caller hangs up. local = 1 skips
   the carrier check (local sessions have no carrier). */
AdKey ad_input_get(int local);
/* Nonzero when more input is already waiting (read-ahead or queued in
   OpenDoors) -- the editor skips redrawing between events that arrive
   in one burst and draws once for the lot. */
int ad_input_pending(void);
/* Drops everything typed ahead (read-ahead queue and OpenDoors' buffer). */
void ad_input_flush(void);
/* Async-signal-safe: marks that the window changed size (SIGWINCH);
   the next ad_input_get() returns AD_KEY_RESIZE. */
void ad_input_note_resize(void);

#endif /* ANETDRAW_INPUT_H */
