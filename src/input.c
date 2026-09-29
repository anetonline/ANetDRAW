#include "../include/input.h"
#include "../include/anetdraw.h"
#include "OpenDoor.h"
#include "../include/wingui.h"
#include <signal.h>
#include <string.h>

/* How long one od_get_input() call waits before the carrier and the
   resize flag are rechecked. Short enough that a hung-up caller's
   process exits promptly and a resized window re-lays-out without a
   visible lag, long enough that an idle editor costs nothing. */
#define INPUT_POLL_MS 200

static volatile sig_atomic_t g_resized = 0;

void ad_input_note_resize(void) { g_resized = 1; }

/* A mouse report arrives as one burst, so its bytes follow the ESC
   almost instantly; a person pressing Esc then '[' can't be this fast. */
#define MOUSE_BYTE_MS 60

/* Previous raw character, so a CR LF pair (some telnet clients send
   Enter that way) counts as one Enter, not two. */
static unsigned char g_last_raw = 0;

/* Keys read ahead while checking whether an ESC started a mouse
   report, handed out before anything new is read. */
#define QUEUE_LEN 32
static AdKey g_queue[QUEUE_LEN];
static int g_qhead = 0, g_qlen = 0;

static void enqueue(AdKey k) {
    if (g_qlen < QUEUE_LEN) {
        g_queue[(g_qhead + g_qlen) % QUEUE_LEN] = k;
        g_qlen++;
    }
}

static AdKey blank_key(void) {
    AdKey k;
    memset(&k, 0, sizeof(k));
    return k;
}

/* One OpenDoors input event -> one editor key. Returns 0 for events
   the editor ignores. */
static int translate(const tODInputEvent *ev, AdKey *k) {
    *k = blank_key();
    if (ev->EventType == EVENT_EXTENDED_KEY) {
        unsigned char x = (unsigned char)ev->chKeyPress;
        g_last_raw = 0;
        switch (x) {
            case OD_KEY_UP:     k->kind = AD_KEY_UP; return 1;
            case OD_KEY_DOWN:   k->kind = AD_KEY_DOWN; return 1;
            case OD_KEY_LEFT:   k->kind = AD_KEY_LEFT; return 1;
            case OD_KEY_RIGHT:  k->kind = AD_KEY_RIGHT; return 1;
            case OD_KEY_HOME:   k->kind = AD_KEY_HOME; return 1;
            case OD_KEY_END:    k->kind = AD_KEY_END; return 1;
            case OD_KEY_PGUP:   k->kind = AD_KEY_PGUP; return 1;
            case OD_KEY_PGDN:   k->kind = AD_KEY_PGDN; return 1;
            case OD_KEY_INSERT: k->kind = AD_KEY_INSERT; return 1;
            case OD_KEY_DELETE: k->kind = AD_KEY_BACKSPACE; return 1;
            /* 0x01 = the PC scan code for Esc. A Windows console key
               event with AsciiChar 0 becomes an "extended key" of
               its scan code (ODConsole.c) -- and Wine's console
               delivers Esc that way (vk 27, char 0; confirmed with
               a ReadConsoleInput probe), so without this the local
               Esc key does nothing. */
            case 0x01:          k->kind = AD_KEY_ESCAPE; return 1;
            case OD_KEY_F11:    k->kind = AD_KEY_FN; k->fn = 11; return 1;
            case OD_KEY_F12:    k->kind = AD_KEY_FN; k->fn = 12; return 1;
            default:
                if (x >= OD_KEY_F1 && x <= OD_KEY_F10) {
                    k->kind = AD_KEY_FN;
                    k->fn = x - OD_KEY_F1 + 1;
                    return 1;
                }
                return 0; /* unmapped extended key -- ignore */
        }
    }
    {
        unsigned char c = (unsigned char)ev->chKeyPress;
        unsigned char prev = g_last_raw;
        g_last_raw = c;
        if (c == 0x0a && prev == 0x0d) return 0;
        if (c == 0x1b) { k->kind = AD_KEY_ESCAPE; return 1; }
        if (c == 0x0d || c == 0x0a) { k->kind = AD_KEY_ENTER; return 1; }
        if (c == 0x08 || c == 0x7f) { k->kind = AD_KEY_BACKSPACE; return 1; }
        if (c == 0x09) { k->kind = AD_KEY_TAB; return 1; }
        if (c >= 1 && c <= 26) { k->kind = AD_KEY_CTRL; k->ch = (char)('A' + c - 1); return 1; }
        if (c >= 0x20 && c < 0x7f) { k->kind = AD_KEY_CHAR; k->ch = (char)c; return 1; }
        return 0; /* anything else (high bytes, stray NULs) -- ignore */
    }
}

/* Next event within ms, as a plain character if it is one. Returns -1
   on timeout; for a non-character event, stores it in *other and
   returns -2. */
static int next_char(int ms, tODInputEvent *other) {
    tODInputEvent ev;
    if (!od_get_input(&ev, ms, GETIN_RAWCTRL)) return -1;
    if (ev.EventType != EVENT_CHARACTER) { *other = ev; return -2; }
    return (unsigned char)ev.chKeyPress;
}

/* Hands back whatever was read ahead during a failed mouse parse, as
   ordinary keys, so nothing the caller typed is lost. */
