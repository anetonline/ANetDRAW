#include "../include/tdf.h"
#include "../include/files.h"
#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define MAGIC "\x13TheDraw FONTS file\x1a"
#define MAGIC_LEN 20
#define MAX_GLYPH_W 30   /* the spec's limits; a glyph past them is corrupt */
#define MAX_GLYPH_H 12   /* (44 such glyphs in 4 fonts of the pack) */
#define MAX_TEXT_W 1000

static int has_tdf_ext(const char *n) {
    size_t l = strlen(n);
    return l > 4 && n[l - 4] == '.' && tolower((unsigned char)n[l - 3]) == 't' &&
           tolower((unsigned char)n[l - 2]) == 'd' && tolower((unsigned char)n[l - 1]) == 'f';
}

static int by_name(const void *a, const void *b) {
    const AdTdfEntry *x = (const AdTdfEntry *)a, *y = (const AdTdfEntry *)b;
    int i;
    for (i = 0; i < 13; i++) {
        int cx = tolower((unsigned char)x->name[i]), cy = tolower((unsigned char)y->name[i]);
        if (cx != cy) return cx - cy;
        if (!cx) break;
    }
    return x->file != y->file ? x->file - y->file : (x->offset < y->offset ? -1 : 1);
}

static void add_file_fonts(AdTdfIndex *ix, const char *path, int *cap) {
    FILE *f = fopen(path, "rb");
    unsigned char h[25];
    long pos = MAGIC_LEN, len;
    char magic[MAGIC_LEN];
    int fi = -1;
    if (!f) return;
    if (fread(magic, 1, MAGIC_LEN, f) != MAGIC_LEN || memcmp(magic, MAGIC, MAGIC_LEN) != 0) {
        fclose(f);
        return;
    }
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    while (pos + 25 + 188 <= len) {
        size_t size;
        AdTdfEntry *e;
        int nl, i;
        if (fseek(f, pos, SEEK_SET) != 0 || fread(h, 1, 25, f) != 25) break;
        if (h[0] != 0x55 || h[1] != 0xAA || h[2] != 0x00 || h[3] != 0xFF) break;
        size = (size_t)(h[23] | h[24] << 8);
        if (pos + 25 + 188 + (long)size > len) break;
        if (fi < 0) {
            char **nf = (char **)realloc(ix->files, (size_t)(ix->nfiles + 1) * sizeof(char *));
            if (!nf) break;
            ix->files = nf;
            ix->files[ix->nfiles] = (char *)malloc(strlen(path) + 1);
            if (!ix->files[ix->nfiles]) break;
            strcpy(ix->files[ix->nfiles], path);
            fi = ix->nfiles++;
        }
        if (ix->n == *cap) {
            AdTdfEntry *ne;
            *cap = *cap ? *cap * 2 : 256;
            ne = (AdTdfEntry *)realloc(ix->e, (size_t)*cap * sizeof(AdTdfEntry));
            if (!ne) break;
            ix->e = ne;
        }
        e = &ix->e[ix->n++];
        e->file = fi;
        e->offset = pos;
        e->type = h[21];
        nl = h[4] > 12 ? 12 : h[4];
        for (i = 0; i < nl; i++) e->name[i] = (h[5 + i] >= 0x20 && h[5 + i] < 0x7f) ? (char)h[5 + i] : '?';
        while (nl > 0 && e->name[nl - 1] == ' ') nl--;
        e->name[nl] = 0;
        if (!e->name[0]) strcpy(e->name, "(unnamed)");
        pos += 25 + 188 + (long)size;
    }
    fclose(f);
}

int ad_tdf_scan(const char *dir, AdTdfIndex *ix) {
    DIR *d;
    struct dirent *de;
    int cap = 0;
    memset(ix, 0, sizeof(*ix));
    d = opendir(dir);
    if (!d) return 0;
    while ((de = readdir(d)) != NULL) {
        char path[AD_PATH_MAX];
        struct stat st;
        if (de->d_name[0] == '.' || !has_tdf_ext(de->d_name)) continue;
        ad_path_join(dir, de->d_name, path, sizeof(path));
        if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) continue;
        add_file_fonts(ix, path, &cap);
    }
    closedir(d);
    if (ix->n) qsort(ix->e, (size_t)ix->n, sizeof(AdTdfEntry), by_name);
    return ix->n;
}

void ad_tdf_free_index(AdTdfIndex *ix) {
    int i;
    for (i = 0; i < ix->nfiles; i++) free(ix->files[i]);
    free(ix->files);
    free(ix->e);
    memset(ix, 0, sizeof(*ix));
}

