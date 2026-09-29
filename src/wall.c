/* The shared wall -- see include/wall.h. */
#include "../include/wall.h"
#include "../include/fileio.h"
#include "../include/files.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define open _open
#define write _write
#define close _close
#ifndef O_BINARY
#define O_BINARY _O_BINARY
#endif
#else
#include <unistd.h>
#define O_BINARY 0
#endif

#define HDR 16
#define REC 8
#define COMPACT_BYTES (256L * 1024)

static void wall_path(const AdWall *w, const char *name, char *out, size_t outsz) {
    ad_path_join(w->dir, name, out, outsz);
}

static void put16(unsigned char *p, unsigned v) { p[0] = (unsigned char)v; p[1] = (unsigned char)(v >> 8); }
static unsigned get16(const unsigned char *p) { return p[0] | p[1] << 8; }

/* Reads the whole log file. */
static unsigned char *read_log(const AdWall *w, long *len) {
    char path[AD_PATH_MAX];
    FILE *f;
    unsigned char *b;
    long n;
    *len = 0;
    wall_path(w, "wall.log", path, sizeof(path));
    f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0 || (n = ftell(f)) < 0 || fseek(f, 0, SEEK_SET) != 0) { fclose(f); return NULL; }
    b = (unsigned char *)malloc((size_t)n + 1);
    if (b && fread(b, 1, (size_t)n, f) != (size_t)n) { free(b); b = NULL; }
    fclose(f);
    if (b) *len = n;
    return b;
}

static int write_log_header(const AdWall *w, unsigned gen) {
    char path[AD_PATH_MAX], err[80];
    unsigned char h[HDR];
    memcpy(h, "ANDWALL1", 8);
    h[8] = (unsigned char)gen; h[9] = (unsigned char)(gen >> 8);
    h[10] = (unsigned char)(gen >> 16); h[11] = (unsigned char)(gen >> 24);
    put16(h + 12, (unsigned)w->w);
    put16(h + 14, (unsigned)w->h);
    wall_path(w, "wall.log", path, sizeof(path));
    return ad_write_file(path, h, HDR, err, sizeof(err));
}

static unsigned log_gen(const unsigned char *b, long len) {
    if (len < HDR || memcmp(b, "ANDWALL1", 8) != 0) return 0;
    return (unsigned)b[8] | (unsigned)b[9] << 8 | (unsigned)b[10] << 16 | (unsigned)b[11] << 24;
}

static void apply(AdCanvas *c, AdCanvas *shadow, const unsigned char *r, int *changed) {
    int x = (int)get16(r), y = (int)get16(r + 2);
    AdCell old;
    if (x >= c->w || y >= c->h) return;
    old = ad_canvas_get(c, x, y);
    if (old.ch != r[4] || old.attr != r[5]) (*changed)++;
    ad_canvas_set(c, x, y, r[4], r[5]);
    if (shadow) ad_canvas_set(shadow, x, y, r[4], r[5]);
}

/* Snapshot + the whole log into c. */
static int load_all(AdWall *w, AdCanvas *c, char *err, size_t errsz) {
    char path[AD_PATH_MAX];
    AdSauce meta;
    unsigned char *log;
    long len, i;
    int changed = 0;
    wall_path(w, "wall.ans", path, sizeof(path));
    if (!ad_ans_load(path, c, &meta, err, errsz)) return 0;
    if (c->w != w->w || c->h < w->h) ad_canvas_resize(c, w->w, w->h);
    c->h = w->h;
    c->fixed = 1;
    log = read_log(w, &len);
    w->gen = log ? log_gen(log, len) : 0;
    w->offset = HDR;
    for (i = HDR; log && i + REC <= len; i += REC) apply(c, NULL, log + i, &changed);
    w->offset = log ? HDR + ((len - HDR) / REC) * REC : HDR;
    free(log);
    return 1;
}

