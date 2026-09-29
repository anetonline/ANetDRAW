/* ANetDRAW Door -- ANSI/ASCII art editor for BBS callers, C/OpenDoors.
   Session info + the output chokepoint. Each subsystem has its own
   header (canvas.h, render.h, input.h, editor.h, ...). */
#ifndef ANETDRAW_H
#define ANETDRAW_H

#include <stdint.h>
#include <stddef.h>

#define AD_VERSION "0.1.0"

/* Screen size limits. 80 columns minimum: the status bar is laid out
   for it. The maximum covers a maximized local window on a big monitor. */
#define AD_MIN_COLS 80
#define AD_MAX_COLS 400
#define AD_MIN_ROWS 10
#define AD_MAX_ROWS 200

#define AD_NAME_LEN 64
#define AD_BBS_NAME_LEN 64

/* Session info, populated from OpenDoors' own od_control after
   od_init() has parsed whatever dropfile (DOOR32.SYS/DOOR.SYS/etc) the
   BBS handed us -- see ad_door_init() in door_init.c. Same shape as
   ANetCRAFT's AcDoor and RDQ3's Door32 -- proven pattern in this
   environment, not reinvented. All actual terminal I/O goes through
   ad_dout()/ad_doutf() (output.c), not through this struct directly. */
typedef struct {
    int local;
    int node;
    char user_name[AD_NAME_LEN];
    char bbs_name[AD_BBS_NAME_LEN];
    int cols;
    int rows;
    int sec_level;
    int baud;
    int mouse;   /* 1 = turn on terminal mouse reporting (--no-mouse = 0) */
    int canvas_w; /* new-canvas width (--width, default 80) */
    /* Saving. The sysop -- local mode, or a caller whose dropfile
       security level is at least --sysop-level -- gets a real file
       browser. Everyone else saves into their own folder under
       --data (default "anetdraw_data"), and never types a path. */
    int sysop;
    char data_dir[1024];   /* absolute */
    char user_dir[1024];   /* absolute: data_dir/users/<key> */
    char user_key[96];     /* "<num>_<name>": owns this caller's gallery pieces */
    char gallery_dir[1024]; /* absolute: data_dir/gallery, shared by everyone */
    char fonts_dir[1024];   /* TheDraw fonts (--fonts, default "fonts") */
} AdDoor;

/* door_init.c */
int  ad_door_init(AdDoor *d, int argc, char **argv);
void ad_door_close(AdDoor *d);
/* Current terminal size: the OS window in local mode, else asks the
   terminal (CSI 6n, up to 1.5s). Leaves cols/rows alone if neither
   answers. */
void ad_door_query_size(const AdDoor *d, int *cols, int *rows);

/* output.c -- CRLF-safe output chokepoint (od_disp_str()/od_printf()
   never translate a bare '\n' to '\r\n' -- see the door-game-release
   project convention; this tracks the last char sent and inserts '\r'
   before any '\n' not already preceded by one, same fix already
   proven in RDQ3's common.c dout()). Every byte the door sends goes
   through here -- never call od_disp_str()/od_printf() directly. */
/* Local mode under a UTF-8 locale: ad_door_init() calls
   ad_output_take_utf8() so the door, not OpenDoors, converts CP437 to
   UTF-8 (see output.c). ad_output_utf8() says whether that is on;
   ad_cp437_utf8() writes one CP437 glyph as 1-3 UTF-8 bytes;
   ad_dout_utf8() sends already-UTF-8 text untouched. */
void ad_output_take_utf8(void);
int ad_output_utf8(void);
size_t ad_cp437_utf8(unsigned char ch, char *out);
void ad_dout_utf8(const char *s);
void ad_dout(const char *s);
void ad_doutf(const char *fmt, ...);

/* --trace FILE: millisecond-stamped log of input events and screen
   flushes, for diagnosing lag on a real BBS link (is the door slow, or
   is the delay upstream?). No-op unless ad_trace_open() succeeded. */
int  ad_trace_open(const char *path);
void ad_trace(const char *fmt, ...);

#endif /* ANETDRAW_H */