int ad_tdf_load(const AdTdfIndex *ix, int i, AdTdfFont *font) {
    const AdTdfEntry *e;
    FILE *f;
    unsigned char h[25 + 188];
    int k;
    memset(font, 0, sizeof(*font));
    if (i < 0 || i >= ix->n) return 0;
    e = &ix->e[i];
    f = fopen(ix->files[e->file], "rb");
    if (!f) return 0;
    if (fseek(f, e->offset, SEEK_SET) != 0 || fread(h, 1, sizeof(h), f) != sizeof(h)) {
        fclose(f);
        return 0;
    }
    font->type = h[21];
    font->outline_style = AD_TDF_OUTLINE_DEFAULT;
    font->spacing = h[22] > 40 ? 40 : h[22];
    font->size = (size_t)(h[23] | h[24] << 8);
    for (k = 0; k < 94; k++) font->offs[k] = (unsigned short)(h[25 + k * 2] | h[26 + k * 2] << 8);
    memcpy(font->name, e->name, sizeof(font->name));
    font->data = (unsigned char *)malloc(font->size + 1);
    if (!font->data || fread(font->data, 1, font->size, f) != font->size) {
        fclose(f);
        ad_tdf_free(font);
        return 0;
    }
    fclose(f);
    return 1;
}

void ad_tdf_free(AdTdfFont *f) {
    free(f->data);
    f->data = NULL;
    f->size = 0;
}

/* Glyph index for a character, trying upper case when a font has no
   lower case (most don't). -1 = not in the font. */
static int glyph_index(const AdTdfFont *f, unsigned char c) {
    if (c >= 33 && c <= 126 && f->offs[c - 33] != 0xFFFF && f->offs[c - 33] + 2u <= f->size) {
        const unsigned char *p = f->data + f->offs[c - 33];
        /* out-of-spec sizes mean a damaged glyph: treat it as missing */
        if (p[0] >= 1 && p[0] <= MAX_GLYPH_W && p[1] >= 1 && p[1] <= MAX_GLYPH_H) return c - 33;
    }
    if (islower(c)) return glyph_index(f, (unsigned char)toupper(c));
    return -1;
}

static void glyph_size(const AdTdfFont *f, int gi, int *w, int *h) {
    const unsigned char *p = f->data + f->offs[gi];  /* checked by glyph_index() */
    *w = p[0];
    *h = p[1];
}

/* TheDraw's 19 outline styles: the CP437 character each letter code
   'A'..'Q' becomes. Table as TheDraw had it, via retrofont (Mike Krueger,
   Apache-2.0, crates/retrofont/src/glyph.rs). Style 10 (index 9) is the
   one Roy/SAC's TDF spec prints, and the default here. */
static const unsigned char OUTLINE_STYLES[AD_TDF_OUTLINE_STYLES][17] = {
    { 0xC4, 0xC4, 0xB3, 0xB3, 0xDA, 0xBF, 0xDA, 0xBF, 0xC0, 0xD9, 0xC0, 0xD9, 0xB4, 0xC3, 0x20, 0x20, 0x20 },
    { 0xCD, 0xC4, 0xB3, 0xB3, 0xD5, 0xB8, 0xDA, 0xBF, 0xD4, 0xBE, 0xC0, 0xD9, 0xB5, 0xC3, 0x20, 0x20, 0x20 },
    { 0xC4, 0xCD, 0xB3, 0xB3, 0xDA, 0xBF, 0xD5, 0xB8, 0xC0, 0xD9, 0xD4, 0xBE, 0xB4, 0xC6, 0x20, 0x20, 0x20 },
    { 0xCD, 0xCD, 0xB3, 0xB3, 0xD5, 0xB8, 0xD5, 0xB8, 0xD4, 0xBE, 0xD4, 0xBE, 0xB5, 0xC6, 0x20, 0x20, 0x20 },
    { 0xC4, 0xC4, 0xBA, 0xB3, 0xD6, 0xBF, 0xDA, 0xB7, 0xC0, 0xBD, 0xD3, 0xD9, 0xB6, 0xC3, 0x20, 0x20, 0x20 },
    { 0xCD, 0xC4, 0xBA, 0xB3, 0xC9, 0xB8, 0xDA, 0xB7, 0xD4, 0xBC, 0xD3, 0xD9, 0xB9, 0xC3, 0x20, 0x20, 0x20 },
    { 0xC4, 0xCD, 0xBA, 0xB3, 0xD6, 0xBF, 0xD5, 0xBB, 0xC0, 0xBD, 0xC8, 0xBE, 0xB6, 0xC6, 0x20, 0x20, 0x20 },
    { 0xCD, 0xCD, 0xBA, 0xB3, 0xC9, 0xB8, 0xD5, 0xBB, 0xD4, 0xBC, 0xC8, 0xBE, 0xB9, 0xC6, 0x20, 0x20, 0x20 },
    { 0xC4, 0xC4, 0xB3, 0xBA, 0xDA, 0xB7, 0xD6, 0xBF, 0xD3, 0xD9, 0xC0, 0xBD, 0xB4, 0xC7, 0x20, 0x20, 0x20 },
    { 0xCD, 0xC4, 0xB3, 0xBA, 0xD5, 0xBB, 0xD6, 0xBF, 0xC8, 0xBE, 0xC0, 0xBD, 0xB5, 0xC7, 0x20, 0x20, 0x20 },
    { 0xC4, 0xCD, 0xB3, 0xBA, 0xDA, 0xB7, 0xC9, 0xB8, 0xD3, 0xD9, 0xD4, 0xBC, 0xB4, 0xCC, 0x20, 0x20, 0x20 },
    { 0xCD, 0xCD, 0xB3, 0xBA, 0xD5, 0xBB, 0xC9, 0xB8, 0xC8, 0xBE, 0xD4, 0xBC, 0xB5, 0xCC, 0x20, 0x20, 0x20 },
    { 0xC4, 0xC4, 0xBA, 0xBA, 0xD6, 0xB7, 0xD6, 0xB7, 0xD3, 0xBD, 0xD3, 0xBD, 0xB6, 0xC7, 0x20, 0x20, 0x20 },
    { 0xCD, 0xC4, 0xBA, 0xBA, 0xC9, 0xBB, 0xD6, 0xB7, 0xC8, 0xBC, 0xD3, 0xBD, 0xB9, 0xC7, 0x20, 0x20, 0x20 },
    { 0xC4, 0xCD, 0xBA, 0xBA, 0xD6, 0xB7, 0xC9, 0xBB, 0xD3, 0xBD, 0xC8, 0xBC, 0xB6, 0xCC, 0x20, 0x20, 0x20 },
    { 0xCD, 0xCD, 0xBA, 0xBA, 0xC9, 0xBB, 0xC9, 0xBB, 0xC8, 0xBC, 0xC8, 0xBC, 0xB9, 0xCC, 0x20, 0x20, 0x20 },
    { 0xDC, 0xDC, 0xDB, 0xDB, 0xDC, 0xDC, 0xDC, 0xDC, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0x20, 0x20, 0x20 },
    { 0xDF, 0xDF, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xDF, 0xDF, 0xDF, 0xDF, 0xDB, 0xDB, 0x20, 0x20, 0x20 },
    { 0xDF, 0xDC, 0xDE, 0xDD, 0xDE, 0xDD, 0xDC, 0xDC, 0xDF, 0xDF, 0xDE, 0xDD, 0xDB, 0xDB, 0x20, 0x20, 0x20 },
};

