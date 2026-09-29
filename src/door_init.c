/* ANetDRAW's session-init layer, built on OpenDoors. Copied from
   ANetCRAFT_opendoors/src/door_init.c, which was adapted from
   rdq3_opendoors/src/door32.c, itself ported from ANetCHESS's
   src/main.c -- the same fix, already proven live over real
   telnet/SSH sessions against ANetBBS, Synchronet, and Mystic. Not
   new/experimental logic -- see the comments below for why each
   piece exists. */
#include "../include/anetdraw.h"
#include "../include/canvas.h"
#include "../include/input.h"
#include "../include/files.h"
#include "../include/wingui.h"
#include "OpenDoor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <process.h> /* _getpid() */
#define strcasecmp _stricmp
#define getpid _getpid
#else
#include <strings.h> /* strcasecmp() */
#include <unistd.h>  /* getpid() */
#include <signal.h>  /* signal(SIGPIPE), SIGWINCH */
#include <sys/ioctl.h> /* TIOCGWINSZ */
#endif

/* Root cause (found live on ANetCHESS, confirmed against real
   Synchronet/ANetBBS dropfiles, not guessed): some DOOR32.SYS writers
   use Comm Type=2 with Handle=-1 as a "use stdio" sentinel a few
   FreePascal door kits understand, but OpenDoors does not -- ANY
   nonzero od_open_handle (negative included) routes it into
   ODComOpenFromExistingHandle() instead of the normal port-open path,
   and a fd of -1 treated as a real socket just silently fails forever
   (black screen, not a crash). No real OS handle/fd is ever negative,
   so rewriting a corrected copy of the dropfile (Comm Type/Handle
   forced to 0/0, everything else passed through) before od_init()
   ever sees it is the only clean fix -- see WriteFixedDoor32Sys(). */
static char g_fixed_dropfile_path[1024];

#define OD_TEMP_STALE_SECONDS 300

static int IsAllDigits(const char *s, size_t len) {
    size_t i;
    if (len == 0) return 0;
    for (i = 0; i < len; i++) {
        if (s[i] < '0' || s[i] > '9') return 0;
    }
    return 1;
}

/* Removes leftover od<pid><suffix> files a past session's hard exit
   (od_exit() on Windows skips atexit/cleanup code -- ExitProcess())
   left behind. Age-gated, not PID-liveness-gated -- simpler, and a
   genuinely active concurrent session's own file is always seconds
   old. */
static void sweep_stale_od_temp_files(const char *dir, const char *suffix) {
    DIR *d = opendir(dir);
    struct dirent *ent;
    size_t suffix_len = strlen(suffix);
    time_t now = time(NULL);

    if (!d) return;

    while ((ent = readdir(d)) != NULL) {
        const char *name = ent->d_name;
        size_t len = strlen(name);
        char full_path[1280];
        struct stat st;

        if (len <= 2 + suffix_len) continue;
        if (name[0] != 'o' || name[1] != 'd') continue;
        if (strcmp(name + len - suffix_len, suffix) != 0) continue;
        if (!IsAllDigits(name + 2, len - 2 - suffix_len)) continue;

        snprintf(full_path, sizeof(full_path), "%s/%s", dir, name);
        if (stat(full_path, &st) == 0 &&
            (now - st.st_mtime) > OD_TEMP_STALE_SECONDS) {
            remove(full_path);
        }
    }
    closedir(d);
}

static int HasNegativeHandle(const char *path) {
    FILE *f = fopen(path, "r");
    char line1[64], line2[64];
    int ok;
    if (!f) return 0;
    ok = (fgets(line1, sizeof(line1), f) != NULL) &&
         (fgets(line2, sizeof(line2), f) != NULL);
    fclose(f);
    if (!ok) return 0;
    line1[strcspn(line1, "\r\n")] = '\0';
    line2[strcspn(line2, "\r\n")] = '\0';
    (void)line1;
    return atoi(line2) < 0;
}

