#ifndef ANETDRAW_FORMATS_H
#define ANETDRAW_FORMATS_H

/* Save / open formats besides ANSI (fileio.c does ANSI + SAUCE):
 *
 *   ASCII     .asc  glyphs only, rows end CR LF; SAUCE Character/ASCII
 *   BIN       .bin  raw char+attr pairs; SAUCE BinaryText (width/2 in
 *                   FileType -- an odd width is padded one column)
 *   XBin      .xb   "XBIN" 1A, width, height, font size 16, flags (bit 3
 *                   = iCE), raw data; no palette or font; reads the
 *                   compressed kind too; SAUCE XBin
 *   PCBoard   .pcb  @X<bg hex><fg hex>; @CLS@ clears. @X00 / @XFF mean
 *                   save / restore color, so they are never written
 *                   (Synchronet wiki, custom:colors)
 *   Pipe      .pip  |00-|15 fg, |16-|23 bg, |24-|31 bright bg, |CL
 *                   clears (Mystic BBS wiki, displaycodes)
 *   Ctrl-A    .msg  Synchronet: ^A K R G Y B M C W fg, 0-7 bg, H bright,
 *                   I blink, E bright bg, N normal, L clears
 *                   (Synchronet wiki, custom:ctrl-a_codes)
 *   PNG       .png  a picture of the art in the IBM VGA font (save only)
 *
 * The BBS color-code formats carry no SAUCE record: BBS software shows
 * them as display files, and a SAUCE tail would print as junk there. */

#include <stddef.h>
#include "fileio.h"

enum {
    AD_FMT_ANSI = 0, AD_FMT_ASCII, AD_FMT_BIN, AD_FMT_XBIN,
    AD_FMT_PCBOARD, AD_FMT_PIPE, AD_FMT_CTRLA, AD_FMT_PNG,
    AD_FMT_COUNT
};

const char *ad_fmt_ext(int fmt);     /* ".ans", ".pcb", ... */
const char *ad_fmt_name(int fmt);    /* "ANSI", "PCBoard @X", ... */
/* The format a file name's extension means (AD_FMT_ANSI when unknown). */
int  ad_fmt_from_path(const char *path);
/* Whether a name ends in an extension ANSIDraw reads or writes. */
int  ad_fmt_known_ext(const char *name);

size_t ad_fmt_encode(const AdCanvas *c, const AdSauce *meta, int fmt, unsigned char **out);
/* Decodes; fmt is the hint from the extension -- .asc/.txt/.ans files
   carrying PCBoard, pipe or Ctrl-A codes are recognized from content.
   Sets meta->format to what the file turned out to be. */
int  ad_fmt_decode(const unsigned char *data, size_t len, int fmt, AdCanvas *c, AdSauce *meta,
                   char *err, size_t errsz);

#endif