static int outline_char(unsigned char c, int style) {
    if (style < 0 || style >= AD_TDF_OUTLINE_STYLES) style = AD_TDF_OUTLINE_DEFAULT;
    if (c == 'O') return ' ';             /* hard space: inside the letter */
    if (c == '@' || c == '&') return -1;  /* filler / descender mark: see-through */
    if (c >= 'A' && c <= 'Q') return OUTLINE_STYLES[style][c - 'A'];
    return c;
}

static void draw_glyph(const AdTdfFont *f, int gi, unsigned char attr, AdClipboard *out, int x0) {
    const unsigned char *p = f->data + f->offs[gi] + 2, *end = f->data + f->size;
    int w, h, row = 0, col = 0;
    glyph_size(f, gi, &w, &h);
    while (p < end && *p) {
        unsigned char ch = *p++, a = attr;
        int draw;
        if (ch == 0x0D) {
            row++;
            col = 0;
            continue;
        }
        if (f->type == AD_TDF_COLOR) {
            if (p >= end) break;
            a = *p++;
        }
        draw = ch < 0x20 ? ' ' : ch;
        if (f->type == AD_TDF_OUTLINE) draw = outline_char(ch, f->outline_style);
        if (draw >= 0 && row < h && col < w && row < out->h && x0 + col < out->w) {
            AdCell *c = &out->cells[(size_t)row * out->w + x0 + col];
            c->ch = (unsigned char)draw;
            c->attr = a;
        }
        col++;
    }
}

int ad_tdf_render(const AdTdfFont *f, const char *text, unsigned char attr, AdClipboard *out) {
    int total = 0, height = 0, space = 2, n = 0, x, gi, w, h;
    const unsigned char *t;
    AdClipboard cb;
    if (!f->data) return 0;
    /* a space is half an 'A' wide (TDF fonts have no space glyph) */
    if ((gi = glyph_index(f, 'A')) >= 0 || (gi = glyph_index(f, 'a')) >= 0) {
        glyph_size(f, gi, &w, &h);
        space = w / 2 < 2 ? 2 : w / 2;
    }
    for (t = (const unsigned char *)text; *t; t++) {
        if (*t == ' ') w = space, h = 0;
        else if ((gi = glyph_index(f, *t)) >= 0) glyph_size(f, gi, &w, &h);
        else continue;
        if (n++) total += f->spacing;
        total += w;
        if (h > height) height = h;
        if (total > MAX_TEXT_W) break;
    }
    if (total <= 0 || height <= 0) return 0;
    if (total > MAX_TEXT_W) total = MAX_TEXT_W;
    cb.w = total;
    cb.h = height;
    cb.cells = (AdCell *)calloc((size_t)total * height, sizeof(AdCell));  /* char 0 on black: see-through */
    if (!cb.cells) return 0;
    x = 0;
    n = 0;
    for (t = (const unsigned char *)text; *t && x < total; t++) {
        if (*t == ' ') w = space;
        else if ((gi = glyph_index(f, *t)) >= 0) glyph_size(f, gi, &w, &h);
        else continue;
        if (n++) x += f->spacing;
        if (*t != ' ') draw_glyph(f, gi, attr, &cb, x);
        x += w;
    }
    ad_clip_free(out);
    *out = cb;
    return 1;
}
