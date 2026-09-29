#define _GNU_SOURCE
/* ZMODEM sender test against a real receiver (lrzsz's rz), over a
 * socketpair. Checks byte-exact delivery of awkward files (every byte
 * value, ZDLE runs, CR, 0xFF, empty, multi-block), recovery from a line
 * error, the caller cancelling, and that the sent stream never holds a
 * telnet IAC (0xFF) or a CR that isn't part of a hex header's CR LF.
 *
 *   cc -O1 -g -fsanitize=address,undefined -o test_zmodem \
 *      tests/test_zmodem.c src/zmodem.c
 *   ./test_zmodem /path/to/lrz
 */
#include "../include/zmodem.h"
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } \
                           else { printf("PASS: " __VA_ARGS__); printf("\n"); } } while (0)

typedef struct {
    int fd;
    unsigned char *log;     /* everything the sender wrote */
    size_t nlog, cap;
    long corrupt_at;        /* flip one byte at this output offset (-1 = never) */
    long corrupt_in_at;     /* flip one received byte at this offset (-1 = never) */
    long nin;
    int swap_del;           /* a BBS swapping Delete/Backspace: 0x7f arrives as 0x08 */
    int progress_calls;
} Link;

static int link_send(void *ctx, const unsigned char *buf, size_t len) {
    Link *l = (Link *)ctx;
    unsigned char tmp[16384];
    size_t off = 0;
    if (l->nlog + len > l->cap) {
        l->cap = (l->nlog + len) * 2;
        l->log = (unsigned char *)realloc(l->log, l->cap);
    }
    memcpy(l->log + l->nlog, buf, len);
    memcpy(tmp, buf, len);
    if (l->corrupt_at >= 0 && (size_t)l->corrupt_at >= l->nlog && (size_t)l->corrupt_at < l->nlog + len) {
        tmp[l->corrupt_at - (long)l->nlog] ^= 0x01;
        l->corrupt_at = -1;
    }
    l->nlog += len;
    while (off < len) {
        ssize_t w = write(l->fd, tmp + off, len - off);
        if (w < 0) { if (errno == EINTR) continue; return -1; }
        off += (size_t)w;
    }
    return 0;
}

static int link_recv(void *ctx, int ms) {
    Link *l = (Link *)ctx;
    struct pollfd p;
    unsigned char c;
    p.fd = l->fd;
    p.events = POLLIN;
    if (poll(&p, 1, ms) <= 0) return -1;
    if (read(l->fd, &c, 1) != 1) return -2;
    if (l->corrupt_in_at >= 0 && l->nin == l->corrupt_in_at) c ^= 0x04;
    if (l->swap_del && c == 0x7f) c = '\b';
    l->nin++;
    return c;
}

static void link_progress(void *ctx, long sent, long total) {
    (void)sent; (void)total;
    ((Link *)ctx)->progress_calls++;
}

static unsigned char *slurp(const char *path, long *len) {
    FILE *f = fopen(path, "rb");
    unsigned char *b;
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    *len = ftell(f);
    fseek(f, 0, SEEK_SET);
    b = (unsigned char *)malloc((size_t)*len + 1);
    if (fread(b, 1, (size_t)*len, f) != (size_t)*len) { free(b); b = NULL; }
    fclose(f);
    return b;
}

/* Runs rz in dir on one end of a socketpair and sends files down the other. */
static int run(const char *rz, const char *dir, const AdZmFile *files, int n, Link *l, int *done,
               const char *opt1, const char *opt2) {
    int sv[2], rc, st;
    pid_t pid;
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) return -1;
    pid = fork();
    if (pid == 0) {
        int fd = open("/dev/null", 1);
        dup2(sv[1], 0);
        dup2(sv[1], 1);
        if (fd >= 0) dup2(fd, 2);
        close(sv[0]);
        if (chdir(dir) != 0) _exit(98);
        execl(rz, rz, "-y", "-q", opt1, opt2, (char *)NULL);
        _exit(99);
    }
    close(sv[1]);
    l->fd = sv[0];
    {
        AdZmIo io;
        io.ctx = l;
        io.send = link_send;
        io.recv = link_recv;
        io.progress = link_progress;
        io.log = NULL;
        rc = ad_zm_send(&io, files, n, done);
    }
    close(sv[0]);
    waitpid(pid, &st, 0);
    return rc;
}

