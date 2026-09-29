#ifndef ANETDRAW_TRANSFER_H
#define ANETDRAW_TRANSFER_H

/* Sends files to the caller with ZMODEM over the door's own connection
   (see zmodem.h). Turns mouse reporting and CP437->UTF-8 output
   translation off for the transfer, and swallows whatever the terminal
   sends after it. The caller repaints the screen afterwards. */

#include "anetdraw.h"
#include "zmodem.h"

int ad_xfer_send(const AdDoor *door, const AdZmFile *files, int n, int *done);

#endif
