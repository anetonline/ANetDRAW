/* ZMODEM sender. Frame layout, CRCs and escapes follow the ZMODEM spec as
   implemented by lrzsz (src/zm.c, src/lsz.c) and Synchronet/SyncTERM
   (src/sbbs3/zmodem.c); see include/zmodem.h for the choices made here. */
#include "../include/zmodem.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define ZPAD '*'
#define ZDLE 0x18
#define ZHEX 'B'
#define ZBIN 'A'
#define ZBIN32 'C'
#define XON 0x11
#define XOFF 0x13

/* frame types */
#define ZRQINIT 0
#define ZRINIT 1
#define ZACK 3
#define ZFILE 4
#define ZSKIP 5
#define ZNAK 6
#define ZABORT 7
#define ZFIN 8
#define ZRPOS 9
#define ZDATA 10
#define ZEOF 11
#define ZFERR 12
#define ZCRC 13
#define ZCHALLENGE 14
#define ZCAN 16

/* data subpacket ends */
#define ZCRCE 'h'
#define ZCRCG 'i'
#define ZCRCQ 'j'
#define ZCRCW 'k'
#define ZRUB0 'l'
#define ZRUB1 'm'

/* ZRINIT ZF0 bits */
#define CANFC32 0x20
#define ESCCTL 0x40
/* ZFILE ZF0: binary conversion */
#define ZCBIN 1

/* get_header() results besides a frame type */
#define H_TIMEOUT (-1)
#define H_HANGUP (-2)
#define H_CANCEL (-3)
#define H_GARBAGE (-4)
#define H_BADCRC (-5)

#define BLOCK 1024
#define MAX_ERRORS 10
#define WAIT_MS 10000

typedef struct {
    const AdZmIo *io;
    unsigned char out[8192];
    size_t n;
    int dead;
    int crc32;          /* receiver can check CRC-32 */
    int escctl;         /* receiver wants every control character escaped */
    unsigned bufsize;   /* receiver buffer (0 = full streaming) */
    int peek;           /* one byte read ahead while streaming, or -1 */
    int init;           /* still waiting for a receiver: keys cancel */
    unsigned char hdr[5];
} Zm;

static void zlog(const Zm *z, const char *fmt, ...) {
    char buf[160];
    va_list ap;
    if (!z->io->log) return;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    z->io->log(z->io->ctx, buf);
}

/* ---------------------------------------------------------------- CRCs */

