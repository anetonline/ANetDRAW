/* Windows local window -- see include/wingui.h. */
#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601  /* Windows 7 */
#endif
#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include "../include/wingui.h"
#include "../include/anetdraw.h"

extern const unsigned char ad_font8x16[256][16];

#define CW 8
#define CH 16
#define WM_AD_REDRAW (WM_APP + 1)
#define BLINK_TIMER 1
#define QLEN 512

/* VGA palette, PC color order, as 0x00RRGGBB for a 32-bit DIB */
static const DWORD PALETTE[16] = {
    0x000000, 0x0000AA, 0x00AA00, 0x00AAAA, 0xAA0000, 0xAA00AA, 0xAA5500, 0xAAAAAA,
    0x555555, 0x5555FF, 0x55FF55, 0x55FFFF, 0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF
};
static const int ANSI_TO_PC[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };

typedef struct { unsigned char ch, attr; } GCell;

static CRITICAL_SECTION g_lock;
static HANDLE g_key_event, g_ready;
static HWND g_hwnd;
static volatile LONG g_active = 0, g_posted = 0;

/* the ANSI screen (all under g_lock) */
static int g_cols = 80, g_rows = 25;
static GCell *g_grid;            /* AD_MAX_COLS x AD_MAX_ROWS, row stride AD_MAX_COLS */
static int g_cx, g_cy, g_wrap, g_saved_x, g_saved_y;
static unsigned char g_attr = 0x07;
static int g_ice, g_cursor_on = 1, g_mouse_on;
static int g_state;              /* 0 text, 1 after ESC, 2 in CSI */
static int g_params[16], g_nparams, g_private;
static char g_inter;

/* keys for the door */
static AdKey g_q[QLEN];
static int g_qhead, g_qlen;

/* drawing (GUI thread only) */
/* Zoom, in percent, stepped through like other drawing programs: whole
   multiples stay pixel-sharp, the steps between are smoothed. */
static const int ZOOMS[] = { 50, 75, 100, 125, 150, 175, 200, 250, 300, 400, 500, 600 };
#define NZOOMS ((int)(sizeof(ZOOMS) / sizeof(ZOOMS[0])))
static int g_zoom = 100, g_fullscreen, g_blink_phase = 1;
static int g_windowed_zoom = 100, g_max_zoomed;  /* zoom to go back to */
#define ZPX(n) ((n) * g_zoom / 100)  /* DIB pixels -> window pixels */
static WINDOWPLACEMENT g_saved_place = { sizeof(WINDOWPLACEMENT) };
static LONG g_saved_style;
static BITMAPINFO g_bmi;
static DWORD *g_pixels;
static int g_dib_w, g_dib_h;
static GCell *g_shown;           /* what the DIB currently shows, per cell */
static unsigned char *g_shown_flags;
static int g_mouse_button = -1, g_mouse_lx = -1, g_mouse_ly = -1;

#define CELL(x, y) g_grid[(y) * AD_MAX_COLS + (x)]

/* ------------------------------------------------------------ keys */

static AdKey blank(void) {
    AdKey k;
    memset(&k, 0, sizeof(k));
    return k;
}

static void push_key(AdKey k) {
    EnterCriticalSection(&g_lock);
    if (g_qlen < QLEN) {
        /* collapse a run of drags (a fast mouse) to the newest one */
        if (k.kind == AD_KEY_MOUSE && k.mdrag && g_qlen > 0) {
            AdKey *last = &g_q[(g_qhead + g_qlen - 1) % QLEN];
            if (last->kind == AD_KEY_MOUSE && last->mdrag && last->mbutton == k.mbutton && g_qlen > 64) {
                *last = k;
                LeaveCriticalSection(&g_lock);
                SetEvent(g_key_event);
                return;
            }
        }
        g_q[(g_qhead + g_qlen) % QLEN] = k;
        g_qlen++;
    }
    LeaveCriticalSection(&g_lock);
    SetEvent(g_key_event);
}

static void push_kind(AdKeyKind kind) {
    AdKey k = blank();
    k.kind = kind;
    push_key(k);
}

int ad_gui_get_key(AdKey *k, int ms) {
    DWORD start = GetTickCount();
    for (;;) {
        DWORD waited;
        EnterCriticalSection(&g_lock);
        if (g_qlen > 0) {
            *k = g_q[g_qhead];
            g_qhead = (g_qhead + 1) % QLEN;
            g_qlen--;
            LeaveCriticalSection(&g_lock);
            return 1;
        }
        LeaveCriticalSection(&g_lock);
        waited = GetTickCount() - start;
        if ((int)waited >= ms) return 0;
        WaitForSingleObject(g_key_event, (DWORD)ms - waited);
    }
}

