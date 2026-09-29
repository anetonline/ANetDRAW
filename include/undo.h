/* Multi-level undo/redo as cell diffs (not whole-canvas snapshots).
 *
 * Every committed cell change reaches ad_undo_record() through the
 * AdSurface commit hook (tools.h), so each tool gets undo for free.
 * Changes are grouped into actions -- the editor opens one per
 * keypress -- so a flood fill or a paste undoes as one step.
 *
 * Bounded: at most AD_UNDO_MAX_CELLS recorded cells and
 * AD_UNDO_MAX_ACTIONS actions; the oldest actions fall off first. An
 * action too large to record at all drops the history (and says so)
 * rather than leaving a half-recorded step that would undo wrongly. */
#ifndef ANETDRAW_UNDO_H
#define ANETDRAW_UNDO_H

#include "canvas.h"

#ifndef AD_UNDO_MAX_CELLS          /* overridable for the unit test */
#define AD_UNDO_MAX_CELLS   262144
#endif
#ifndef AD_UNDO_MAX_ACTIONS
#define AD_UNDO_MAX_ACTIONS 2000
#endif

typedef struct {
    unsigned short x, y;
    AdCell before, after;
} AdUndoCell;

typedef struct {
    int start;   /* first cell in AdUndo.cells */
    int count;
} AdUndoAction;

typedef struct {
    AdUndoCell *cells;
    AdUndoAction *acts;
    int ncells;     /* cells used by acts[0..nacts-1] */
    int nacts;      /* recorded actions, including undone ones (redo) */
    int applied;    /* actions currently applied; redo = applied..nacts-1 */
    int open;       /* between ad_undo_begin() and ad_undo_end() */
    int started;    /* the open action has recorded its first cell */
    int overflow;   /* the open action outgrew the buffer */
} AdUndo;

int  ad_undo_init(AdUndo *u);   /* 0 on alloc failure */
void ad_undo_free(AdUndo *u);
/* Opens an action. Nothing changes until the first recorded cell --
   only then is the redo history dropped -- so keys that don't edit
   (cursor moves, menus) never cost the artist their redo steps. */
void ad_undo_begin(AdUndo *u);
/* Closes the open action. Returns -1 if it was too big to keep (history
   was dropped), 1 if it recorded something, 0 if it was empty. */
int  ad_undo_end(AdUndo *u);
/* AdCommitHook-compatible recorder (ctx = AdUndo *). */
void ad_undo_record(void *ctx, int x, int y, AdCell before, AdCell after);
/* Returns 1 if something was undone/redone. */
int  ad_undo_undo(AdUndo *u, AdCanvas *c);
int  ad_undo_redo(AdUndo *u, AdCanvas *c);
/* Forgets all history (e.g. after the canvas changes size, when the
   recorded cell positions no longer line up). */
void ad_undo_clear(AdUndo *u);
int  ad_undo_can_undo(const AdUndo *u);
int  ad_undo_can_redo(const AdUndo *u);

#endif /* ANETDRAW_UNDO_H */