static unsigned short crc16_step(unsigned short crc, unsigned char c) {
    int i;
    crc ^= (unsigned short)(c << 8);
    for (i = 0; i < 8; i++)
        crc = (unsigned short)((crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1);
    return crc;
}

static unsigned long crc32_tab[256];

static void crc32_init(void) {
    unsigned long i, j, c;
    if (crc32_tab[1]) return;
    for (i = 0; i < 256; i++) {
        c = i;
        for (j = 0; j < 8; j++) c = (c & 1) ? 0xEDB88320UL ^ (c >> 1) : c >> 1;
        crc32_tab[i] = c;
    }
}

static unsigned long crc32_step(unsigned long crc, unsigned char c) {
    return (crc32_tab[(crc ^ c) & 0xff] ^ (crc >> 8)) & 0xffffffffUL;
}

/* ---------------------------------------------------------------- output */

static void flush(Zm *z) {
    if (z->n && !z->dead && z->io->send(z->io->ctx, z->out, z->n) != 0) z->dead = 1;
    z->n = 0;
}

static void put_raw(Zm *z, unsigned char c) {
    if (z->n == sizeof(z->out)) flush(z);
    z->out[z->n++] = c;
}

static void put_esc(Zm *z, unsigned char c) {
    switch (c) {
    case ZDLE: case 0x98:
    case 0x10: case 0x90:       /* DLE: some modems and telnet gateways eat it */
    case XON: case XON | 0x80:
    case XOFF: case XOFF | 0x80:
    case '\r': case '\r' | 0x80: /* no bare CR: telnet would pad it with a NUL */
        put_raw(z, ZDLE);
        put_raw(z, (unsigned char)(c ^ 0x40));
        return;
    case 0xff:                  /* no telnet IAC byte either */
        put_raw(z, ZDLE);
        put_raw(z, ZRUB1);
        return;
    default:
        if (z->escctl && (c & 0x60) == 0) {
            put_raw(z, ZDLE);
            put_raw(z, (unsigned char)(c ^ 0x40));
        } else {
            put_raw(z, c);
        }
    }
}

static void put_hex(Zm *z, unsigned char c) {
    static const char digits[] = "0123456789abcdef";
    put_raw(z, (unsigned char)digits[c >> 4]);
    put_raw(z, (unsigned char)digits[c & 15]);
}

static void set_pos(unsigned char *h, int type, unsigned long pos) {
    h[0] = (unsigned char)type;
    h[1] = (unsigned char)(pos & 0xff);
    h[2] = (unsigned char)((pos >> 8) & 0xff);
    h[3] = (unsigned char)((pos >> 16) & 0xff);
    h[4] = (unsigned char)((pos >> 24) & 0xff);
}

static unsigned long get_pos(const unsigned char *h) {
    return (unsigned long)h[1] | (unsigned long)h[2] << 8 |
           (unsigned long)h[3] << 16 | (unsigned long)h[4] << 24;
}

static void send_hex_header(Zm *z, const unsigned char *h) {
    unsigned short crc = 0;
    int i;
    put_raw(z, ZPAD);
    put_raw(z, ZPAD);
    put_raw(z, ZDLE);
    put_raw(z, ZHEX);
    for (i = 0; i < 5; i++) {
        put_hex(z, h[i]);
        crc = crc16_step(crc, h[i]);
    }
    put_hex(z, (unsigned char)(crc >> 8));
    put_hex(z, (unsigned char)(crc & 0xff));
    put_raw(z, '\r');
    put_raw(z, '\n');
    if (h[0] != ZACK && h[0] != ZFIN) put_raw(z, XON);
    flush(z);
}

static void send_bin_header(Zm *z, const unsigned char *h) {
    int i;
    put_raw(z, ZPAD);
    put_raw(z, ZPAD);
    put_raw(z, ZDLE);
    if (z->crc32) {
        unsigned long crc = 0xffffffffUL;
        put_raw(z, ZBIN32);
        for (i = 0; i < 5; i++) {
            crc = crc32_step(crc, h[i]);
            put_esc(z, h[i]);
        }
        crc = ~crc;
        for (i = 0; i < 4; i++) put_esc(z, (unsigned char)((crc >> (8 * i)) & 0xff));
    } else {
        unsigned short crc = 0;
        put_raw(z, ZBIN);
        for (i = 0; i < 5; i++) {
            crc = crc16_step(crc, h[i]);
            put_esc(z, h[i]);
        }
        put_esc(z, (unsigned char)(crc >> 8));
        put_esc(z, (unsigned char)(crc & 0xff));
    }
}

static void send_subpacket(Zm *z, const unsigned char *p, size_t len, int end) {
    size_t i;
    if (z->crc32) {
        unsigned long crc = 0xffffffffUL;
        for (i = 0; i < len; i++) {
            crc = crc32_step(crc, p[i]);
            put_esc(z, p[i]);
        }
        crc = crc32_step(crc, (unsigned char)end);
        put_raw(z, ZDLE);
        put_raw(z, (unsigned char)end);
        crc = ~crc;
        for (i = 0; i < 4; i++) put_esc(z, (unsigned char)((crc >> (8 * i)) & 0xff));
    } else {
        unsigned short crc = 0;
        for (i = 0; i < len; i++) {
            crc = crc16_step(crc, p[i]);
            put_esc(z, p[i]);
        }
        crc = crc16_step(crc, (unsigned char)end);
        put_raw(z, ZDLE);
        put_raw(z, (unsigned char)end);
        put_esc(z, (unsigned char)(crc >> 8));
        put_esc(z, (unsigned char)(crc & 0xff));
    }
    if (end == ZCRCW) put_raw(z, XON);
}

static void send_abort(Zm *z) {
    int i;
    z->n = 0;
    for (i = 0; i < 8; i++) put_raw(z, 0x18);
    for (i = 0; i < 8; i++) put_raw(z, 0x08);
    flush(z);
}

/* ---------------------------------------------------------------- input */

static int get_byte(Zm *z, int ms) {
    int c;
    if (z->peek >= 0) {
        c = z->peek;
        z->peek = -1;
        return c;
    }
    if (z->dead) return -2;
    c = z->io->recv(z->io->ctx, ms);
    if (c == -2) z->dead = 1;
    return c;
}

/* A ZDLE-decoded byte of a binary header; negative H_* on trouble. */
static int zdl_byte(Zm *z, int ms) {
    int c, cans = 0;
    for (;;) {
        c = get_byte(z, ms);
        if (c < 0) return c == -2 ? H_HANGUP : H_TIMEOUT;
        if (c == XON || c == XOFF || c == (XON | 0x80) || c == (XOFF | 0x80)) continue;
        if (c != ZDLE) return c;
        for (;;) {
            c = get_byte(z, ms);
            if (c < 0) return c == -2 ? H_HANGUP : H_TIMEOUT;
            if (c == ZDLE) {
                if (++cans >= 4) return H_CANCEL;  /* the fifth CAN in a row */
                continue;
            }
            if (c == XON || c == XOFF || c == (XON | 0x80) || c == (XOFF | 0x80)) continue;
            if (c == ZRUB0) return 0x7f;
            if (c == ZRUB1) return 0xff;
            if ((c & 0x60) == 0x40) return c ^ 0x40;
            return H_GARBAGE;
        }
    }
}

static int hex_nibble(int c) {
    c &= 0x7f;
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* Waits up to ms between bytes for the next header from the receiver.
   Returns its frame type (header bytes in z->hdr) or a negative H_*. */
static int get_header_raw(Zm *z, int ms) {
    int c, i, cans = 0, pads = 0, junk = 0;
    for (;;) {
        c = get_byte(z, ms);
        if (c < 0) return c == -2 ? H_HANGUP : H_TIMEOUT;
        if (c == ZPAD || c == (ZPAD | 0x80)) {
            pads++;
            cans = 0;
            continue;
        }
        if (c == ZDLE && pads) break;
        if (c == ZDLE) {
            if (++cans >= 5) return H_CANCEL;
            continue;
        }
        /* keys a caller might press at a terminal that never answered */
        if (z->init && (c == 0x1b || c == 0x03)) return H_CANCEL;
        cans = pads = 0;
        if (++junk > 1400) return H_GARBAGE;
    }
    c = get_byte(z, ms);
    if (c < 0) return c == -2 ? H_HANGUP : H_TIMEOUT;
    c &= 0x7f;
    if (c == ZHEX) {
        unsigned short crc = 0;
        unsigned char b[7];
        for (i = 0; i < 7; i++) {
            int hi, lo;
            if ((c = get_byte(z, ms)) < 0) return c == -2 ? H_HANGUP : H_TIMEOUT;
            hi = hex_nibble(c);
            if ((c = get_byte(z, ms)) < 0) return c == -2 ? H_HANGUP : H_TIMEOUT;
            lo = hex_nibble(c);
            if (hi < 0 || lo < 0) return H_GARBAGE;
            b[i] = (unsigned char)(hi << 4 | lo);
        }
        for (i = 0; i < 5; i++) crc = crc16_step(crc, b[i]);
        if (crc != (unsigned short)(b[5] << 8 | b[6])) return H_BADCRC;
        memcpy(z->hdr, b, 5);
        return z->hdr[0];
    }
    if (c == ZBIN || c == ZBIN32) {
        int want = c == ZBIN32 ? 9 : 7;
        int b[9];
        for (i = 0; i < want; i++)
            if ((b[i] = zdl_byte(z, ms)) < 0) return b[i];
        if (c == ZBIN32) {
            unsigned long crc = 0xffffffffUL, got;
            for (i = 0; i < 5; i++) crc = crc32_step(crc, (unsigned char)b[i]);
            got = (unsigned long)b[5] | (unsigned long)b[6] << 8 |
                  (unsigned long)b[7] << 16 | (unsigned long)b[8] << 24;
            if ((~crc & 0xffffffffUL) != got) return H_BADCRC;
        } else {
            unsigned short crc = 0;
            for (i = 0; i < 5; i++) crc = crc16_step(crc, (unsigned char)b[i]);
            if (crc != (unsigned short)(b[5] << 8 | b[6])) return H_BADCRC;
        }
        for (i = 0; i < 5; i++) z->hdr[i] = (unsigned char)b[i];
        return z->hdr[0];
    }
    return H_GARBAGE;
}

static int get_header(Zm *z, int ms) {
    static const char *const NAMES[] = { "ZRQINIT", "ZRINIT", "ZSINIT", "ZACK", "ZFILE", "ZSKIP",
        "ZNAK", "ZABORT", "ZFIN", "ZRPOS", "ZDATA", "ZEOF", "ZFERR", "ZCRC", "ZCHALLENGE",
        "ZCOMPL", "ZCAN", "ZFREECNT", "ZCOMMAND", "ZSTDERR" };
    static const char *const ERRS[] = { "timeout", "hangup", "cancel", "garbage", "bad CRC" };
    int t = get_header_raw(z, ms);
    if (t >= 0 && t < (int)(sizeof(NAMES) / sizeof(NAMES[0])))
        zlog(z, "rx %s pos/flags %02x %02x %02x %02x", NAMES[t], z->hdr[1], z->hdr[2], z->hdr[3], z->hdr[4]);
    else if (t < 0 && t >= -5)
        zlog(z, "rx %s (waited up to %d ms)", ERRS[-t - 1], ms);
    else
        zlog(z, "rx frame type %d", t);
    return t;
}

/* ---------------------------------------------------------------- session */

static int fail_code(Zm *z, int h) {
    if (z->dead || h == H_HANGUP) return AD_ZM_HANGUP;
    if (h == H_CANCEL) return AD_ZM_CANCELLED;
    return AD_ZM_FAILED;
}

/* Streams one file from pos. Returns AD_ZM_OK once the receiver takes the
   ZEOF (by asking for the next file), or an error code. */
static int send_data(Zm *z, const AdZmFile *f, unsigned long pos) {
    unsigned char h[5];
    int errors = 0;
    unsigned long since_ack = 0;
    unsigned long len = (unsigned long)f->len;

restart:
    if (errors > MAX_ERRORS) return AD_ZM_FAILED;
    if (pos > len) pos = len;
    since_ack = 0;
    set_pos(h, ZDATA, pos);
    send_bin_header(z, h);
    if (len == 0) send_subpacket(z, NULL, 0, ZCRCE);
    while (pos < len) {
        size_t n = len - pos > BLOCK ? BLOCK : (size_t)(len - pos);
        int end;
        if (pos + n >= len) end = ZCRCE;
        else if (z->bufsize && since_ack + n + BLOCK > z->bufsize) end = ZCRCW;
        else end = ZCRCG;
        send_subpacket(z, f->data + pos, n, end);
        pos += n;
        since_ack += n;
        if (z->io->progress) z->io->progress(z->io->ctx, (long)pos, (long)len);
        if (end == ZCRCW) {
            int t;
            flush(z);
            for (;;) {
                t = get_header(z, WAIT_MS);
                if (t == ZACK) { since_ack = 0; break; }
                if (t == ZRPOS) { pos = get_pos(z->hdr); errors++; goto restart; }
                if (t == H_CANCEL || t == H_HANGUP || z->dead) return fail_code(z, t);
                if (t == H_TIMEOUT || ++errors > MAX_ERRORS) return AD_ZM_FAILED;
            }
        } else if (end == ZCRCG) {
            /* anything from the receiver mid-stream is a header */
            int c = get_byte(z, 0);
            if (c == -2) return AD_ZM_HANGUP;
            if (c == ZPAD || c == ZDLE) {
                int t;
                z->peek = c;
                flush(z);
                t = get_header(z, 2000);
                if (t == ZRPOS) {
                    pos = get_pos(z->hdr);
                    errors++;
                    goto restart;
                }
                if (t == H_CANCEL || t == H_HANGUP) return fail_code(z, t);
                if (t == ZSKIP) return AD_ZM_SKIPPED;
                /* ZACK or noise: keep streaming */
            }
        }
        if (z->dead) return AD_ZM_HANGUP;
    }
    flush(z);

    for (;;) {
        int t;
        set_pos(h, ZEOF, len);
        send_bin_header(z, h);
        flush(z);
        for (;;) {
            t = get_header(z, WAIT_MS);
            if (t == ZACK) continue;  /* a late ack of a ZCRCW */
            break;
        }
        if (t == ZRINIT) return AD_ZM_OK;
        if (t == ZRPOS) { pos = get_pos(z->hdr); errors++; goto restart; }
        if (t == ZSKIP) return AD_ZM_SKIPPED;
        if (t == H_CANCEL || t == H_HANGUP || z->dead) return fail_code(z, t);
        if (t == ZFERR || t == ZABORT || t == ZFIN) return AD_ZM_FAILED;
        if (++errors > MAX_ERRORS) return AD_ZM_FAILED;
    }
}

static int send_file(Zm *z, const AdZmFile *f, int files_left, long bytes_left) {
    unsigned char h[5], info[400];
    int errors = 0, n;
    size_t name_len = strlen(f->name);
    if (name_len > 255) name_len = 255;

    memcpy(info, f->name, name_len);
    info[name_len] = 0;
    n = snprintf((char *)info + name_len + 1, sizeof(info) - name_len - 1,
                 "%ld %lo %o 0 %d %ld", f->len, (unsigned long)(f->mtime > 0 ? f->mtime : 0),
                 0100644, files_left, bytes_left);
    if (n < 0) n = 0;

    for (;;) {
        int t, quiet = 0;
        memset(h, 0, sizeof(h));
        h[0] = ZFILE;
        h[4] = ZCBIN;  /* ZF0 */
        send_bin_header(z, h);
        send_subpacket(z, info, name_len + 1 + (size_t)n + 1, ZCRCW);
        flush(z);
        for (;;) {
            t = get_header(z, WAIT_MS);
            /* A quiet receiver may be asking its user something (SyncTERM:
               "Duplicate file ... Overwrite?"). Resending ZFILE meanwhile
               only queues up stale frames, so just keep waiting, up to
               two minutes. */
            if (t == H_TIMEOUT && ++quiet < 12) continue;
            if (t == ZCRC) {  /* the receiver wants the file's CRC (to resume) */
                unsigned long crc = 0xffffffffUL;
                long i;
                for (i = 0; i < f->len; i++) crc = crc32_step(crc, f->data[i]);
                set_pos(h, ZCRC, ~crc & 0xffffffffUL);
                send_hex_header(z, h);
                continue;
            }
            break;
        }
        if (t == ZRPOS) return send_data(z, f, get_pos(z->hdr));
        if (t == ZSKIP) return AD_ZM_SKIPPED;
        if (t == H_CANCEL || t == H_HANGUP || z->dead) return fail_code(z, t);
        if (t == ZFERR || t == ZABORT || t == ZFIN) return AD_ZM_FAILED;
        /* ZRINIT again, ZNAK, a bad CRC or a timeout: send ZFILE again */
        if (++errors > MAX_ERRORS) return AD_ZM_FAILED;
    }
}

int ad_zm_send(const AdZmIo *io, const AdZmFile *files, int nfiles, int *files_done) {
    Zm z;
    unsigned char h[5];
    int i, t = H_TIMEOUT, tries, rc = AD_ZM_OK, done = 0;
    long bytes_left = 0;

    crc32_init();
    memset(&z, 0, sizeof(z));
    z.io = io;
    z.peek = -1;
    z.init = 1;
    if (files_done) *files_done = 0;
    for (i = 0; i < nfiles; i++) bytes_left += files[i].len;

    /* ZRQINIT until a receiver answers with ZRINIT */
    for (tries = 0; tries < 4; tries++) {
        memset(h, 0, sizeof(h));
        send_hex_header(&z, h);
        for (;;) {
            t = get_header(&z, 5000);
            if (t == ZCHALLENGE) {
                h[0] = ZACK;
                memcpy(h + 1, z.hdr + 1, 4);
                send_hex_header(&z, h);
                continue;
            }
            if (t == ZRINIT || t < 0) break;
            /* ZRQINIT echoed back, ZCOMMAND, ...: keep listening */
        }
        if (t == ZRINIT || t == H_CANCEL || t == H_HANGUP || z.dead) break;
    }
    if (t != ZRINIT) {
        rc = t == H_CANCEL ? AD_ZM_CANCELLED : (t == H_HANGUP || z.dead) ? AD_ZM_HANGUP : AD_ZM_NO_RECEIVER;
        if (rc != AD_ZM_HANGUP) send_abort(&z);
        return rc;
    }
    z.init = 0;
    z.crc32 = (z.hdr[4] & CANFC32) != 0;
    z.escctl = (z.hdr[4] & ESCCTL) != 0;
    z.bufsize = (unsigned)(z.hdr[1] | z.hdr[2] << 8);

    for (i = 0; i < nfiles; i++) {
        int r = send_file(&z, &files[i], nfiles - i, bytes_left);
        bytes_left -= files[i].len;
        if (r == AD_ZM_OK) done++;
        else if (r == AD_ZM_SKIPPED) rc = AD_ZM_SKIPPED;
        else {
            if (r != AD_ZM_HANGUP) send_abort(&z);
            if (files_done) *files_done = done;
            return r;
        }
    }

    /* ZFIN, the receiver's ZFIN, then "OO" (over and out) */
    for (tries = 0; tries < 3; tries++) {
        memset(h, 0, sizeof(h));
        h[0] = ZFIN;
        send_hex_header(&z, h);
        t = get_header(&z, 5000);
        if (t == ZFIN || t == H_HANGUP || t == H_CANCEL) break;
    }
    if (t == ZFIN) {
        put_raw(&z, 'O');
        put_raw(&z, 'O');
        flush(&z);
    }
    if (files_done) *files_done = done;
    if (z.dead) return done == nfiles ? AD_ZM_OK : AD_ZM_HANGUP;
    return rc;
}

const char *ad_zm_result_text(int rc) {
    switch (rc) {
    case AD_ZM_OK: return "Download complete";
    case AD_ZM_NO_RECEIVER: return "No ZMODEM answer (does your terminal support ZMODEM?)";
    case AD_ZM_CANCELLED: return "Download cancelled";
    case AD_ZM_SKIPPED: return "Your terminal skipped the file (already there?)";
    case AD_ZM_HANGUP: return "Connection lost";
    default: return "Download failed";
    }
}