int ad_gui_pending(void) {
    int n;
    EnterCriticalSection(&g_lock);
    n = g_qlen;
    LeaveCriticalSection(&g_lock);
    return n > 0;
}

void ad_gui_flush_input(void) {
    EnterCriticalSection(&g_lock);
    g_qlen = 0;
    LeaveCriticalSection(&g_lock);
}

/* ------------------------------------------------------------ ANSI screen */

static void clear_cells(int x0, int y0, int x1, int y1) {  /* inclusive, one row range */
    int x, y;
    for (y = y0; y <= y1; y++)
        for (x = (y == y0 ? x0 : 0); x <= (y == y1 ? x1 : g_cols - 1); x++) {
            CELL(x, y).ch = ' ';
            CELL(x, y).attr = (unsigned char)(g_attr & 0xF0 & 0x70) | 0x07;
        }
}

static void scroll_up(void) {
    int y;
    for (y = 1; y < g_rows; y++)
        memcpy(&CELL(0, y - 1), &CELL(0, y), (size_t)g_cols * sizeof(GCell));
    clear_cells(0, g_rows - 1, g_cols - 1, g_rows - 1);
}

static void line_feed(void) {
    if (++g_cy >= g_rows) {
        g_cy = g_rows - 1;
        scroll_up();
    }
}

static void put_glyph(unsigned char ch) {
    if (g_wrap) {
        g_wrap = 0;
        g_cx = 0;
        line_feed();
    }
    CELL(g_cx, g_cy).ch = ch;
    CELL(g_cx, g_cy).attr = g_attr;
    if (++g_cx >= g_cols) {
        g_cx = g_cols - 1;
        g_wrap = 1;  /* deferred wrap, VT style */
    }
}

static int param(int i, int def) {
    return (i < g_nparams && g_params[i] > 0) ? g_params[i] : def;
}

static void sgr(void) {
    int i, fg = g_attr & 15, bg = (g_attr >> 4) & 15;
    if (g_nparams == 0) { fg = 7; bg = 0; }
    for (i = 0; i < g_nparams; i++) {
        int p = g_params[i];
        if (p == 0) { fg = 7; bg = 0; }
        else if (p == 1) fg |= 8;
        else if (p == 22) fg &= 7;
        else if (p == 5 || p == 6) bg |= 8;
        else if (p == 25) bg &= 7;
        else if (p >= 30 && p <= 37) fg = (fg & 8) | ANSI_TO_PC[p - 30];
        else if (p == 39) fg = (fg & 8) | 7;
        else if (p >= 40 && p <= 47) bg = (bg & 8) | ANSI_TO_PC[p - 40];
        else if (p == 49) bg &= 8;
        else if (p >= 90 && p <= 97) fg = 8 | ANSI_TO_PC[p - 90];
        else if (p >= 100 && p <= 107) bg = 8 | ANSI_TO_PC[p - 100];
    }
    g_attr = (unsigned char)((bg << 4) | fg);
}

static void set_mode(int on) {
    int i;
    for (i = 0; i < g_nparams; i++) {
        int p = g_params[i];
        if (!g_private) continue;
        if (p == 25) g_cursor_on = on;
        else if (p == 33) g_ice = on;
        else if (p == 1000 || p == 1002 || p == 1003) g_mouse_on = on;
    }
}

static void csi(unsigned char f) {
    int n;
    switch (f) {
        case 'H': case 'f':
            g_cy = param(0, 1) - 1;
            g_cx = param(1, 1) - 1;
            break;
        case 'A': g_cy -= param(0, 1); break;
        case 'B': g_cy += param(0, 1); break;
        case 'C': g_cx += param(0, 1); break;
        case 'D': g_cx -= param(0, 1); break;
        case 'G': g_cx = param(0, 1) - 1; break;
        case 'd': g_cy = param(0, 1) - 1; break;
        case 'J':
            n = g_nparams ? g_params[0] : 0;
            if (n == 2 || n == 3) { clear_cells(0, 0, g_cols - 1, g_rows - 1); }
            else if (n == 1) clear_cells(0, 0, g_cx, g_cy);
            else clear_cells(g_cx, g_cy, g_cols - 1, g_rows - 1);
            break;
        case 'K':
            n = g_nparams ? g_params[0] : 0;
            if (n == 2) clear_cells(0, g_cy, g_cols - 1, g_cy);
            else if (n == 1) clear_cells(0, g_cy, g_cx, g_cy);
            else clear_cells(g_cx, g_cy, g_cols - 1, g_cy);
            break;
        case 'm': if (!g_private && !g_inter) sgr(); break;
        case 'h': set_mode(1); break;
        case 'l': set_mode(0); break;
        case 's': g_saved_x = g_cx; g_saved_y = g_cy; break;
        case 'u': g_cx = g_saved_x; g_cy = g_saved_y; break;
        default: break;  /* DECSCUSR, DECSCS, CPR requests ...: nothing to do */
    }
    g_wrap = 0;
    if (g_cx < 0) g_cx = 0;
    if (g_cx >= g_cols) g_cx = g_cols - 1;
    if (g_cy < 0) g_cy = 0;
    if (g_cy >= g_rows) g_cy = g_rows - 1;
}

