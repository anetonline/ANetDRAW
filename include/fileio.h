/* Saving and loading ANSI art, with SAUCE.
 *
 * Checked against the real references, not from memory: the SAUCE spec
 * (acid.org/info/sauce/sauce.htm) and Moebius's libtextmode
 * (textmode.js add_sauce_bytes()/get_sauce(), ansi.js encode_as_ansi()
 * and its parser). Files written here open in Moebius / PabloDraw /
 * SyncTERM exactly as drawn, and theirs open here.
 *
 * File layout (SAUCE spec, "SAUCE applied to a file"):
 *   art bytes | 0x1A (EOF) | [ "COMNT" + 64-byte lines ] | 128-byte record
 * Record: "SAUCE" "00" Title[35] Author[20] Group[20] Date[8] (CCYYMMDD,
 * space-padded strings) FileSize u32le (art only) DataType=1 (Character)
 * FileType=1 (ANSi) TInfo1=width TInfo2=height TInfo3/4=0 Comments
 * TFlags (bit0 iCE, bits1-2 letter spacing, bits3-4 aspect) TInfoS[22]
 * (font name, zero-padded). */
#ifndef ANETDRAW_FILEIO_H
#define ANETDRAW_FILEIO_H

#include <stddef.h>
#include "canvas.h"

#define AD_SAUCE_TITLE_LEN  35
#define AD_SAUCE_AUTHOR_LEN 20
#define AD_SAUCE_GROUP_LEN  20

typedef struct {
    char title[AD_SAUCE_TITLE_LEN + 1];
    char author[AD_SAUCE_AUTHOR_LEN + 1];
    char group[AD_SAUCE_GROUP_LEN + 1];
    char date[9];          /* CCYYMMDD, as read; set on save */
    int  has_sauce;        /* load: the file had a SAUCE record */
    /* Display options, as TheDraw offered when saving. Written at the
       start of the art (and read back on load):
         clear_screen  ESC[2J ESC[H first, so it shows on a clean screen
         speed         DECSCS "CSI 0;n*r" display speed, n = 1..11
                       (300 ... 115200 bps, see AD_SPEEDS); 0 = full
                       speed. The art ends with "CSI 0;0*r" to put the
                       viewer's terminal back to full speed. */
    int  clear_screen;
    int  speed;
} AdSauce;

/* DECSCS speed index -> bps (SyncTERM cterm.adoc), index 0 = full speed */
#define AD_SPEED_COUNT 12
extern const long AD_SPEEDS[AD_SPEED_COUNT];

/* Encodes the canvas as ANSI + SAUCE into a malloc'd buffer (caller
   frees). Returns the size, or 0 on allocation failure. */
size_t ad_ans_encode(const AdCanvas *c, const AdSauce *meta, unsigned char **out);

/* Writes the canvas to path atomically (temp file + rename). Returns 1
   on success; on failure 0 with a short reason in err. */
int ad_ans_save(const char *path, const AdCanvas *c, const AdSauce *meta,
                char *err, size_t errsz);

/* Decodes ANSI/ASCII art (SAUCE optional) into c, which must be
   initialized; it is resized to the art's size (width from SAUCE, else
   80; height from SAUCE, else the rows used). The canvas's iCE mode
   comes from the SAUCE flag. Returns 1 on success. */
int ad_ans_decode(const unsigned char *data, size_t len, AdCanvas *c, AdSauce *meta,
                  char *err, size_t errsz);

/* Reads a file (at most AD_MAX_FILE_BYTES) and decodes it. */
#define AD_MAX_FILE_BYTES (4L * 1024 * 1024)
int ad_ans_load(const char *path, AdCanvas *c, AdSauce *meta, char *err, size_t errsz);

#endif /* ANETDRAW_FILEIO_H */