int ad_wall_open(AdWall *w, const char *data_dir, int width, int height, const char *me,
                 AdCanvas *c, char *err, size_t errsz) {
    char path[AD_PATH_MAX], who[AD_PATH_MAX];
    memset(w, 0, sizeof(*w));
    ad_path_join(data_dir, "wall", w->dir, sizeof(w->dir));
    ad_path_join(w->dir, "who", who, sizeof(who));
    ad_mkdirs(who);
    snprintf(w->me, sizeof(w->me), "%.60s", me);
    w->compact_at = COMPACT_BYTES;
    wall_path(w, "wall.ans", path, sizeof(path));
    if (!ad_file_exists(path)) {
        /* a new, blank wall */
        AdCanvas blank;
        AdSauce meta;
        if (!ad_canvas_init(&blank, width, height)) { snprintf(err, errsz, "out of memory"); return 0; }
        ad_canvas_resize(&blank, width, height);
        blank.ice = 1;
        memset(&meta, 0, sizeof(meta));
        snprintf(meta.title, sizeof(meta.title), "The Wall");
        ad_ans_save(path, &blank, &meta, err, errsz);
        ad_canvas_free(&blank);
        w->w = width;
        w->h = height;
        write_log_header(w, 1);
    }
    {
        /* the wall's own size is in the log header (the sysop may have
           changed --wall since it was made) */
        long len;
        unsigned char *log = read_log(w, &len);
        if (log && len >= HDR && log_gen(log, len)) {
            w->w = (int)get16(log + 12);
            w->h = (int)get16(log + 14);
        } else {
            w->w = width;
            w->h = height;
            write_log_header(w, 1);
        }
        free(log);
    }
    return load_all(w, c, err, errsz);
}

int ad_wall_poll(AdWall *w, AdCanvas *c, AdCanvas *shadow) {
    long len, i;
    int changed = 0;
    unsigned char *log = read_log(w, &len);
    if (!log) return 0;
    if (log_gen(log, len) != w->gen) {
        /* compacted by another door: start over from the new snapshot */
        char err[80];
        free(log);
        if (!load_all(w, c, err, sizeof(err))) return 0;
        if (shadow) {
            int x, y;
            for (y = 0; y < c->h; y++)
                for (x = 0; x < c->w; x++) {
                    AdCell cell = ad_canvas_get(c, x, y);
                    ad_canvas_set(shadow, x, y, cell.ch, cell.attr);
                }
        }
        return -1;
    }
    for (i = w->offset; i + REC <= len; i += REC) apply(c, shadow, log + i, &changed);
    w->offset = i;
    free(log);
    return changed;
}

/* The writers' lock: held around each append and around a compaction.
   An OS lock (fcntl / LockFileEx) on wall.lck -- it goes away by itself
   if a door dies holding it. Readers don't need it: the log is replaced
   by an atomic rename. */
static int lock_take(const AdWall *w) {
    char path[AD_PATH_MAX];
    int fd;
    wall_path(w, "wall.lck", path, sizeof(path));
    fd = open(path, O_CREAT | O_RDWR | O_BINARY, 0664);
    if (fd < 0) return -1;
#ifdef _WIN32
    {
        OVERLAPPED ov;
        memset(&ov, 0, sizeof(ov));
        if (!LockFileEx((HANDLE)_get_osfhandle(fd), LOCKFILE_EXCLUSIVE_LOCK, 0, 1, 0, &ov)) {
            close(fd);
            return -1;
        }
    }
#else
    {
        struct flock fl;
        memset(&fl, 0, sizeof(fl));
        fl.l_type = F_WRLCK;
        fl.l_whence = SEEK_SET;
        while (fcntl(fd, F_SETLKW, &fl) != 0) {
            if (errno != EINTR) { close(fd); return -1; }
        }
    }
#endif
    return fd;
}

static void lock_drop(int fd) {
    if (fd < 0) return;
#ifdef _WIN32
    {
        OVERLAPPED ov;
        memset(&ov, 0, sizeof(ov));
        UnlockFileEx((HANDLE)_get_osfhandle(fd), 0, 1, 0, &ov);
    }
#endif
    close(fd);  /* (POSIX: closing drops the fcntl lock) */
}

/* Folds the log into a new snapshot and starts a new generation. */
static void compact(AdWall *w) {
    AdCanvas c;
    AdSauce meta;
    char path[AD_PATH_MAX], err[80];
    int lk = lock_take(w);
    if (lk < 0) return;
    if (ad_canvas_init(&c, w->w, w->h) && load_all(w, &c, err, sizeof(err))) {
        memset(&meta, 0, sizeof(meta));
        snprintf(meta.title, sizeof(meta.title), "The Wall");
        c.fixed = 1;
        wall_path(w, "wall.ans", path, sizeof(path));
        if (ad_ans_save(path, &c, &meta, err, sizeof(err))) write_log_header(w, w->gen + 1);
    }
    ad_canvas_free(&c);
    lock_drop(lk);
}

