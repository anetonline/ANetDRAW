#ifndef ANETDRAW_GALLERY_H
#define ANETDRAW_GALLERY_H

/* The shared gallery: one flat folder (--data/gallery) that every
   caller can browse and download from. A published piece is stored as
   "<owner key>-<name>.ans" -- the owner key is the caller's
   "<num>_<name>" folder key, which never contains '-' -- so it is clear
   whose it is (they and the sysop may remove it). Files the sysop drops
   in by hand (no owner prefix) show up too. Titles and artists come from
   each file's SAUCE record; there is no separate index to get out of
   step, and publishing is a write-then-rename, so callers on other nodes
   never see half a file. */

#include <stddef.h>
#include "fileio.h"
#include "files.h"

typedef struct {
    char file[256];        /* name in the gallery folder */
    char owner[96];        /* owner key, "" for sysop-added pieces */
    char title[AD_SAUCE_TITLE_LEN + 1];   /* SAUCE title, else the name */
    char author[AD_SAUCE_AUTHOR_LEN + 1];
    char date[9];          /* SAUCE CCYYMMDD, "" if none */
    int w, h;              /* SAUCE size, 0 if none */
    long size;
    long mtime;
} AdGalleryItem;

/* Lists the gallery newest first. *items is malloc'd (free() it).
   Returns the count (0 for an empty or missing folder). */
int ad_gallery_list(const char *dir, AdGalleryItem **items);

/* The name a piece downloads as: the part after the owner prefix. */
const char *ad_gallery_plain_name(const char *file);

/* The gallery file name for owner + a file name (sanitized, .ans). */
int ad_gallery_file_name(const char *owner, const char *name, char *out, size_t outsz);

/* Writes data as the gallery file `file`. Returns 1 ok, 0 error (err set). */
int ad_gallery_write(const char *dir, const char *file, const unsigned char *data, size_t len,
                     char *err, size_t errsz);

/* Removes a gallery file; refuses anything that isn't a plain name. */
int ad_gallery_remove(const char *dir, const char *file);

/* Reads a whole gallery file (at most 4 MB) into a malloc'd buffer. */
unsigned char *ad_gallery_read(const char *dir, const char *file, size_t *len);

#endif