/* Short, fixed path -- od_control.info_path is a fixed char[60], and
   od_init() re-detects dropfile type by matching the filename TAIL
   ("door32.sys"), so the copy must both be short and end that way.
   PID-suffixed so concurrent node sessions never collide. */
static int WriteFixedDoor32Sys(const char *src_path) {
    FILE *in = fopen(src_path, "r");
    FILE *out;
    char line[256];
    int line_no = 0;

    if (!in) return 0;

#ifdef _WIN32
    snprintf(g_fixed_dropfile_path, sizeof(g_fixed_dropfile_path),
        "od%ddoor32.sys", (int)getpid());
#else
    snprintf(g_fixed_dropfile_path, sizeof(g_fixed_dropfile_path),
        "/tmp/od%ddoor32.sys", (int)getpid());
#endif
    out = fopen(g_fixed_dropfile_path, "w");
    if (!out) {
        fclose(in);
        return 0;
    }

    while (fgets(line, sizeof(line), in) != NULL) {
        line_no++;
        if (line_no == 1 || line_no == 2) {
            fputs("0\n", out);
        } else {
            fputs(line, out);
        }
    }

    fclose(in);
    fclose(out);
    return 1;
}

static void FixUpDropFileArgIfNeeded(int argc, char *argv[]) {
    int i;
    for (i = 1; i < argc - 1; i++) {
        if (strcasecmp(argv[i], "-D") == 0 ||
            strcasecmp(argv[i], "-DROPFILE") == 0) {
            if (HasNegativeHandle(argv[i + 1]) &&
                WriteFixedDoor32Sys(argv[i + 1])) {
                argv[i + 1] = g_fixed_dropfile_path;
            }
            return;
        }
    }
}

/* Asks the terminal how big it is: park the cursor at 999;999 (every
   terminal clamps CUP to the real screen edge), send a cursor position
   report request (CSI 6n -- ECMA-48, and confirmed in SyncTERM's own
   cterm.adoc), and read back CSI rows;cols R. Needed because DOOR32.SYS
   has no screen-size field at all -- OpenDoors then falls back to 80x23
   (ODInEx1.c). Returns 0 if nothing sensible comes back in time. */
#define TERMSIZE_TIMEOUT_MS 1500

static int probe_terminal_size(int *cols, int *rows) {
    tODInputEvent ev;
    char buf[16];
    int n = 0, state = 0, r = 0;
    int waited_ms = 0, events = 0;

    od_disp_str("\x1b[s\x1b[999;999H\x1b[6n\x1b[u");

    for (;;) {
        if (!od_get_input(&ev, 100, GETIN_RAW)) {
            waited_ms += 100;
            if (waited_ms >= TERMSIZE_TIMEOUT_MS) return 0;
            continue;
        }
        /* A caller mashing keys during the probe must not keep this
           loop alive forever -- no real CPR reply is anywhere near
           this long. */
        if (++events > 64) return 0;
        if (ev.EventType != EVENT_CHARACTER) continue;
        {
            char c = ev.chKeyPress;
            switch (state) {
                case 0: if (c == 0x1b) state = 1; break;
                case 1: state = (c == '[') ? 2 : (c == 0x1b ? 1 : 0); n = 0; break;
                case 2:
                    if (c >= '0' && c <= '9' && n < (int)sizeof(buf) - 1) buf[n++] = c;
                    else if (c == ';' && n > 0) { buf[n] = 0; r = atoi(buf); n = 0; state = 3; }
                    else state = (c == 0x1b) ? 1 : 0;
                    break;
                case 3:
                    if (c >= '0' && c <= '9' && n < (int)sizeof(buf) - 1) buf[n++] = c;
                    else if (c == 'R' && n > 0) {
                        buf[n] = 0;
                        *rows = r;
                        *cols = atoi(buf);
                        return 1;
                    } else state = (c == 0x1b) ? 1 : 0;
                    break;
            }
        }
    }
}