static int append(const AdWall *w, const unsigned char *buf, size_t n) {
    char path[AD_PATH_MAX];
    int fd;
    size_t done = 0;
    wall_path(w, "wall.log", path, sizeof(path));
    fd = open(path, O_WRONLY | O_APPEND | O_BINARY);
    if (fd < 0) return 0;
    while (done < n) {
        /* whole records per write, so readers never see a torn one */
        size_t chunk = n - done > 4096 ? 4096 : n - done;
        long r = (long)write(fd, buf + done, (unsigned)chunk);
        if (r <= 0) break;
        done += (size_t)r;
    }
    close(fd);
    return done == n;
}

int ad_wall_publish(AdWall *w, const AdWallOp *ops, int n) {
    unsigned char *buf;
    int i, ok, lk;
    long len = 0;
    if (n <= 0) return 1;
    buf = (unsigned char *)malloc((size_t)n * REC);
    if (!buf) return 0;
    for (i = 0; i < n; i++) {
        unsigned char *r = buf + (size_t)i * REC;
        put16(r, ops[i].x);
        put16(r + 2, ops[i].y);
        r[4] = ops[i].ch;
        r[5] = ops[i].attr;
        r[6] = r[7] = 0;
    }
    lk = lock_take(w);   /* no compaction can run between our read and write */
    ok = append(w, buf, (size_t)n * REC);
    {
        char path[AD_PATH_MAX];
        struct stat st;
        wall_path(w, "wall.log", path, sizeof(path));
        if (stat(path, &st) == 0) len = (long)st.st_size;
    }
    lock_drop(lk);
    free(buf);
    if (len > w->compact_at) compact(w);
    return ok;
}

void ad_wall_here(AdWall *w, const char *name, int x, int y) {
    char path[AD_PATH_MAX], who[AD_PATH_MAX], file[96], line[128], err[80];
    int n;
    ad_path_join(w->dir, "who", who, sizeof(who));
    snprintf(file, sizeof(file), "%s.txt", w->me);
    ad_path_join(who, file, path, sizeof(path));
    n = snprintf(line, sizeof(line), "%ld|%d|%d|%.35s\n", (long)time(NULL), x, y, name);
    ad_write_file(path, (const unsigned char *)line, (size_t)n, err, sizeof(err));
}

int ad_wall_peers(AdWall *w, AdWallPeer *peers, int max) {
    char who[AD_PATH_MAX], mine[96];
    DIR *d;
    struct dirent *de;
    int n = 0;
    long now = (long)time(NULL);
    ad_path_join(w->dir, "who", who, sizeof(who));
    snprintf(mine, sizeof(mine), "%s.txt", w->me);
    d = opendir(who);
    if (!d) return 0;
    while ((de = readdir(d)) != NULL && n < max) {
        char path[AD_PATH_MAX], line[160];
        FILE *f;
        long t;
        int x, y;
        char name[40];
        if (de->d_name[0] == '.' || strcmp(de->d_name, mine) == 0) continue;
        ad_path_join(who, de->d_name, path, sizeof(path));
        f = fopen(path, "r");
        if (!f) continue;
        if (!fgets(line, sizeof(line), f) || sscanf(line, "%ld|%d|%d|%35[^\n]", &t, &x, &y, name) != 4) {
            fclose(f);
            continue;
        }
        if (now - t > 86400) {
            fclose(f);
            remove(path);  /* left behind by a door that never said goodbye */
            continue;
        }
        if (now - t <= 10) {
            snprintf(peers[n].name, sizeof(peers[n].name), "%.35s", name);
            peers[n].x = x;
            peers[n].y = y;
            peers[n].color = 9 + n % 6;   /* bright blue, green, cyan, red, magenta, yellow */
            n++;
        }
        fclose(f);
    }
    closedir(d);
    return n;
}

void ad_wall_leave(AdWall *w) {
    char path[AD_PATH_MAX], who[AD_PATH_MAX], file[96];
    ad_path_join(w->dir, "who", who, sizeof(who));
    snprintf(file, sizeof(file), "%s.txt", w->me);
    ad_path_join(who, file, path, sizeof(path));
    remove(path);
}
