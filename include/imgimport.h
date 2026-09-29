#ifndef ANETDRAW_IMGIMPORT_H
#define ANETDRAW_IMGIMPORT_H

/* Image -> ANSI: PNG, JPEG, GIF or BMP (stb_image, public domain), scaled
 * to `cols` columns of half-block cells -- each cell is two square
 * pixels, top in the foreground of an upper-half block (0xDF), bottom in
 * the background -- in the 16 VGA colors, with optional Floyd-Steinberg
 * dithering. With ice = 0 the background can only be one of the first 8
 * colors, so a cell whose bottom pixel is bright uses a lower-half block
 * (0xDC) with that pixel in the foreground instead.
 *
 * The data may come from a caller's upload, so the image's size is
 * checked (stbi_info) before it is decoded: at most 8192 pixels on a
 * side and 40 megapixels. */

#include <stddef.h>
#include "canvas.h"

int ad_image_to_canvas(const unsigned char *data, size_t len, int cols, int ice, int dither,
                       AdCanvas *out, int *img_w, int *img_h, char *err, size_t errsz);

/* Is this file name an image ANetDRAW can import? */
int ad_image_name(const char *name);

#endif