static void feed(unsigned char c) {
    if (g_state == 1) {
        if (c == '[') {
            g_state = 2;
            g_nparams = 0;
            g_private = 0;
            g_inter = 0;
            memset(g_params, 0, sizeof(g_params));
        } else {
            g_state = 0;
        }
        return;
    }
    if (g_state == 2) {
        if (c >= '0' && c <= '9') {
            if (g_nparams == 0) g_nparams = 1;
            if (g_params[g_nparams - 1] < 100000) g_params[g_nparams - 1] = g_params[g_nparams - 1] * 10 + (c - '0');
        } else if (c == ';') {
            if (g_nparams == 0) g_nparams = 1;
            if (g_nparams < 16) g_params[g_nparams++] = 0;
        } else if (c == '?' || c == '<' || c == '=' || c == '>') {
            g_private = 1;
        } else if (c >= 0x20 && c <= 0x2F) {
            g_inter = (char)c;
        } else if (c >= 0x40 && c <= 0x7E) {
            csi(c);
            g_state = 0;
        } else {
            g_state = 0;
        }
        return;
    }
    /* Text. Like SyncTERM, only these control codes act; every other
       byte below 0x20 is a CP437 picture glyph (the renderer already
       swaps these for look-alikes when a canvas cell holds one). */
    switch (c) {
        case 0x1B: g_state = 1; return;
        case '\r': g_cx = 0; g_wrap = 0; return;
        case '\n': g_wrap = 0; line_feed(); return;
        case '\b': if (g_cx > 0) g_cx--; g_wrap = 0; return;
        case 0x00: case 0x07: return;
        default: put_glyph(c); return;
    }
}

void ad_gui_write(const char *buf, size_t len) {
    size_t i;
    if (!g_active) return;
    EnterCriticalSection(&g_lock);
    for (i = 0; i < len; i++) feed((unsigned char)buf[i]);
    LeaveCriticalSection(&g_lock);
    if (InterlockedExchange(&g_posted, 1) == 0) PostMessageW(g_hwnd, WM_AD_REDRAW, 0, 0);
}

void ad_gui_size(int *cols, int *rows) {
    EnterCriticalSection(&g_lock);
    *cols = g_cols;
    *rows = g_rows;
    LeaveCriticalSection(&g_lock);
}

int ad_gui_active(void) { return g_active != 0; }

/* ------------------------------------------------------------ drawing */

static void ensure_dib(int cols, int rows) {
    int w = cols * CW, h = rows * CH;
    if (w == g_dib_w && h == g_dib_h && g_pixels) return;
    free(g_pixels);
    g_pixels = (DWORD *)calloc((size_t)w * h, sizeof(DWORD));
    g_dib_w = w;
    g_dib_h = h;
    memset(&g_bmi, 0, sizeof(g_bmi));
    g_bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    g_bmi.bmiHeader.biWidth = w;
    g_bmi.bmiHeader.biHeight = -h;  /* top-down */
    g_bmi.bmiHeader.biPlanes = 1;
    g_bmi.bmiHeader.biBitCount = 32;
    g_bmi.bmiHeader.biCompression = BI_RGB;
    /* force every cell to be drawn */
    memset(g_shown_flags, 0xFF, (size_t)AD_MAX_COLS * AD_MAX_ROWS);
}

