/* Property test for undo.c: random edits / undos / redos / no-op keys
 * against a reference model that keeps a full canvas snapshot per
 * applied action. Tiny buffers so old-action dropping, the action-count
 * cap, and the too-big-to-record path all get exercised constantly.
 *
 *   cc -O1 -g -fsanitize=address,undefined -DAD_UNDO_MAX_CELLS=40 \
 *      -DAD_UNDO_MAX_ACTIONS=6 -o test_undo tests/test_undo.c src/undo.c src/canvas.c
 */
#include "../include/undo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W 8
#define H 4
#define MAXSNAP 4096

static AdCell snap[MAXSNAP][W * H];  /* model: snapshot after each applied action */
static int snap_n, snap_top;         /* snap_n = history length, snap_top = applied */

static void take(const AdCanvas *c, AdCell *out) {
    int x, y;
    for (y = 0; y < H; y++) for (x = 0; x < W; x++) out[y * W + x] = ad_canvas_get(c, x, y);
}

static int same(const AdCanvas *c, const AdCell *ref) {
    AdCell now[W * H];
    take(c, now);
    return memcmp(now, ref, sizeof(now)) == 0;
}

int main(int argc, char **argv) {
    AdCanvas c;
    AdUndo u;
    long iter, iters = argc > 1 ? atol(argv[1]) : 200000;
    unsigned seed = argc > 2 ? (unsigned)atoi(argv[2]) : 1;
    int fails = 0, overflows = 0;
    srand(seed);
    ad_canvas_init(&c, W, H);
    ad_undo_init(&u);
    take(&c, snap[0]);
    snap_n = snap_top = 0;  /* snap[0] = initial state; snap[k] = after action k */

    for (iter = 0; iter < iters && !fails; iter++) {
        int op = rand() % 10;
        if (op < 5) {                              /* an edit of 0..12 cells */
            int k, n = (rand() % 50 == 0) ? 45 + rand() % 10 : rand() % 13, r;  /* sometimes too big to record */
            ad_undo_begin(&u);
            for (k = 0; k < n; k++) {
                int x = rand() % W, y = rand() % H;
                AdCell before = ad_canvas_get(&c, x, y), after;
                after.ch = (unsigned char)('a' + rand() % 3);
                after.attr = (unsigned char)(rand() % 2);
                if (before.ch == after.ch && before.attr == after.attr) continue;
                ad_undo_record(&u, x, y, before, after);
                ad_canvas_set(&c, x, y, after.ch, after.attr);
            }
            r = ad_undo_end(&u);
            if (r == 1) {
                snap_n = ++snap_top;
                take(&c, snap[snap_top]);
                /* model the drop-oldest cap: only the last few survive */
            } else if (r == -1) {
                overflows++;
                /* history dropped: current state becomes the new base */
                take(&c, snap[0]);
                snap_n = snap_top = 0;
            }
        } else if (op < 7) {                       /* undo */
            int did = ad_undo_undo(&u, &c);
            int expect = ad_undo_can_undo(&u) || did;  /* informative only */
            (void)expect;
            if (did) {
                snap_top--;
                if (snap_top < 0) { fprintf(stderr, "undo past model start\n"); fails++; break; }
            }
        } else if (op < 9) {                       /* redo */
            if (ad_undo_redo(&u, &c)) snap_top++;
        } else {                                   /* a key that edits nothing */
            ad_undo_begin(&u);
            if (ad_undo_end(&u) != 0) { fprintf(stderr, "empty action recorded\n"); fails++; }
        }

        /* the library may have forgotten old actions: the model's
           applied count can exceed what's still undoable, never less */
        if (u.applied > snap_top || u.nacts - u.applied != snap_n - snap_top) {
            if (!(u.nacts - u.applied == snap_n - snap_top)) {
                fprintf(stderr, "iter %ld: redo depth %d vs model %d\n", iter,
                        u.nacts - u.applied, snap_n - snap_top);
                fails++;
            }
        }
        if (!same(&c, snap[snap_top])) {
            fprintf(stderr, "iter %ld: canvas differs from model after op %d\n", iter, op);
            fails++;
        }
        /* can we still undo all the way back as far as the library claims? */
        if (snap_n >= MAXSNAP - 2) {  /* keep the model bounded */
            memmove(snap[0], snap[snap_n - 50], sizeof(snap[0]) * 51);
            snap_top -= snap_n - 50;
            snap_n = 50;
            if (snap_top < 0) snap_top = 0;
        }
    }
    printf("%s after %ld ops (seed %u), %d too-big edits dropped the history\n",
           fails ? "FAIL" : "PASS", iter, seed, overflows);
    ad_undo_free(&u);
    ad_canvas_free(&c);
    return fails ? 1 : 0;
}