/* Local mode on POSIX: the window is right there -- ask the OS. */
static int os_window_size(int *cols, int *rows) {
#ifndef _WIN32
    struct winsize ws;
    if (isatty(STDOUT_FILENO) && ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 &&
        ws.ws_col > 0 && ws.ws_row > 0) {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 1;
    }
#else
    if (ad_gui_active()) {
        ad_gui_size(cols, rows);
        return 1;
    }
#endif
    return 0;
}

static int sane_size(int cols, int rows) {
    return cols >= AD_MIN_COLS && cols <= AD_MAX_COLS &&
           rows >= AD_MIN_ROWS && rows <= AD_MAX_ROWS;
}

/* Whether the terminal answers CSI 6n: -1 not asked yet, 0 no, 1 yes.
   Once a probe has gone unanswered, later ones (^L) skip it -- each
   unanswered probe stalls for the full timeout and eats any keys or
   clicks that arrive meanwhile. */
static int g_cpr_answers = -1;

void ad_door_query_size(const AdDoor *d, int *cols, int *rows) {
    int c = 0, r = 0;
    if (d->local && os_window_size(&c, &r) && sane_size(c, r)) {
        *cols = c;
        *rows = r;
        return;
    }
#ifdef _WIN32
    /* OpenDoors' own Windows console window never answers CSI 6n --
       don't make ^L sit through the timeout there */
    if (d->local) return;
#endif
    if (g_cpr_answers == 0) return;
    if (probe_terminal_size(&c, &r)) {
        g_cpr_answers = 1;
        if (sane_size(c, r)) {
            *cols = c;
            *rows = r;
        }
    } else {
        g_cpr_answers = 0;
    }
    /* otherwise leave the caller's current size alone */
}

#ifndef _WIN32
static void on_sigwinch(int sig) {
    (void)sig;
    ad_input_note_resize();
}
#endif

static AdDoor *g_active_door = NULL;

void ad_set_active_door(AdDoor *door) { g_active_door = door; }
AdDoor *ad_get_active_door(void) { return g_active_door; }