/* Draws changed cells into the DIB. Returns whether any cell blinks. */
static int render(void) {
    int x, y, any_blink = 0, cols, rows, cx, cy, cur;
    EnterCriticalSection(&g_lock);
    cols = g_cols;
    rows = g_rows;
    ensure_dib(cols, rows);
    cx = g_cx;
    cy = g_cy;
    cur = g_cursor_on;
    for (y = 0; y < rows; y++) {
        for (x = 0; x < cols; x++) {
            GCell c = CELL(x, y);
            int fg = c.attr & 15, bg = (c.attr >> 4) & 15, gy, gx;
            int is_cur = cur && x == cx && y == cy;
            unsigned char flags;
            DWORD *row;
            if (bg >= 8 && !g_ice) {  /* blink mode: normal background, blinking text */
                bg &= 7;
                any_blink = 1;
                if (!g_blink_phase) fg = bg;
            }
            flags = (unsigned char)(0x80 | (is_cur ? 1 : 0));
            {
                size_t i = (size_t)y * AD_MAX_COLS + x;
                /* the shown key: glyph, final colors, cursor */
                unsigned char key_attr = (unsigned char)((bg << 4) | fg);
                if (g_shown[i].ch == c.ch && g_shown[i].attr == key_attr && g_shown_flags[i] == flags) continue;
                g_shown[i].ch = c.ch;
                g_shown[i].attr = key_attr;
                g_shown_flags[i] = flags;
            }
            for (gy = 0; gy < CH; gy++) {
                unsigned char bits = ad_font8x16[c.ch][gy];
                if (is_cur && gy >= CH - 2) bits = 0xFF;  /* steady underline cursor */
                row = g_pixels + (size_t)(y * CH + gy) * g_dib_w + x * CW;
                for (gx = 0; gx < CW; gx++)
                    row[gx] = PALETTE[(bits & (0x80 >> gx)) ? (is_cur && gy >= CH - 2 && fg == bg ? 15 - bg : fg) : bg];
            }
        }
    }
    LeaveCriticalSection(&g_lock);
    return any_blink;
}

static void paint(HWND hwnd, HDC hdc) {
    RECT rc;
    int w, h, ox, oy;
    HBRUSH black = (HBRUSH)GetStockObject(BLACK_BRUSH);
    GetClientRect(hwnd, &rc);
    w = ZPX(g_dib_w);
    h = ZPX(g_dib_h);
    ox = (rc.right - w) / 2;
    oy = (rc.bottom - h) / 2;
    if (ox < 0) ox = 0;
    if (oy < 0) oy = 0;
    if (g_zoom % 100 == 0) {
        SetStretchBltMode(hdc, COLORONCOLOR);  /* whole multiples: sharp pixels */
    } else {
        SetStretchBltMode(hdc, HALFTONE);      /* in between: smooth, even */
        SetBrushOrgEx(hdc, 0, 0, NULL);
    }
    StretchDIBits(hdc, ox, oy, w, h, 0, 0, g_dib_w, g_dib_h, g_pixels, &g_bmi, DIB_RGB_COLORS, SRCCOPY);
    /* letterbox bars */
    {
        RECT r;
        SetRect(&r, 0, 0, rc.right, oy); FillRect(hdc, &r, black);
        SetRect(&r, 0, oy + h, rc.right, rc.bottom); FillRect(hdc, &r, black);
        SetRect(&r, 0, oy, ox, oy + h); FillRect(hdc, &r, black);
        SetRect(&r, ox + w, oy, rc.right, oy + h); FillRect(hdc, &r, black);
    }
}

/* ------------------------------------------------------------ sizing */

static RECT work_area(HWND hwnd) {
    MONITORINFO mi;
    RECT r;
    mi.cbSize = sizeof(mi);
    if (hwnd && GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi))
        return g_fullscreen ? mi.rcMonitor : mi.rcWork;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &r, 0);
    return r;
}

/* The client area changed: work out the new grid, tell the door. */
static void on_client_size(int cw, int ch) {
    int cols = cw * 100 / (CW * g_zoom), rows = ch * 100 / (CH * g_zoom);
    int changed;
    ad_trace("GUI client %dx%d zoom %d%% fullscreen %d", cw, ch, g_zoom, g_fullscreen);
    if (cols < AD_MIN_COLS) cols = AD_MIN_COLS;
    if (cols > AD_MAX_COLS) cols = AD_MAX_COLS;
    if (rows < AD_MIN_ROWS) rows = AD_MIN_ROWS;
    if (rows > AD_MAX_ROWS) rows = AD_MAX_ROWS;
    EnterCriticalSection(&g_lock);
    changed = cols != g_cols || rows != g_rows;
    if (changed) {
        int y;
        /* cells beyond the old size start blank */
        for (y = 0; y < rows; y++) {
            int x;
            for (x = (y < g_rows ? g_cols : 0); x < cols; x++) {
                CELL(x, y).ch = ' ';
                CELL(x, y).attr = 0x07;
            }
        }
        g_cols = cols;
        g_rows = rows;
        if (g_cx >= cols) g_cx = cols - 1;
        if (g_cy >= rows) g_cy = rows - 1;
    }
    LeaveCriticalSection(&g_lock);
    if (changed) push_kind(AD_KEY_RESIZE);
    InvalidateRect(g_hwnd, NULL, FALSE);
}

