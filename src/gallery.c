#include "../include/gallery.h"
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#define GALLERY_MAX_FILE (4L * 1024 * 1024)

static int plain_name(const char *f) {
    return f[0] && f[0] != '.' && !strchr(f, '/') && !strchr(f, '\\') && !strchr(f, ':');
}

static int is_ans(const char *name) {
    static const char *const EXT[] = { ".ans", ".asc", ".ice" };
    size_t n = strlen(name), i, j;
    for (i = 0; i < sizeof(EXT) / sizeof(EXT[0]); i++) {
        size_t e = strlen(EXT[i]);
        if (n <= e) continue;
        for (j = 0; j < e && tolower((unsigned char)name[n - e + j]) == EXT[i][j]; j++) {}
        if (j == e) return 1;
    }
    return 0;
}

/* An owner key is what ad_user_key() makes: [0-9a-z_] only. */
static size_t owner_len(const char *file) {
    const char *dash = strchr(file, '-');
    const char *p;
    if (!dash || dash == file || !dash[1]) return 0;
    for (p = file; p < dash; p++)
        if (!(isdigit((unsigned char)*p) || islower((unsigned char)*p) || *p == '_')) return 0;
    return (size_t)(dash - file);
}

const char *ad_gallery_plain_name(const char *file) {
    size_t n = owner_len(file);
    return n ? file + n + 1 : file;
}

int ad_gallery_file_name(const char *owner, const char *name, char *out, size_t outsz) {
    char clean[AD_NAME_MAX + 8];
    if (!ad_safe_filename(name, clean, sizeof(clean))) return 0;
    if (owner && owner[0]) return snprintf(out, outsz, "%s-%s", owner, clean) < (int)outsz;
    return snprintf(out, outsz, "%s", clean) < (int)outsz;
}

static void sauce_field(char *dst, size_t dstsz, const unsigned char *src, size_t len) {
    size_t n = len < dstsz - 1 ? len : dstsz - 1, i;
    for (i = 0; i < n; i++) dst[i] = (src[i] >= 0x20 && src[i] != 0x7f) ? (char)src[i] : ' ';
    while (n > 0 && dst[n - 1] == ' ') n--;
    dst[n] = 0;
}

static void peek_sauce(const char *path, AdGalleryItem *it) {
    unsigned char r[128];
    FILE *f = fopen(path, "rb");
    if (!f) return;
    if (it->size >= 128 && fseek(f, -128L, SEEK_END) == 0 && fread(r, 1, 128, f) == 128 &&
        memcmp(r, "SAUCE00", 7) == 0) {
        sauce_field(it->title, sizeof(it->title), r + 7, 35);
        sauce_field(it->author, sizeof(it->author), r + 42, 20);
        sauce_field(it->date, sizeof(it->date), r + 82, 8);
        if (r[94] == 1) {  /* DataType character: TInfo1/2 are width/height */
            it->w = r[96] | r[97] << 8;
            it->h = r[98] | r[99] << 8;
        }
    }
    fclose(f);
}

static int newest_first(const void *a, const void *b) {
    const AdGalleryItem *x = (const AdGalleryItem *)a, *y = (const AdGalleryItem *)b;
    if (x->mtime != y->mtime) return x->mtime < y->mtime ? 1 : -1;
    return strcmp(x->file, y->file);
}

int ad_gallery_list(const char *dir, AdGalleryItem **items) {
    DIR *d = opendir(dir);
    struct dirent *de;
    int n = 0, cap = 32;
    AdGalleryItem *v;
    *items = NULL;
    if (!d) return 0;
    v = (AdGalleryItem *)malloc((size_t)cap * sizeof(*v));
    if (!v) { closedir(d); return 0; }
    while ((de = readdir(d)) != NULL) {
        char path[AD_PATH_MAX];
        struct stat st;
        AdGalleryItem *it;
        size_t on;
        if (!plain_name(de->d_name) || !is_ans(de->d_name)) continue;
        if (strlen(de->d_name) >= sizeof(v[0].file)) continue;
        ad_path_join(dir, de->d_name, path, sizeof(path));
        if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) continue;
        if (n == cap) {
            AdGalleryItem *nv;
            if (cap >= 20000) break;
            cap *= 2;
            nv = (AdGalleryItem *)realloc(v, (size_t)cap * sizeof(*v));
            if (!nv) break;
            v = nv;
        }
        it = &v[n];
        memset(it, 0, sizeof(*it));
        snprintf(it->file, sizeof(it->file), "%s", de->d_name);
        on = owner_len(it->file);
        if (on && on < sizeof(it->owner)) memcpy(it->owner, it->file, on);
        it->size = (long)st.st_size;
        it->mtime = (long)st.st_mtime;
        peek_sauce(path, it);
        if (!it->title[0]) {
            char *dot;
            snprintf(it->title, sizeof(it->title), "%.35s", ad_gallery_plain_name(it->file));
            dot = strrchr(it->title, '.');
            if (dot && dot != it->title) *dot = 0;
        }
        n++;
    }
    closedir(d);
    qsort(v, (size_t)n, sizeof(*v), newest_first);
    *items = v;
    return n;
}

int ad_gallery_write(const char *dir, const char *file, const unsigned char *data, size_t len,
                     char *err, size_t errsz) {
    char path[AD_PATH_MAX], tmp[AD_PATH_MAX + 32];
    FILE *f;
    if (!plain_name(file)) {
        snprintf(err, errsz, "bad file name");
        return 0;
    }
    ad_mkdirs(dir);
    ad_path_join(dir, file, path, sizeof(path));
#ifdef _WIN32
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
#else
    snprintf(tmp, sizeof(tmp), "%s.%ld.tmp", path, (long)getpid());
#endif
    f = fopen(tmp, "wb");
    if (!f) {
        snprintf(err, errsz, "%s", strerror(errno));
        return 0;
    }
    if (fwrite(data, 1, len, f) != len || fclose(f) != 0) {
        snprintf(err, errsz, "write failed");
        remove(tmp);
        return 0;
    }
#ifdef _WIN32
    remove(path);  /* rename() won't replace on Windows */
#endif
    if (rename(tmp, path) != 0) {
        snprintf(err, errsz, "%s", strerror(errno));
        remove(tmp);
        return 0;
    }
    return 1;
}

int ad_gallery_remove(const char *dir, const char *file) {
    char path[AD_PATH_MAX];
    if (!plain_name(file)) return 0;
    ad_path_join(dir, file, path, sizeof(path));
    return remove(path) == 0;
}

unsigned char *ad_gallery_read(const char *dir, const char *file, size_t *len) {
    char path[AD_PATH_MAX];
    FILE *f;
    long n;
    unsigned char *b;
    *len = 0;
    if (!plain_name(file)) return NULL;
    ad_path_join(dir, file, path, sizeof(path));
    f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0 || (n = ftell(f)) < 0 || n > GALLERY_MAX_FILE) {
        fclose(f);
        return NULL;
    }
    rewind(f);
    b = (unsigned char *)malloc((size_t)n + 1);
    if (b && fread(b, 1, (size_t)n, f) != (size_t)n) {
        free(b);
        b = NULL;
    }
    fclose(f);
    if (b) *len = (size_t)n;
    return b;
}
