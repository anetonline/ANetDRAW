#ifndef ANETDRAW_WALL_H
#define ANETDRAW_WALL_H

/* The shared wall: one canvas several callers (on different nodes, so
 * different processes) draw on at once, SyncWall-style. It lives in
 * <data>/wall/ as plain files, so it works the same on every BBS:
 *
 *   wall.ans   the wall as of the last compaction (ANSI + SAUCE)
 *   wall.log   header "ANDWALL1" + generation (u32) + width, height
 *              (u16 each), then 8-byte records: x, y (u16 each), glyph,
 *              attr, 2 spare. Every door appends its changes (a single
 *              O_APPEND write) and reads everyone's from where it left off.
 *   who/       one small file per door: name, cursor, time -- who's here
 *   wall.lck   an OS lock (fcntl / LockFileEx) every writer holds briefly:
 *              around each append, and around a whole compaction
 *
 * Compaction (under the lock, so no append can slip between reading the
 * log and replacing it) writes a new wall.ans, then a new wall.log with
 * the next generation, each by write-then-rename. Readers don't lock: they
 * see the old log or the new one, and a door that sees the generation
 * change reloads everything. */

#include <stddef.h>
#include "canvas.h"

#define AD_WALL_MAX_PEERS 16

typedef struct {
    unsigned short x, y;
    unsigned char ch, attr;
} AdWallOp;

typedef struct {
    char name[36];
    int x, y;
    int color;       /* marker color for this peer */
} AdWallPeer;

typedef struct {
    char dir[1024];
    char me[64];         /* this door's presence file name */
    unsigned gen;
    long offset;         /* bytes of wall.log already applied */
    int w, h;
    long compact_at;     /* log size that triggers a compaction */
} AdWall;

/* Opens (creating if needed, at w x h) the wall under data_dir and loads
   it into c. `me` names this door (node number or process id). */
int  ad_wall_open(AdWall *wall, const char *data_dir, int w, int h, const char *me,
                  AdCanvas *c, char *err, size_t errsz);
/* Applies everyone's new changes to c (and to shadow, if given). Returns
   how many cells changed, or -1 if the wall had to be reloaded (c was
   replaced wholesale). */
int  ad_wall_poll(AdWall *wall, AdCanvas *c, AdCanvas *shadow);
/* Shares this door's changes. Returns 1 on success. */
int  ad_wall_publish(AdWall *wall, const AdWallOp *ops, int n);
/* Presence: say where this door's caller is; list everyone else who has
   said so in the last 10 seconds. */
void ad_wall_here(AdWall *wall, const char *name, int x, int y);
int  ad_wall_peers(AdWall *wall, AdWallPeer *peers, int max);
void ad_wall_leave(AdWall *wall);

#endif