/* Window size for a given client size. */
static void client_to_window(int cw, int ch, int *ww, int *wh) {
    RECT r;
    SetRect(&r, 0, 0, cw, ch);
    AdjustWindowRectEx(&r, (DWORD)GetWindowLongW(g_hwnd, GWL_STYLE), FALSE, 0);
    *ww = r.right - r.left;
    *wh = r.bottom - r.top;
}

static void set_title(void) {
    char title[160];
    WCHAR wtitle[160];
    wsprintfA(title, "ANetDRAW %s (local)  %d%%   -  Alt+Enter: full screen   Ctrl +/-: zoom", AD_VERSION, g_zoom);
    MultiByteToWideChar(CP_ACP, 0, title, -1, wtitle, 160);
    SetWindowTextW(g_hwnd, wtitle);
}

/* Does an 80x25 screen fit in w x h at zoom z? */
static int fits(int z, int w, int h, int cols, int rows) {
    return CW * cols * z / 100 <= w && CH * rows * z / 100 <= h;
}

/* Largest zoom at which 80x25 still fits the screen. */
static int max_zoom(void) {
    RECT wa = work_area(g_hwnd);
    int i;
    for (i = NZOOMS - 1; i > 0; i--)
        if (fits(ZOOMS[i], wa.right - wa.left, wa.bottom - wa.top - 40, 80, 25)) return ZOOMS[i];
    return ZOOMS[0];
}

static void set_zoom(int z) {
    RECT rc;
    int cols, rows;
    if (z < ZOOMS[0] || z > max_zoom() || z == g_zoom) return;
    ad_gui_size(&cols, &rows);
    g_zoom = z;
    set_title();
    if (g_fullscreen || IsZoomed(g_hwnd)) {
        GetClientRect(g_hwnd, &rc);
        on_client_size(rc.right, rc.bottom);
    } else {
        /* keep the same grid, resize the window around it (clamped to the screen) */
        RECT wa = work_area(g_hwnd);
        int ww, wh;
        client_to_window(ZPX(cols * CW), ZPX(rows * CH), &ww, &wh);
        if (ww > wa.right - wa.left) ww = wa.right - wa.left;
        if (wh > wa.bottom - wa.top) wh = wa.bottom - wa.top;
        GetWindowRect(g_hwnd, &rc);
        if (rc.left + ww > wa.right) rc.left = wa.right - ww;
        if (rc.top + wh > wa.bottom) rc.top = wa.bottom - wh;
        if (rc.left < wa.left) rc.left = wa.left;
        if (rc.top < wa.top) rc.top = wa.top;
        SetWindowPos(g_hwnd, NULL, rc.left, rc.top, ww, wh, SWP_NOZORDER);
        GetClientRect(g_hwnd, &rc);
        on_client_size(rc.right, rc.bottom);
    }
    InvalidateRect(g_hwnd, NULL, TRUE);
}

/* One step in or out along ZOOMS. */
static void zoom_step(int dir) {
    int i;
    for (i = 0; i < NZOOMS && ZOOMS[i] < g_zoom; i++) {}
    if (dir > 0) i = (i < NZOOMS && ZOOMS[i] == g_zoom) ? i + 1 : i;
    else i = i - 1;
    if (i >= 0 && i < NZOOMS) set_zoom(ZOOMS[i]);
}

/* Zoom for a big area (full screen / maximized): the largest one, 100%
   or more, that still leaves room for the toolbox (80 canvas columns +
   39), else the largest that fits 80x25. */
#define TOOLBOX_COLS 119
static int best_zoom_for(int w, int h) {
    int i;
    for (i = NZOOMS - 1; i >= 0 && ZOOMS[i] >= 100; i--)
        if (fits(ZOOMS[i], w, h, TOOLBOX_COLS, 25)) return ZOOMS[i];
    for (i = NZOOMS - 1; i > 0; i--)
        if (fits(ZOOMS[i], w, h, AD_MIN_COLS, 25)) return ZOOMS[i];
    return ZOOMS[0];
}

