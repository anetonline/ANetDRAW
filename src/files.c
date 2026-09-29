#include "../include/files.h"
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#define ad_mkdir_one(p) _mkdir(p)
#else
#include <strings.h>
#include <unistd.h>
#define ad_mkdir_one(p) mkdir((p), 0775)
#endif

static int safe_char(int c) {
    return isalnum(c) || c == ' ' || c == '_' || c == '-' || c == '.';
}

static int has_ext(const char *name, const char *ext) {
    size_t n = strlen(name), e = strlen(ext);
    size_t i;
    if (n < e) return 0;
    for (i = 0; i < e; i++)
        if (tolower((unsigned char)name[n - e + i]) != tolower((unsigned char)ext[i])) return 0;
    return 1;
}

int ad_safe_filename(const char *in, char *out, size_t outsz) {
    size_t n = 0;
    const char *p;
    if (outsz < 8) return 0;
    for (p = in; *p && n < AD_NAME_MAX - 5 && n < outsz - 5; p++) {
        unsigned char c = (unsigned char)*p;
        if (!safe_char(c)) continue;          /* drops / \ : and friends */
        if (n == 0 && (c == '.' || c == ' ')) continue;  /* no hidden files */
        if (c == '.' && n > 0 && out[n - 1] == '.') continue;  /* no ".." */
        out[n++] = (char)c;
    }
    while (n > 0 && (out[n - 1] == ' ' || out[n - 1] == '.')) n--;
    out[n] = 0;
    if (n == 0) return 0;
    if (!has_ext(out, ".ans")) {
        if (n + 4 >= outsz) return 0;
        strcat(out, ".ans");
    }
    return 1;
}

void ad_user_key(int user_num, const char *user_name, char *out, size_t outsz) {
    char name[64];
    size_t n = 0;
    const char *p;
    for (p = user_name; *p && n < sizeof(name) - 1; p++) {
        unsigned char c = (unsigned char)*p;
        if (isalnum(c)) name[n++] = (char)tolower(c);
        else if ((c == ' ' || c == '_' || c == '-') && n > 0 && name[n - 1] != '_') name[n++] = '_';
    }
    while (n > 0 && name[n - 1] == '_') n--;
    name[n] = 0;
    if (!name[0]) strcpy(name, "user");
    if (user_num > 0) snprintf(out, outsz, "%d_%s", user_num, name);
    else snprintf(out, outsz, "%s", name);
}

int ad_mkdirs(const char *path) {
    char buf[AD_PATH_MAX];
    size_t i, n = strlen(path);
    struct stat st;
    if (n == 0 || n >= sizeof(buf)) return 0;
    memcpy(buf, path, n + 1);
    for (i = 1; i <= n; i++) {
        if (buf[i] == '/' || buf[i] == '\\' || buf[i] == 0) {
            char save = buf[i];
            buf[i] = 0;
            if (stat(buf, &st) != 0) {
                if (ad_mkdir_one(buf) != 0 && errno != EEXIST) return 0;
            }
            buf[i] = save;
        }
    }
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

int ad_abspath(const char *in, char *out, size_t outsz) {
#ifdef _WIN32
    return _fullpath(out, in, outsz) != NULL;
#else
    char tmp[4096];
    if (!realpath(in, tmp)) return 0;
    if (strlen(tmp) >= outsz) return 0;
    strcpy(out, tmp);
    return 1;
#endif
}

void ad_parent_dir(const char *in, char *out, size_t outsz) {
    size_t n;
    char *slash, *bslash, *cut;
    snprintf(out, outsz, "%s", in);
    n = strlen(out);
    /* drop trailing separators (but never the root's own) */
    while (n > 1 && (out[n - 1] == '/' || out[n - 1] == '\\') &&
           !(n == 3 && out[1] == ':'))
        out[--n] = 0;
    slash = strrchr(out, '/');
    bslash = strrchr(out, '\\');
    cut = (bslash && (!slash || bslash > slash)) ? bslash : slash;
    if (!cut) return;                          /* no separator: leave as is */
    if (cut == out) { out[1] = 0; return; }    /* "/x" -> "/" */
    if (cut == out + 2 && out[1] == ':') { out[3] = 0; return; }  /* "C:\x" -> "C:\" */
    *cut = 0;
}

void ad_path_join(const char *dir, const char *name, char *out, size_t outsz) {
    size_t n = strlen(dir);
    if (n > 0 && (dir[n - 1] == '/' || dir[n - 1] == '\\'))
        snprintf(out, outsz, "%s%s", dir, name);
    else
        snprintf(out, outsz, "%s/%s", dir, name);
}

int ad_file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

static int art_file(const char *name) {
    static const char *const EXT[] = { ".ans", ".asc", ".txt", ".ice", ".diz", ".nfo" };
    size_t i;
    for (i = 0; i < sizeof(EXT) / sizeof(EXT[0]); i++)
        if (has_ext(name, EXT[i])) return 1;
    return 0;
}

static int cmp_entry(const void *a, const void *b) {
    const AdDirEntry *x = (const AdDirEntry *)a, *y = (const AdDirEntry *)b;
    if (x->is_dir != y->is_dir) return y->is_dir - x->is_dir;  /* dirs first */
#ifdef _WIN32
    return _stricmp(x->name, y->name);
#else
    return strcasecmp(x->name, y->name);
#endif
}

int ad_list_dir(const char *dir, int want_dirs, AdDirList *out) {
    DIR *d = opendir(dir);
    struct dirent *de;
    int cap = 64;
    out->n = 0;
    out->e = NULL;
    if (!d) return 0;
    out->e = (AdDirEntry *)malloc((size_t)cap * sizeof(AdDirEntry));
    if (!out->e) { closedir(d); return 0; }
    while ((de = readdir(d)) != NULL) {
        char full[AD_PATH_MAX];
        struct stat st;
        int is_dir;
        if (de->d_name[0] == '.') continue;
        if (strlen(de->d_name) >= sizeof(out->e[0].name)) continue;
        ad_path_join(dir, de->d_name, full, sizeof(full));
        if (stat(full, &st) != 0) continue;
        is_dir = S_ISDIR(st.st_mode);
        if (is_dir ? !want_dirs : (!S_ISREG(st.st_mode) || !art_file(de->d_name))) continue;
        if (out->n == cap) {
            AdDirEntry *ne;
            if (cap >= 5000) break;  /* enough for any browser page */
            cap *= 2;
            ne = (AdDirEntry *)realloc(out->e, (size_t)cap * sizeof(AdDirEntry));
            if (!ne) break;
            out->e = ne;
        }
        snprintf(out->e[out->n].name, sizeof(out->e[0].name), "%s", de->d_name);
        out->e[out->n].is_dir = is_dir;
        out->e[out->n].size = (long)st.st_size;
        out->n++;
    }
    closedir(d);
    qsort(out->e, (size_t)out->n, sizeof(AdDirEntry), cmp_entry);
    return 1;
}

void ad_free_dir(AdDirList *l) {
    free(l->e);
    l->e = NULL;
    l->n = 0;
}
