#include "../include/undo.h"
#include <stdlib.h>
#include <string.h>

int ad_undo_init(AdUndo *u) {
    memset(u, 0, sizeof(*u));
    u->cells = (AdUndoCell *)malloc(AD_UNDO_MAX_CELLS * sizeof(AdUndoCell));
    u->acts = (AdUndoAction *)malloc(AD_UNDO_MAX_ACTIONS * sizeof(AdUndoAction));
    if (!u->cells || !u->acts) {
        ad_undo_free(u);
        return 0;
    }
    return 1;
}

void ad_undo_free(AdUndo *u) {
    free(u->cells);
    free(u->acts);
    u->cells = NULL;
    u->acts = NULL;
}

/* Drops the oldest action, shifting everything down -- including the
   action still being recorded (acts[nacts]) when one is open, since
   this runs mid-recording to make room. */
static void drop_oldest(AdUndo *u) {
    int n, i;
    int live = u->open && u->started;  /* acts[nacts] is being recorded */
    int total_acts = u->nacts + (live ? 1 : 0);
    int total_cells = u->ncells + (live ? u->acts[u->nacts].count : 0);
    if (u->nacts == 0) return;
    n = u->acts[0].count;
    memmove(u->cells, u->cells + n, (size_t)(total_cells - n) * sizeof(AdUndoCell));
    memmove(u->acts, u->acts + 1, (size_t)(total_acts - 1) * sizeof(AdUndoAction));
    u->nacts--;
    u->ncells -= n;
    if (u->applied > 0) u->applied--;
    for (i = 0; i < total_acts - 1; i++) u->acts[i].start -= n;
}

void ad_undo_begin(AdUndo *u) {
    if (!u->cells || u->open) return;
    u->open = 1;
    u->started = 0;
    u->overflow = 0;
}

/* First real change of the open action: drop the redo history and
   claim the next action slot. */
static void start_action(AdUndo *u) {
    u->nacts = u->applied;
    u->ncells = u->nacts ? u->acts[u->nacts - 1].start + u->acts[u->nacts - 1].count : 0;
    /* started is still 0 here, so drop_oldest() doesn't treat the
       not-yet-claimed slot as a live action */
    if (u->nacts == AD_UNDO_MAX_ACTIONS) drop_oldest(u);
    u->acts[u->nacts].start = u->ncells;
    u->acts[u->nacts].count = 0;
    u->started = 1;
}

void ad_undo_record(void *ctx, int x, int y, AdCell before, AdCell after) {
    AdUndo *u = (AdUndo *)ctx;
    AdUndoAction *a;
    AdUndoCell *c;
    if (!u->cells || !u->open || u->overflow) return;
    if (!u->started) start_action(u);
    a = &u->acts[u->nacts];
    /* make room by forgetting old actions -- never the open one */
    while (u->ncells + a->count >= AD_UNDO_MAX_CELLS && u->nacts > 0) {
        drop_oldest(u);
        a = &u->acts[u->nacts];
    }
    if (u->ncells + a->count >= AD_UNDO_MAX_CELLS) {
        u->overflow = 1;
        return;
    }
    c = &u->cells[a->start + a->count];
    c->x = (unsigned short)x;
    c->y = (unsigned short)y;
    c->before = before;
    c->after = after;
    a->count++;
}

int ad_undo_end(AdUndo *u) {
    AdUndoAction *a;
    if (!u->cells || !u->open) return 0;
    u->open = 0;
    if (!u->started) return 0;
    a = &u->acts[u->nacts];
    if (u->overflow) {
        /* can't undo this one correctly -- and older steps would
           then restore on top of it wrongly, so drop them all */
        u->nacts = u->applied = u->ncells = 0;
        return -1;
    }
    if (a->count == 0) return 0;
    u->ncells = a->start + a->count;
    u->nacts++;
    u->applied = u->nacts;
    return 1;
}

void ad_undo_clear(AdUndo *u) {
    u->nacts = u->applied = u->ncells = 0;
    u->open = u->started = u->overflow = 0;
}

int ad_undo_can_undo(const AdUndo *u) { return u->cells && u->applied > 0; }
int ad_undo_can_redo(const AdUndo *u) { return u->cells && u->applied < u->nacts; }

int ad_undo_undo(AdUndo *u, AdCanvas *c) {
    AdUndoAction *a;
    int i;
    if (!ad_undo_can_undo(u) || u->open) return 0;
    a = &u->acts[--u->applied];
    /* reverse order, so a cell written twice in one action ends up at
       its very first 'before' */
    for (i = a->count - 1; i >= 0; i--) {
        const AdUndoCell *uc = &u->cells[a->start + i];
        ad_canvas_set(c, uc->x, uc->y, uc->before.ch, uc->before.attr);
    }
    return 1;
}

int ad_undo_redo(AdUndo *u, AdCanvas *c) {
    AdUndoAction *a;
    int i;
    if (!ad_undo_can_redo(u) || u->open) return 0;
    a = &u->acts[u->applied++];
    for (i = 0; i < a->count; i++) {
        const AdUndoCell *uc = &u->cells[a->start + i];
        ad_canvas_set(c, uc->x, uc->y, uc->after.ch, uc->after.attr);
    }
    return 1;
}