static void toggle_fullscreen(HWND hwnd) {
    if (!g_fullscreen) {
        MONITORINFO mi;
        mi.cbSize = sizeof(mi);
        GetWindowPlacement(hwnd, &g_saved_place);
        g_saved_style = GetWindowLongW(hwnd, GWL_STYLE);
        GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi);
        g_windowed_zoom = g_zoom;
        g_zoom = best_zoom_for(mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top);
        set_title();
        g_fullscreen = 1;
        SetWindowLongW(hwnd, GWL_STYLE, g_saved_style & ~(WS_OVERLAPPEDWINDOW));
        SetWindowPos(hwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
                     mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top,
                     SWP_FRAMECHANGED | SWP_NOOWNERZORDER);
    } else {
        g_fullscreen = 0;
        g_zoom = g_windowed_zoom;
        set_title();
        SetWindowLongW(hwnd, GWL_STYLE, g_saved_style);
        SetWindowPlacement(hwnd, &g_saved_place);
        SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
    InvalidateRect(hwnd, NULL, TRUE);
}

/* ------------------------------------------------------------ mouse */

static int mouse_cell(LPARAM lp, int *cx, int *cy) {
    RECT rc;
    int x = (short)LOWORD(lp), y = (short)HIWORD(lp), ox, oy, cols, rows;
    GetClientRect(g_hwnd, &rc);
    ox = (rc.right - ZPX(g_dib_w)) / 2;
    oy = (rc.bottom - ZPX(g_dib_h)) / 2;
    if (ox < 0) ox = 0;
    if (oy < 0) oy = 0;
    ad_gui_size(&cols, &rows);
    *cx = (x - ox) * 100 / (CW * g_zoom);
    *cy = (y - oy) * 100 / (CH * g_zoom);
    if (x < ox) *cx = 0;
    if (y < oy) *cy = 0;
    if (*cx >= cols) *cx = cols - 1;
    if (*cy >= rows) *cy = rows - 1;
    return 1;
}

static void mouse_event_key(int button, int drag, int release, int cx, int cy) {
    AdKey k = blank();
    k.kind = AD_KEY_MOUSE;
    k.mbutton = button;
    k.mdrag = drag;
    k.mrelease = release;
    k.mx = cx;
    k.my = cy;
    push_key(k);
}

static void on_button(HWND hwnd, int button, int down, LPARAM lp) {
    int cx, cy, on;
    EnterCriticalSection(&g_lock);
    on = g_mouse_on;
    LeaveCriticalSection(&g_lock);
    mouse_cell(lp, &cx, &cy);
    if (down) {
        SetCapture(hwnd);
        g_mouse_button = button;
        g_mouse_lx = cx;
        g_mouse_ly = cy;
        if (on) mouse_event_key(button, 0, 0, cx, cy);
    } else {
        if (on) mouse_event_key(button, 0, 1, cx, cy);
        if (button == g_mouse_button) {
            g_mouse_button = -1;
            ReleaseCapture();
        }
    }
}

/* ------------------------------------------------------------ window */

