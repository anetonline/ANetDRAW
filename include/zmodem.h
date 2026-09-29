#ifndef ANETDRAW_ZMODEM_H
#define ANETDRAW_ZMODEM_H

/* A small ZMODEM sender (download to the caller), checked against the
   receivers that matter here: SyncTERM's (Synchronet src/sbbs3/zmodem.c)
   and lrzsz's rz. It never sends ZSINIT and never asks the receiver to
   escape anything. On the way out it escapes more than the minimum:
   besides ZDLE, XON/XOFF and DLE (lrzsz's default set), it escapes CR and
   0xFF, so the stream holds no bare CR or telnet IAC byte and needs no
   telnet cooking on any transport. */

#include <stddef.h>

typedef struct {
    void *ctx;
    /* Write len bytes; 0 = ok, nonzero = the connection is gone. */
    int (*send)(void *ctx, const unsigned char *buf, size_t len);
    /* One byte within ms milliseconds (ms 0 = only if one is waiting).
       Returns the byte, -1 on timeout, -2 if the connection is gone. */
    int (*recv)(void *ctx, int ms);
    /* Optional: called as data goes out, for a progress line. */
    void (*progress)(void *ctx, long sent, long total);
    /* Optional: one line per protocol event, for a debug trace. */
    void (*log)(void *ctx, const char *msg);
} AdZmIo;

typedef struct {
    const char *name;           /* just the file name, no path */
    const unsigned char *data;
    long len;
    long mtime;                 /* unix time, 0 = unknown */
} AdZmFile;

enum {
    AD_ZM_OK = 0,
    AD_ZM_NO_RECEIVER,  /* nothing answered: the terminal can't do ZMODEM */
    AD_ZM_CANCELLED,    /* the caller cancelled (Ctrl-X x5, Esc or Ctrl-C) */
    AD_ZM_SKIPPED,      /* the receiver declined the file (it may already have it) */
    AD_ZM_FAILED,       /* too many errors */
    AD_ZM_HANGUP,
    AD_ZM_TOO_BIG       /* receive: the file is over the size limit */
};

/* Sends the files as one batch. Returns an AD_ZM_* code; *files_done
   (if given) says how many arrived. On a cancel or failure it sends the
   standard abort (CAN x8 + BS x8) so the terminal leaves transfer mode. */
int ad_zm_send(const AdZmIo *io, const AdZmFile *files, int nfiles, int *files_done);

const char *ad_zm_result_text(int rc);

/* Receives one file (an upload from the caller): sends ZRINIT until a
   sender starts (about 90 seconds), takes the first file and skips any
   others. On AD_ZM_OK, out->data (malloc'd, free it) holds out->len
   bytes and out->name the sender's file name (no path). AD_ZM_NO_RECEIVER
   here means nobody started sending. */
typedef struct {
    char name[256];
    unsigned char *data;
    size_t len;
} AdZmRecv;
int ad_zm_receive(const AdZmIo *io, size_t max_bytes, AdZmRecv *out);

#endif
