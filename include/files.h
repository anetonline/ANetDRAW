/* Filesystem helpers for Save / Open: safe file names for callers,
 * per-caller folders, directory listings for the file browser.
 *
 * Callers never type a path. Their file names go through
 * ad_safe_filename() (letters, digits, space, _ - . only; no leading dot;
 * .ans added) and are only ever joined onto their own folder. Only the
 * sysop's browser walks the real filesystem. */
#ifndef ANETDRAW_FILES_H
#define ANETDRAW_FILES_H

#include <stddef.h>

#define AD_PATH_MAX 1024
#define AD_NAME_MAX 48

/* Cleans a caller-typed name into a safe file name ending in .ans.
   Returns 0 if nothing usable is left. */
int ad_safe_filename(const char *in, char *out, size_t outsz);

/* A folder-name-safe key for a user: "<number>_<name>" or "<name>". */
void ad_user_key(int user_num, const char *user_name, char *out, size_t outsz);

/* mkdir -p. Returns 1 if the directory exists afterwards. */
int ad_mkdirs(const char *path);

/* Absolute, normalized form of a path (realpath/_fullpath). */
int ad_abspath(const char *in, char *out, size_t outsz);

/* Parent directory of an absolute path (stays at the root). */
void ad_parent_dir(const char *in, char *out, size_t outsz);

void ad_path_join(const char *dir, const char *name, char *out, size_t outsz);
int  ad_file_exists(const char *path);

/* One directory listing, for the browser. */
typedef struct {
    char name[256];
    int  is_dir;
    long size;
} AdDirEntry;

typedef struct {
    AdDirEntry *e;
    int n;
} AdDirList;

/* Lists dir: sub-directories first (only if want_dirs), then art files
   (.ans .asc .txt .ice .diz .nfo, case-insensitive), each group sorted.
   Hidden (dot) entries are skipped. Returns 0 if the directory can't be
   read. */
int  ad_list_dir(const char *dir, int want_dirs, AdDirList *out);
/* The same, listing image files (.png .jpg .jpeg .gif .bmp) instead. */
int  ad_list_images(const char *dir, int want_dirs, AdDirList *out);
void ad_free_dir(AdDirList *l);

#endif /* ANETDRAW_FILES_H */