static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_AD_REDRAW:
            InterlockedExchange(&g_posted, 0);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            render();
            paint(hwnd, hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_TIMER:
            if (wp == BLINK_TIMER) {
                int blinks;
                g_blink_phase = !g_blink_phase;
                EnterCriticalSection(&g_lock);
                blinks = !g_ice;
                LeaveCriticalSection(&g_lock);
                if (blinks) InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        case WM_SIZE:
            /* maximizing picks a zoom with room for the toolbox, like full
               screen does; restoring puts the window's own zoom back */
            if (wp == SIZE_MAXIMIZED && !g_fullscreen && !g_max_zoomed) {
                g_windowed_zoom = g_zoom;
                g_zoom = best_zoom_for(LOWORD(lp), HIWORD(lp));
                set_title();
                g_max_zoomed = 1;
            } else if (wp == SIZE_RESTORED && g_max_zoomed && !g_fullscreen) {
                g_zoom = g_windowed_zoom;
                set_title();
                g_max_zoomed = 0;
            }
            if (wp != SIZE_MINIMIZED) on_client_size(LOWORD(lp), HIWORD(lp));
            return 0;
        case WM_GETMINMAXINFO: {
            MINMAXINFO *mm = (MINMAXINFO *)lp;
            int ww, wh;
            if (g_hwnd && !g_fullscreen) {
                client_to_window(ZPX(AD_MIN_COLS * CW), ZPX(AD_MIN_ROWS * CH), &ww, &wh);
                mm->ptMinTrackSize.x = ww;
                mm->ptMinTrackSize.y = wh;
            }
            return 0;
        }
        case WM_CHAR: {
            AdKey k = blank();
            unsigned c = (unsigned)wp;
            if (c == 8 || c == 127) k.kind = AD_KEY_BACKSPACE;
            else if (c == 9) k.kind = AD_KEY_TAB;
            else if (c == 13) k.kind = AD_KEY_ENTER;
            else if (c == 27) k.kind = AD_KEY_ESCAPE;
            else if (c >= 1 && c <= 26) { k.kind = AD_KEY_CTRL; k.ch = (char)('A' + c - 1); }
            else if (c >= 0x20 && c < 0x7F) { k.kind = AD_KEY_CHAR; k.ch = (char)c; }
            else return 0;
            push_key(k);
            return 0;
        }
        case WM_SYSCHAR:
            if (wp == 13) return 0;  /* Alt+Enter: no beep */
            break;
        case WM_SYSKEYDOWN:
            if (wp == VK_RETURN && (lp & (1 << 29))) {
                toggle_fullscreen(hwnd);
                return 0;
            }
            if (wp == VK_F10) {  /* F10 would open the (absent) menu */
                AdKey k = blank();
                k.kind = AD_KEY_FN;
                k.fn = 10;
                push_key(k);
                return 0;
            }
            break;
        case WM_KEYDOWN: {
            AdKey k = blank();
            int ctrl = GetKeyState(VK_CONTROL) < 0;
            if (ctrl && (wp == VK_OEM_PLUS || wp == VK_ADD)) { zoom_step(+1); return 0; }
            if (ctrl && (wp == VK_OEM_MINUS || wp == VK_SUBTRACT)) { zoom_step(-1); return 0; }
            if (ctrl && (wp == '0' || wp == VK_NUMPAD0)) { set_zoom(100); return 0; }
            switch (wp) {
                case VK_UP: k.kind = AD_KEY_UP; break;
                case VK_DOWN: k.kind = AD_KEY_DOWN; break;
                case VK_LEFT: k.kind = AD_KEY_LEFT; break;
                case VK_RIGHT: k.kind = AD_KEY_RIGHT; break;
                case VK_HOME: k.kind = AD_KEY_HOME; break;
                case VK_END: k.kind = AD_KEY_END; break;
                case VK_PRIOR: k.kind = AD_KEY_PGUP; break;
                case VK_NEXT: k.kind = AD_KEY_PGDN; break;
                case VK_INSERT: k.kind = AD_KEY_INSERT; break;
                case VK_DELETE: k.kind = AD_KEY_BACKSPACE; break;  /* see input.h */
                default:
                    if (wp >= VK_F1 && wp <= VK_F12) { k.kind = AD_KEY_FN; k.fn = (int)(wp - VK_F1) + 1; }
                    break;
            }
            if (k.kind != AD_KEY_NONE) { push_key(k); return 0; }
            break;
        }
        case WM_LBUTTONDOWN: on_button(hwnd, 0, 1, lp); return 0;
        case WM_LBUTTONUP:   on_button(hwnd, 0, 0, lp); return 0;
        case WM_MBUTTONDOWN: on_button(hwnd, 1, 1, lp); return 0;
        case WM_MBUTTONUP:   on_button(hwnd, 1, 0, lp); return 0;
        case WM_RBUTTONDOWN: on_button(hwnd, 2, 1, lp); return 0;
        case WM_RBUTTONUP:   on_button(hwnd, 2, 0, lp); return 0;
        case WM_MOUSEMOVE:
            if (g_mouse_button >= 0) {
                int cx, cy, on;
                mouse_cell(lp, &cx, &cy);
                EnterCriticalSection(&g_lock);
                on = g_mouse_on;
                LeaveCriticalSection(&g_lock);
                if (on && (cx != g_mouse_lx || cy != g_mouse_ly))
                    mouse_event_key(g_mouse_button, 1, 0, cx, cy);
                g_mouse_lx = cx;
                g_mouse_ly = cy;
            }
            return 0;
        case WM_MOUSEWHEEL: {
            int delta = (short)HIWORD(wp), on;
            POINT pt;
            if (LOWORD(wp) & MK_CONTROL) { zoom_step(delta > 0 ? 1 : -1); return 0; }
            EnterCriticalSection(&g_lock);
            on = g_mouse_on;
            LeaveCriticalSection(&g_lock);
            if (on) {
                int cx, cy;
                AdKey k = blank();
                pt.x = (short)LOWORD(lp);
                pt.y = (short)HIWORD(lp);
                ScreenToClient(hwnd, &pt);
                mouse_cell(MAKELPARAM(pt.x, pt.y), &cx, &cy);
                k.kind = AD_KEY_MOUSE;
                k.mbutton = -1;
                k.mwheel = delta > 0 ? -1 : 1;
                k.mx = cx;
                k.my = cy;
                push_key(k);
            }
            return 0;
        }
        case WM_CLOSE:
            /* the editor decides: it offers to save first */
            push_kind(AD_KEY_CLOSE);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

typedef struct { int cols, rows; } StartArgs;

static DWORD WINAPI gui_thread(LPVOID arg) {
    StartArgs *a = (StartArgs *)arg;
    WNDCLASSW wc;
    MSG m;
    int ww, wh;
    RECT wa;
    WCHAR wtitle[160];

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = wndproc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wc.hIcon = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"ANetDRAWLocal";
    RegisterClassW(&wc);

    {
        char title[160];
        wsprintfA(title, "ANetDRAW %s (local)  -  Alt+Enter: full screen   Ctrl +/-: zoom", AD_VERSION);
        MultiByteToWideChar(CP_ACP, 0, title, -1, wtitle, 160);
    }
    g_hwnd = CreateWindowExW(0, wc.lpszClassName, wtitle,
                             WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
                             NULL, NULL, wc.hInstance, NULL);
    if (!g_hwnd) {
        SetEvent(g_ready);
        return 1;
    }
    wa = work_area(g_hwnd);
    /* the biggest zoom (100% or more, else less) that fits the grid in
       90% of the screen */
    {
        int i;
        g_zoom = ZOOMS[0];
        for (i = NZOOMS - 1; i >= 0; i--)
            if (fits(ZOOMS[i], (wa.right - wa.left) * 9 / 10, (wa.bottom - wa.top) * 9 / 10, a->cols, a->rows)) {
                g_zoom = ZOOMS[i];
                break;
            }
    }
    set_title();
    client_to_window(ZPX(a->cols * CW), ZPX(a->rows * CH), &ww, &wh);
    SetWindowPos(g_hwnd, NULL, wa.left + ((wa.right - wa.left) - ww) / 2, wa.top + ((wa.bottom - wa.top) - wh) / 2,
                 ww, wh, SWP_NOZORDER);
    {
        RECT rc;
        GetClientRect(g_hwnd, &rc);
        EnterCriticalSection(&g_lock);
        g_cols = rc.right * 100 / (CW * g_zoom);
        g_rows = rc.bottom * 100 / (CH * g_zoom);
        if (g_cols < AD_MIN_COLS) g_cols = AD_MIN_COLS;
        if (g_cols > AD_MAX_COLS) g_cols = AD_MAX_COLS;
        if (g_rows < AD_MIN_ROWS) g_rows = AD_MIN_ROWS;
        if (g_rows > AD_MAX_ROWS) g_rows = AD_MAX_ROWS;
        LeaveCriticalSection(&g_lock);
    }
    SetTimer(g_hwnd, BLINK_TIMER, 450, NULL);
    ShowWindow(g_hwnd, SW_SHOWNORMAL);
    SetForegroundWindow(g_hwnd);
    InterlockedExchange(&g_active, 1);
    SetEvent(g_ready);
    while (GetMessageW(&m, NULL, 0, 0) > 0) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
    return 0;
}

int ad_gui_start(int cols, int rows, int *out_cols, int *out_rows) {
    static StartArgs args;
    HANDLE th;
    /* crisp pixels on high-DPI screens: no bitmap stretching by Windows */
    {
        typedef BOOL (WINAPI *SetAware)(void);
        SetAware f = (SetAware)(void *)GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetProcessDPIAware");
        if (f) f();
    }
    InitializeCriticalSection(&g_lock);
    g_grid = (GCell *)calloc((size_t)AD_MAX_COLS * AD_MAX_ROWS, sizeof(GCell));
    g_shown = (GCell *)calloc((size_t)AD_MAX_COLS * AD_MAX_ROWS, sizeof(GCell));
    g_shown_flags = (unsigned char *)calloc((size_t)AD_MAX_COLS * AD_MAX_ROWS, 1);
    if (!g_grid || !g_shown || !g_shown_flags) return 0;
    {
        size_t i;
        for (i = 0; i < (size_t)AD_MAX_COLS * AD_MAX_ROWS; i++) {
            g_grid[i].ch = ' ';
            g_grid[i].attr = 0x07;
        }
    }
    g_key_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    g_ready = CreateEventW(NULL, TRUE, FALSE, NULL);
    args.cols = cols;
    args.rows = rows;
    th = CreateThread(NULL, 0, gui_thread, &args, 0, NULL);
    if (!th) return 0;
    WaitForSingleObject(g_ready, 10000);
    if (!g_active) return 0;
    CloseHandle(th);
    /* started from Explorer or a shortcut: the console window that came
       with it is empty -- hide it. From a command prompt, leave it be. */
    {
        HWND con = GetConsoleWindow();
        DWORD pids[2];
        if (con && GetConsoleProcessList(pids, 2) <= 1) ShowWindow(con, SW_HIDE);
    }
    ad_gui_size(out_cols, out_rows);
    return 1;
}

#endif /* _WIN32 */
