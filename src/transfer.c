#include "../include/transfer.h"
#include "../include/input.h"
#include "OpenDoor.h"

static int od_send(void *ctx, const unsigned char *buf, size_t len) {
    (void)ctx;
    while (len > 0) {
        INT chunk = len > 4096 ? 4096 : (INT)len;
        od_disp((const char *)buf, chunk, FALSE);
        if (!od_carrier()) return -1;
        buf += chunk;
        len -= (size_t)chunk;
    }
    return 0;
}

static int od_recv(void *ctx, int ms) {
    tODInputEvent ev;
    (void)ctx;
    for (;;) {
        if (!od_carrier()) return -2;
        /* GETIN_RAW: no escape-sequence matching, every byte as it came */
        if (!od_get_input(&ev, (tODMilliSec)ms, GETIN_RAW)) return -1;
        if (ev.EventType == EVENT_CHARACTER) return (unsigned char)ev.chKeyPress;
        /* a local sysop key during a remote transfer: ignore it */
    }
}

static void zm_log(void *ctx, const char *msg) {
    (void)ctx;
    ad_trace("ZMODEM %s", msg);
}

int ad_xfer_send(const AdDoor *door, const AdZmFile *files, int n, int *done) {
    AdZmIo io;
    tODInputEvent ev;
    BOOL utf8 = od_control.od_cp437_to_utf8_out;
    int rc, i;

    if (door->mouse) ad_dout("\x1b[?1002l\x1b[?1006l");
    ad_input_flush();
    od_control.od_cp437_to_utf8_out = FALSE;  /* file bytes go out as they are */

    io.ctx = NULL;
    io.send = od_send;
    io.recv = od_recv;
    io.progress = NULL;  /* the terminal draws its own transfer window */
    io.log = zm_log;
    ad_trace("ZMODEM start: %d file(s), first %s (%ld bytes)", n, n ? files[0].name : "-",
             n ? files[0].len : 0L);
    rc = ad_zm_send(&io, files, n, done);
    ad_trace("ZMODEM end: rc=%d (%s), %d done", rc, ad_zm_result_text(rc), done ? *done : -1);

    od_control.od_cp437_to_utf8_out = utf8;
    /* leftovers: the receiver's last ZFIN, cancel bytes, stray mouse reports */
    for (i = 0; i < 200 && od_carrier() && od_get_input(&ev, 300, GETIN_RAW); i++) {}
    ad_input_flush();
    if (door->mouse) ad_dout("\x1b[?1002h\x1b[?1006h");
    return rc;
}