int ad_door_init(AdDoor *d, int argc, char **argv) {
    if (!d) return 0;
    memset(d, 0, sizeof(*d));

#ifndef _WIN32
    /* A caller hanging up mid-write would otherwise kill the process
       with SIGPIPE (seen in Phase 1 testing: rc=-13) before it can
       notice the lost carrier and exit through od_exit() like normal. */
    signal(SIGPIPE, SIG_IGN);
#endif

    sweep_stale_od_temp_files(".", "intro.ans");
#ifdef _WIN32
    sweep_stale_od_temp_files(".", "door32.sys");
#else
    sweep_stale_od_temp_files("/tmp", "door32.sys");
#endif

    FixUpDropFileArgIfNeeded(argc, argv);
    /* ANetDRAW links OpenDoors::StaticConsole on Windows (see
       CMakeLists.txt), which defines OD_WINDOWS_CONSOLE -- that makes
       OpenDoor.h's od_parse_cmd_line a macro for
       od_parse_cmd_line_cons(argc, argv), the same (int, char**)
       signature POSIX already uses here. Getting this wrong (linking
       plain OpenDoors::Static on Windows) compiles and links fine but
       refuses to run under a console host at runtime ("requires a
       Windows GUI executable") -- caught on RDQ3 by an actual Wine
       run, not by a clean cross-compile; verify the same way here. */
    od_parse_cmd_line(argc, argv);

    /* Same reasoning as RDQ3/ANetCHESS: local/manual testing (Comm
       Type 0, no real socket) hangs od_init() waiting on carrier/BPS
       negotiation without these. Deliberately NOT DIS_LOCAL_INPUT -- a
       PTY-launched native door's only input path IS local input via
       od_force_local; disabling it silently drops every keypress. */
    od_control.od_disable = DIS_NAME_PROMPT | DIS_CARRIERDETECT |
        DIS_TIMEOUT | DIS_BPS_SETTING;

    od_init();

    if (g_fixed_dropfile_path[0] != '\0') {
        remove(g_fixed_dropfile_path);
    }

    /* Board/menu screens are drawn with explicit cursor positioning;
       OpenDoors' own built-in "Continue? [Y/n/=]" pagination during
       od_send_file() is never wanted here. */
    od_control.od_page_pausing = FALSE;

    /* Canvas glyphs include CP437 codes like 0x16/0x19 that RemoteAccess
       treats as color/repeat commands -- never let OpenDoors translate
       them on the way to the caller (see ad_emit() in output.c). */
    od_control.od_no_ra_codes = TRUE;
    ad_output_take_utf8();  /* local UTF-8 terminal: output.c converts */

    /* NO_DOOR_FILE means od_init() found no dropfile at all -- treat
       that as a local test session. */
    d->local = (od_control.od_info_type == NO_DOOR_FILE) ? 1 : 0;
    d->node = (int)od_control.od_node;
    d->mouse = 1;
    d->canvas_w = AD_CANVAS_W;
    {
        int i, want_cols = 0, want_rows = 0, sysop_level = 0, use_console = 0;
        const char *data_arg = "anetdraw_data";
        const char *fonts_arg = "fonts";
        for (i = 1; i < argc; i++) {
            if (strcmp(argv[i], "--no-mouse") == 0) d->mouse = 0;
            if (strcmp(argv[i], "--console") == 0) use_console = 1;
            if (i + 1 < argc && strcmp(argv[i], "--rows") == 0) want_rows = atoi(argv[i + 1]);
            if (i + 1 < argc && strcmp(argv[i], "--cols") == 0) want_cols = atoi(argv[i + 1]);
            if (i + 1 < argc && strcmp(argv[i], "--width") == 0) d->canvas_w = atoi(argv[i + 1]);
            if (i + 1 < argc && strcmp(argv[i], "--trace") == 0) ad_trace_open(argv[i + 1]);
            if (i + 1 < argc && strcmp(argv[i], "--sysop-level") == 0) sysop_level = atoi(argv[i + 1]);
            if (i + 1 < argc && strcmp(argv[i], "--data") == 0) data_arg = argv[i + 1];
            if (i + 1 < argc && strcmp(argv[i], "--fonts") == 0) fonts_arg = argv[i + 1];
        }
        if (d->canvas_w < 1 || d->canvas_w > AD_CANVAS_MAX_W) d->canvas_w = AD_CANVAS_W;

        /* Screen size, first answer wins:
           1. --cols / --rows (sysop override)
           2. the dropfile, but only CHAIN.TXT and BBSDEV.DRP -- both
              carry the caller's real terminal size (CHAIN.TXT lines 9-10,
              BBSDEV.DRP lines 6-7; see ODInEx1.c). Every other format
              leaves OpenDoors' invented 80x23 in od_control, and DOOR.SYS's
              "screen length" is a page-pause setting, not a size.
           3. local mode: the OS window size (TIOCGWINSZ)
           4. asking the terminal (CSI 6n)
           5. 80x24 -- on a 25-row terminal that only leaves the bottom
              line unused; guessing 25 on a 24-row one would scroll it */
        d->cols = 0;
        d->rows = 0;
        if (od_control.od_info_type == CHAINTXT || od_control.od_info_type == BBSDEVDRP) {
            if (sane_size(od_control.user_screenwidth, od_control.user_screen_length)) {
                d->cols = od_control.user_screenwidth;
                d->rows = (int)od_control.user_screen_length;
            }
        }
#ifdef _WIN32
        /* Windows local mode: ANetDRAW's own window (wingui.h) --
           resizable, full screen, mouse. --console keeps OpenDoors'
           console window instead: 80 columns, bottom 2 lines kept for
           the sysop status bar, and it doesn't answer CSI 6n. */
        if (!d->cols && d->local) {
            if (use_console || !ad_gui_start(want_cols ? want_cols : 80, want_rows ? want_rows : 25,
                                             &d->cols, &d->rows)) {
                d->cols = 80;
                d->rows = 23;
            } else {
                want_cols = want_rows = 0;  /* the window was opened at that size */
            }
        }
#else
        (void)use_console;
#endif
        if (!d->cols) ad_door_query_size(d, &d->cols, &d->rows);
        if (!d->cols) { d->cols = 80; d->rows = 24; }
        if (want_cols) d->cols = want_cols;
        if (want_rows) d->rows = want_rows;
        if (d->cols < AD_MIN_COLS) d->cols = AD_MIN_COLS;
        if (d->cols > AD_MAX_COLS) d->cols = AD_MAX_COLS;
        if (d->rows < AD_MIN_ROWS) d->rows = AD_MIN_ROWS;
        if (d->rows > AD_MAX_ROWS) d->rows = AD_MAX_ROWS;

        /* Sysop: local mode always; remotely only when the sysop set
           --sysop-level and this caller's dropfile level reaches it
           (ANetBBS writes 200 for admins; BBSDEV.DRP "sysop" = 100).
           No flag = no remote caller ever gets the file browser. */
        d->sysop = d->local ||
                   (sysop_level > 0 && (int)od_control.user_security >= sysop_level);
        {
            char key[96], users[1024];
            ad_mkdirs(data_arg);
            if (!ad_abspath(data_arg, d->data_dir, sizeof(d->data_dir)))
                snprintf(d->data_dir, sizeof(d->data_dir), "%s", data_arg);
            ad_user_key((int)od_control.user_num,
                        od_control.user_name[0] ? od_control.user_name : "local",
                        key, sizeof(key));
            ad_path_join(d->data_dir, "users", users, sizeof(users));
            ad_path_join(users, key, d->user_dir, sizeof(d->user_dir));
            ad_mkdirs(d->user_dir);
            snprintf(d->user_key, sizeof(d->user_key), "%s", key);
            ad_path_join(d->data_dir, "gallery", d->gallery_dir, sizeof(d->gallery_dir));
            ad_mkdirs(d->gallery_dir);
            if (!ad_abspath(fonts_arg, d->fonts_dir, sizeof(d->fonts_dir)))
                snprintf(d->fonts_dir, sizeof(d->fonts_dir), "%s", fonts_arg);
        }
    }
#ifndef _WIN32
    /* Local mode: resizing/maximizing the terminal window re-lays-out
       the editor live (the input loop picks the flag up). */
    if (d->local) signal(SIGWINCH, on_sigwinch);
#endif
    d->sec_level = (int)od_control.user_security;
    d->baud = (int)od_control.baud;
    if (od_control.user_name[0]) {
        size_t i;
        for (i = 0; i < sizeof(d->user_name) - 1 && od_control.user_name[i]; ++i)
            d->user_name[i] = od_control.user_name[i];
        d->user_name[i] = 0;
    } else {
        strcpy(d->user_name, "LocalUser");
    }
    /* OpenDoors' od_control has no generic "BBS name" field -- no cfg
       system exists yet (Phase 0), so a fixed fallback
       for now; revisit once ANetDRAW gets its own cfg file, same as
       RDQ3's rdq3_cfg_bbs_name(). */
    strcpy(d->bbs_name, "ANetBBS");
    if (d->node <= 0) d->node = 1;

    ad_set_active_door(d);
    return 1;
}

void ad_door_close(AdDoor *d) {
    (void)d;
    ad_set_active_door(NULL);
    /* od_exit() is the real teardown call -- see main.c. Nothing to
       flush/close here; OpenDoors owns the transport. */
}