static void requeue_chars(const int *chars, int n, const tODInputEvent *tail) {
    int i;
    AdKey k;
    tODInputEvent ev;
    memset(&ev, 0, sizeof(ev));
    ev.EventType = EVENT_CHARACTER;
    for (i = 0; i < n; i++) {
        ev.chKeyPress = (char)chars[i];
        if (translate(&ev, &k)) enqueue(k);
    }
    if (tail && translate(tail, &k)) enqueue(k);
}

/* Called right after a raw ESC. OpenDoors has no entry for SGR mouse
   reports (CSI < b ; x ; y M/m -- DECSET 1006, confirmed in SyncTERM's
   cterm.adoc and xterm's ctlseqs), so it hands back ESC, '[' and '<'
   as separate characters; this reassembles them. Returns 1 with *k
   filled for a mouse event, 0 if the ESC was just an ESC (anything
   read ahead is queued). */
static int try_mouse(AdKey *k) {
    int got[24], n = 0, c, field = 0;
    int vals[3] = { 0, 0, 0 };
    tODInputEvent other;

    c = next_char(MOUSE_BYTE_MS, &other);
    if (c == -1) return 0;
    if (c == -2) { requeue_chars(got, 0, &other); return 0; }
    got[n++] = c;
    if (c != '[') { requeue_chars(got, n, NULL); return 0; }

    c = next_char(MOUSE_BYTE_MS, &other);
    if (c == -1) { requeue_chars(got, n, NULL); return 0; }
    if (c == -2) { requeue_chars(got, n, &other); return 0; }
    got[n++] = c;
    if (c != '<') { requeue_chars(got, n, NULL); return 0; }

    for (;;) {
        c = next_char(MOUSE_BYTE_MS, &other);
        if (c == -1) { requeue_chars(got, n, NULL); return 0; }
        if (c == -2) { requeue_chars(got, n, &other); return 0; }
        if (n >= (int)(sizeof(got) / sizeof(got[0]))) { requeue_chars(got, n, NULL); return 0; }
        got[n++] = c;
        if (c >= '0' && c <= '9') {
            if (vals[field] < 100000) vals[field] = vals[field] * 10 + (c - '0');
        } else if (c == ';' && field < 2) {
            field++;
        } else if ((c == 'M' || c == 'm') && field == 2) {
            int b = vals[0];
            *k = blank_key();
            k->kind = AD_KEY_MOUSE;
            k->mx = vals[1] - 1;          /* reports are 1-based */
            k->my = vals[2] - 1;
            k->mrelease = (c == 'm');
            k->mdrag = (b & 32) != 0;
            k->mwheel = (b & 64) ? ((b & 1) ? 1 : -1) : 0;
            k->mbutton = (b & 64) ? -1 : (b & 3);
            return 1;
        } else {
            requeue_chars(got, n, NULL);
            return 0;
        }
    }
}

void ad_input_flush(void) {
    g_qhead = g_qlen = 0;
#ifdef _WIN32
    if (ad_gui_active()) ad_gui_flush_input();
#endif
    od_clear_keybuffer();
}

int ad_input_pending(void) {
#ifdef _WIN32
    if (ad_gui_active()) return g_qlen > 0 || ad_gui_pending();
#endif
    return g_qlen > 0 || od_key_pending();
}

AdKey ad_input_get(int local) {
    return ad_input_get_timeout(local, -1);
}

AdKey ad_input_get_timeout(int local, int ms) {
    AdKey k;
    tODInputEvent ev;
    int left = ms;

    if (g_qlen > 0) {
        k = g_queue[g_qhead];
        g_qhead = (g_qhead + 1) % QUEUE_LEN;
        g_qlen--;
        return k;
    }

#ifdef _WIN32
    /* the local window hands over finished keys (see wingui.h) */
    if (ad_gui_active()) {
        while (!ad_gui_get_key(&k, ms >= 0 && ms < INPUT_POLL_MS ? ms : INPUT_POLL_MS)) {
            if (ms >= 0 && (left -= INPUT_POLL_MS) <= 0) {
                k = blank_key();
                return k;  /* AD_KEY_NONE: the wait ran out */
            }
        }
        return k;
    }
#endif

    for (;;) {
        if (g_resized) {
            g_resized = 0;
            k = blank_key();
            k.kind = AD_KEY_RESIZE;
            return k;
        }
        if (!local && !od_carrier()) {
            k = blank_key();
            k.kind = AD_KEY_HANGUP;
            return k;
        }
        if (!od_get_input(&ev, ms >= 0 && left < INPUT_POLL_MS ? (left > 0 ? left : 0) : INPUT_POLL_MS,
                          GETIN_RAWCTRL)) {
            if (ms >= 0 && (left -= INPUT_POLL_MS) <= 0) {
                k = blank_key();
                return k;  /* AD_KEY_NONE: the wait ran out */
            }
            continue;
        }
        if (!translate(&ev, &k)) {
            ad_trace("IN ignored %s 0x%02X", ev.EventType == EVENT_EXTENDED_KEY ? "ext" : "chr",
                     (unsigned char)ev.chKeyPress);
            continue;
        }
        if (k.kind == AD_KEY_ESCAPE && ev.EventType == EVENT_CHARACTER) {
            AdKey m;
            if (try_mouse(&m)) {
                ad_trace("IN mouse b=%d x=%d y=%d%s%s", m.mbutton, m.mx, m.my,
                         m.mdrag ? " drag" : "", m.mrelease ? " release" : "");
                return m;
            }
        }
        ad_trace("IN key kind=%d ch=0x%02X fn=%d", (int)k.kind, (unsigned char)k.ch, k.fn);
        return k;
    }
}
