/* Intro splash screen. The art itself is generated into src/splash_art.c
 * by util/make_splash.py -- edit the generator, not the output. */
#ifndef ANETDRAW_SPLASH_H
#define ANETDRAW_SPLASH_H

#include "anetdraw.h"

/* The art is 79x24 cells; each row is its SGR + CP437 bytes, placed
   at (row, col) plus the centering offset. */
#define AD_SPLASH_W 80
#define AD_SPLASH_H 24

typedef struct {
    int row, col;
    const char *data;
} AdSplashRow;

extern const AdSplashRow AD_SPLASH_ROWS[];
extern const int AD_SPLASH_NROWS;

/* Shows the splash centered on the door's screen and waits for a key.
   A local window resize while it's up re-centers it (door->cols/rows
   are updated). Returns 0 if the caller hung up, 1 otherwise. */
int ad_splash_show(AdDoor *door);

#endif /* ANETDRAW_SPLASH_H */