/* In the sent stream, 0xFF never appears and every CR is a hex header's CR LF. */
static int stream_is_telnet_safe(const Link *l) {
    size_t i;
    for (i = 0; i < l->nlog; i++) {
        if (l->log[i] == 0xff) return 0;
        if (l->log[i] == '\r' && (i + 1 >= l->nlog || l->log[i + 1] != '\n')) return 0;
    }
    return 1;
}

static int same_file(const char *dir, const AdZmFile *f) {
    char path[512];
    long len = 0;
    unsigned char *got;
    int ok;
    snprintf(path, sizeof(path), "%s/%s", dir, f->name);
    got = slurp(path, &len);
    ok = got && len == f->len && memcmp(got, f->data, (size_t)len) == 0;
    if (!ok) printf("    %s: got %ld bytes, want %ld\n", f->name, got ? len : -1, f->len);
    free(got);
    return ok;
}

int main(int argc, char **argv) {
    const char *rz = argc > 1 ? argv[1] : "rz";
    char dir[] = "/tmp/anetdraw_zm_XXXXXX";
    static unsigned char every[5000], zdles[3000], big[70001];
    const char *ans = "\x1b[0;1;31mHi\x1b[0m\r\n\x1a" "SAUCE00 art";
    AdZmFile files[5];
    Link l;
    int rc, done, i;
    unsigned seed = 7;

    signal(SIGPIPE, SIG_IGN);
    if (!mkdtemp(dir)) return 2;
    for (i = 0; i < (int)sizeof(every); i++) every[i] = (unsigned char)(i * 7 + i / 256);
    for (i = 0; i < (int)sizeof(zdles); i++) {
        static const unsigned char nasty[] = { 0x18, 0x11, 0x13, 0x0d, 0xff, 0x10, 0x7f, 0x8d, 0x98, '*', 0 };
        zdles[i] = nasty[i % sizeof(nasty)];
    }
    for (i = 0; i < (int)sizeof(big); i++) { seed = seed * 1103515245u + 12345u; big[i] = (unsigned char)(seed >> 16); }

    files[0].name = "every_byte.bin"; files[0].data = every; files[0].len = sizeof(every);
    files[1].name = "escapes.bin"; files[1].data = zdles; files[1].len = sizeof(zdles);
    files[2].name = "smiley.ans"; files[2].data = (const unsigned char *)ans; files[2].len = (long)strlen(ans);
    files[3].name = "empty.ans"; files[3].data = (const unsigned char *)""; files[3].len = 0;
    files[4].name = "big.bin"; files[4].data = big; files[4].len = sizeof(big);
    for (i = 0; i < 5; i++) files[i].mtime = 1700000000L;

    memset(&l, 0, sizeof(l));
    l.corrupt_at = -1;
        l.corrupt_in_at = -1;
    rc = run(rz, dir, files, 5, &l, &done, (char *)NULL, (char *)NULL);
    CHECK(rc == AD_ZM_OK && done == 5, "batch of 5 files sent (rc=%d, %s, done=%d)", rc, ad_zm_result_text(rc), done);
    for (i = 0; i < 5; i++) CHECK(same_file(dir, &files[i]), "%s arrives byte for byte", files[i].name);
    CHECK(stream_is_telnet_safe(&l), "sent stream has no 0xFF and no bare CR (%zu bytes)", l.nlog);
    CHECK(l.progress_calls > 70, "progress reported per block (%d calls)", l.progress_calls);
    free(l.log);

    /* a line error in the middle of the big file: rz asks for a resend */
    {
        char dir2[] = "/tmp/anetdraw_zm_XXXXXX";
        if (!mkdtemp(dir2)) return 2;
        memset(&l, 0, sizeof(l));
        l.corrupt_at = 40000;
        rc = run(rz, dir2, &files[4], 1, &l, &done, (char *)NULL, (char *)NULL);
        CHECK(rc == AD_ZM_OK && done == 1, "a corrupted byte is recovered (rc=%d)", rc);
        CHECK(same_file(dir2, &files[4]), "big.bin still arrives intact after the resend");
        CHECK(l.nlog > 71000 + 1024, "the sender resent from the receiver's ZRPOS (%zu bytes out)", l.nlog);
        free(l.log);
    }

    /* a receiver that wants every control character escaped (rz -e),
       and one that injects its own CRC errors every 20000 bytes */
    {
        char dir3[] = "/tmp/anetdraw_zm_XXXXXX", dir4[] = "/tmp/anetdraw_zm_XXXXXX";
        size_t k;
        int bare_ctl = 0;
        if (!mkdtemp(dir3) || !mkdtemp(dir4)) return 2;
        memset(&l, 0, sizeof(l));
        l.corrupt_at = -1;
        l.corrupt_in_at = -1;
        rc = run(rz, dir3, files, 5, &l, &done, "-e", (char *)NULL);
        CHECK(rc == AD_ZM_OK && done == 5, "ESCCTL receiver: batch sent (rc=%d)", rc);
        for (i = 0; i < 5; i++) CHECK(same_file(dir3, &files[i]), "ESCCTL: %s intact", files[i].name);
        /* past the first hex header, only CR LF XON of later hex headers may be raw controls */
        for (k = 0; k < l.nlog; k++) {
            unsigned char c = l.log[k];
            if ((c & 0x60) == 0 && c != 0x18 && c != '\r' && c != '\n' && c != 0x11) bare_ctl++;
        }
        CHECK(bare_ctl == 0, "ESCCTL: no unescaped control bytes in the stream (%d)", bare_ctl);
        free(l.log);
        memset(&l, 0, sizeof(l));
        l.corrupt_at = -1;
        l.corrupt_in_at = -1;
        rc = run(rz, dir4, &files[4], 1, &l, &done, "--errors", "20000");
        CHECK(rc == AD_ZM_OK && same_file(dir4, &files[4]), "receiver-side CRC errors every 20000 bytes recovered (rc=%d)", rc);
        free(l.log);
    }

    /* no receiver: the caller presses Esc at a plain terminal */
    {
        int sv[2];
        AdZmIo io;
        socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
        memset(&l, 0, sizeof(l));
        l.corrupt_at = -1;
        l.corrupt_in_at = -1;
        l.fd = sv[0];
        io.ctx = &l; io.send = link_send; io.recv = link_recv; io.progress = NULL; io.log = NULL;
        if (write(sv[1], "\x1b", 1) != 1) return 2;
        rc = ad_zm_send(&io, files, 1, &done);
        CHECK(rc == AD_ZM_CANCELLED && done == 0, "Esc at a terminal without ZMODEM cancels (rc=%d)", rc);
        CHECK(l.nlog >= 16 && memcmp(l.log + l.nlog - 16, "\x18\x18\x18\x18\x18\x18\x18\x18\b\b\b\b\b\b\b\b", 16) == 0,
              "the standard abort (CAN x8, BS x8) goes out");
        free(l.log);
        /* and a hang-up mid-wait */
        memset(&l, 0, sizeof(l));
        l.corrupt_at = -1;
        l.corrupt_in_at = -1;
        l.fd = sv[0];
        close(sv[1]);
        rc = ad_zm_send(&io, files, 1, &done);
        CHECK(rc == AD_ZM_HANGUP, "a dropped connection is reported as a hang-up (rc=%d)", rc);
        free(l.log);
        close(sv[0]);
    }

    /* ---------------- receiving: lrzsz's sz sends to ad_zm_receive() */
    {
        static const char *const OPTS[][3] = { { NULL }, { "-e", NULL }, { NULL } };
        char sdir[] = "/tmp/anetdraw_zm_XXXXXX";
        int k;
        if (!mkdtemp(sdir)) return 2;
        for (i = 0; i < 2; i++) {
            char path[512];
            FILE *fp;
            snprintf(path, sizeof(path), "%s/%s", sdir, i == 0 ? "big.bin" : "every_byte.bin");
            fp = fopen(path, "wb");
            fwrite(i == 0 ? big : every, 1, i == 0 ? sizeof(big) : sizeof(every), fp);
            fclose(fp);
        }
        for (k = 0; k < 5; k++) {
            int sv[2], st;
            pid_t pid;
            AdZmIo io;
            AdZmRecv got;
            const char *file = k == 1 || k == 4 ? "every_byte.bin" : "big.bin";
            const unsigned char *want = k == 1 || k == 4 ? every : big;
            size_t wlen = k == 1 || k == 4 ? sizeof(every) : sizeof(big);
            socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
            pid = fork();
            if (pid == 0) {
                char sz[600];
                int fd = open("/dev/null", 1);
                dup2(sv[1], 0);
                dup2(sv[1], 1);
                if (fd >= 0) dup2(fd, 2);
                close(sv[0]);
                if (chdir(sdir) != 0) _exit(98);
                snprintf(sz, sizeof(sz), "%s", rz);
                /* lsz lives next to lrz */
                {
                    char *slash = strrchr(sz, '/');
                    if (slash) strcpy(slash + 1, "lsz");
                    else strcpy(sz, "sz");
                }
                if (k == 1) execl(sz, sz, "-q", "-e", file, (char *)NULL);
                else execl(sz, sz, "-q", file, (char *)NULL);
                _exit(99);
            }
            close(sv[1]);
            memset(&l, 0, sizeof(l));
            l.corrupt_at = -1;
            l.corrupt_in_at = k == 2 ? 30000 : -1;
            l.swap_del = k == 4;
            l.fd = sv[0];
            io.ctx = &l; io.send = link_send; io.recv = link_recv; io.progress = NULL; io.log = NULL;
            rc = ad_zm_receive(&io, k == 3 ? 50000 : 1000000, &got);
            close(sv[0]);
            waitpid(pid, &st, 0);
            if (k == 3) {
                CHECK(rc == AD_ZM_TOO_BIG && !got.data, "receive: a file over the limit is refused (rc=%d)", rc);
            } else {
                CHECK(rc == AD_ZM_OK && got.len == wlen && memcmp(got.data, want, wlen) == 0 && strcmp(got.name, file) == 0,
                      "receive %s%s: rc=%d, %zu bytes, name %s", file,
                      k == 1 ? " (sender escapes everything)" : k == 2 ? " (a byte corrupted on the way)"
                      : k == 4 ? " (DEL arrives as backspace, Synchronet SWAP_DELETE)" : "",
                      rc, got.len, got.name);
            }
            free(got.data);
            free(l.log);
            (void)OPTS;
        }
        /* nobody sends: Esc from a plain terminal cancels the wait */
        {
            int sv[2];
            AdZmIo io;
            AdZmRecv got;
            socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
            memset(&l, 0, sizeof(l));
            l.corrupt_at = l.corrupt_in_at = -1;
            l.fd = sv[0];
            io.ctx = &l; io.send = link_send; io.recv = link_recv; io.progress = NULL; io.log = NULL;
            if (write(sv[1], "\x1b", 1) != 1) return 2;
            rc = ad_zm_receive(&io, 1000, &got);
            CHECK(rc == AD_ZM_CANCELLED && l.nlog > 0 && memmem(l.log, l.nlog, "\x18" "B01", 4),
                  "receive: announces ZRINIT, and Esc cancels (rc=%d)", rc);
            free(l.log);
            close(sv[0]);
            close(sv[1]);
        }
    }

    printf("\n%d failure(s)\n", fails);
    {
        char cmd[600];
        snprintf(cmd, sizeof(cmd), "rm -rf /tmp/anetdraw_zm_*");
        if (system(cmd) != 0) { /* leftovers are harmless */ }
    }
    return fails ? 1 : 0;
}
