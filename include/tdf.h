#ifndef ANETDRAW_TDF_H
#define ANETDRAW_TDF_H

/* TheDraw (.TDF) banner fonts. Format per Roy/SAC's TDF specification,
 * checked against tdfiglet and Synchronet's tdfonts_lib.js:
 *
 *   file:  0x13 "TheDraw FONTS file" 0x1A, then one or more fonts
 *   font:  55 AA 00 FF, name length, name[12], 4 unused, type
 *          (0 outline, 1 block, 2 color), letter spacing, data size
 *          (u16), 94 u16 glyph offsets for '!'..'~' (FFFF = none),
 *          then the glyph data
 *   glyph: width, height, then cells row by row -- 0x0D ends a row,
 *          0x00 ends the glyph; color fonts store (char, attr) pairs,
 *          block and outline fonts just the char. Cells a row doesn't
 *          reach are transparent.
 *
 * Fonts come from files nobody vouches for, so every offset, size and
 * row/column is bounds-checked. */

#include <stddef.h>
#include "tools.h"

#define AD_TDF_OUTLINE 0
#define AD_TDF_BLOCK   1
#define AD_TDF_COLOR   2

typedef struct {
    int file;          /* index into AdTdfIndex.files */
    long offset;       /* where this font's 55 AA 00 FF marker is */
    char name[13];
    unsigned char type;
} AdTdfEntry;

typedef struct {
    char **files;
    int nfiles;
    AdTdfEntry *e;
    int n;
} AdTdfIndex;

typedef struct {
    char name[13];
    int type, spacing;
    unsigned short offs[94];
    unsigned char *data;   /* glyph data */
    size_t size;
} AdTdfFont;

/* Lists every font in every .tdf in dir (sorted by name). Returns the
   count; 0 if none or the folder is missing. */
int  ad_tdf_scan(const char *dir, AdTdfIndex *ix);
void ad_tdf_free_index(AdTdfIndex *ix);

int  ad_tdf_load(const AdTdfIndex *ix, int i, AdTdfFont *f);
void ad_tdf_free(AdTdfFont *f);

/* Renders one line of text. attr colors block and outline fonts (color
   fonts carry their own). Cells no glyph covers are left as char 0 on
   black, which a transparent paste skips. Returns 1 if anything was
   drawn (out is replaced), 0 if none of the characters exist. */
int  ad_tdf_render(const AdTdfFont *f, const char *text, unsigned char attr, AdClipboard *out);

#endif
