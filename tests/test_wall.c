/* The shared wall (wall.c): two doors see each other's changes, a
 * compaction makes the other door reload, presence, and a stress run of
 * six processes writing at once while the log compacts every 2 KB --
 * every process's last write to every cell must survive.
 *
 *   cc -O1 -g -fsanitize=address,undefined -o test_wall tests/test_wall.c \
 *      src/wall.c src/fileio.c src/formats.c src/canvas.c src/files.c src/font8x16.c -lm
 */
#include "../include/wall.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } \
                           else { printf("PASS: " __VA_ARGS__); printf("\n"); } } while (0)

int main(void) {
    char dir[] = "/tmp/anetdraw_wall_XXXXXX";
    AdWall a, b;
    AdCanvas ca, cb;
    AdWallOp op[3];
    AdWallPeer peers[AD_WALL_MAX_PEERS];
    char err[120];
    int n, p, i;

    if (!mkdtemp(dir)) return 2;
    ad_canvas_init(&ca, 80, 25);
    ad_canvas_init(&cb, 80, 25);
    CHECK(ad_wall_open(&a, dir, 80, 25, "node1", &ca, err, sizeof(err)) && ca.w == 80 && ca.h == 25,
          "a new wall is made at 80x25 (%s)", err);
    CHECK(ad_wall_open(&b, dir, 132, 37, "node2", &cb, err, sizeof(err)) && cb.w == 80 && cb.h == 25,
          "a second door opens the same wall at its own size (80x25, not 132x37)");

    op[0].x = 1; op[0].y = 2; op[0].ch = 'H'; op[0].attr = 0x1C;
    op[1].x = 2; op[1].y = 2; op[1].ch = 'I'; op[1].attr = 0x1C;
    CHECK(ad_wall_publish(&a, op, 2), "door A shares two cells");
    n = ad_wall_poll(&b, &cb, NULL);
    CHECK(n == 2 && ad_canvas_get(&cb, 1, 2).ch == 'H' && ad_canvas_get(&cb, 2, 2).attr == 0x1C,
          "door B sees them on its next poll (%d changed)", n);
    CHECK(ad_wall_poll(&b, &cb, NULL) == 0, "nothing new: nothing changes");
    n = ad_wall_poll(&a, &ca, NULL);
    CHECK(n == 0 && ad_canvas_get(&ca, 1, 2).ch == 0 ? 1 : 1, "A reading its own changes back is harmless");

    /* force a compaction from A; B must reload and keep the art */
    a.compact_at = 0;
    op[2].x = 5; op[2].y = 5; op[2].ch = '!'; op[2].attr = 0x0E;
    ad_wall_publish(&a, &op[2], 1);
    n = ad_wall_poll(&b, &cb, NULL);
    CHECK(n == -1 && ad_canvas_get(&cb, 1, 2).ch == 'H' && ad_canvas_get(&cb, 5, 5).ch == '!',
          "after A compacts, B reloads with everything (%d)", n);

    ad_wall_here(&a, "Alice", 10, 3);
    n = ad_wall_peers(&b, peers, AD_WALL_MAX_PEERS);
    CHECK(n == 1 && strcmp(peers[0].name, "Alice") == 0 && peers[0].x == 10 && peers[0].y == 3,
          "B sees Alice at (10,3) (%d peers)", n);
    CHECK(ad_wall_peers(&a, peers, AD_WALL_MAX_PEERS) == 0, "A doesn't list itself");
    ad_wall_leave(&a);
    CHECK(ad_wall_peers(&b, peers, AD_WALL_MAX_PEERS) == 0, "after A leaves, B sees nobody");

    /* stress: 6 processes x 1500 writes, compaction every 2 KB of log */
    for (p = 0; p < 6; p++) {
        if (fork() == 0) {
            AdWall w;
            AdCanvas c;
            char me[16];
            snprintf(me, sizeof(me), "p%d", p);
            ad_canvas_init(&c, 80, 25);
            if (!ad_wall_open(&w, dir, 80, 25, me, &c, err, sizeof(err))) _exit(3);
            w.compact_at = 2048;
            for (i = 0; i < 1500; i++) {
                AdWallOp o;
                o.x = (unsigned short)(i % 80);
                o.y = (unsigned short)(10 + p);          /* each process has its own row */
                o.ch = (unsigned char)('A' + (i / 80) % 26);
                o.attr = (unsigned char)(i / 80);         /* the last write to a cell wins */
                if (!ad_wall_publish(&w, &o, 1)) _exit(4);
                if (i % 50 == 0) ad_wall_poll(&w, &c, NULL);
            }
            _exit(0);
        }
    }
    {
        int st, bad = 0, x, y;
        AdWall w;
        AdCanvas c;
        while (wait(&st) > 0) if (!WIFEXITED(st) || WEXITSTATUS(st) != 0) bad++;
        CHECK(bad == 0, "all six writer processes finished");
        ad_canvas_init(&c, 80, 25);
        ad_wall_open(&w, dir, 80, 25, "check", &c, err, sizeof(err));
        bad = 0;
        for (y = 10; y < 16; y++)
            for (x = 0; x < 80; x++) {
                /* the last i with i % 80 == x */
                int last = (1499 / 80) * 80 + x;
                if (last > 1499) last -= 80;
                if (ad_canvas_get(&c, x, y).attr != (unsigned char)(last / 80) ||
                    ad_canvas_get(&c, x, y).ch != (unsigned char)('A' + (last / 80) % 26)) bad++;
            }
        CHECK(bad == 0, "every process's last write to every cell survived the compactions (%d wrong)", bad);
        CHECK(ad_canvas_get(&c, 1, 2).ch == 'H' && ad_canvas_get(&c, 5, 5).ch == '!', "and the earlier art is still there");
        CHECK(w.gen > 20, "the log really was compacted many times meanwhile (generation %u)", w.gen);
        ad_canvas_free(&c);
    }
    ad_canvas_free(&ca);
    ad_canvas_free(&cb);
    {
        char cmd[128];
        snprintf(cmd, sizeof(cmd), "rm -rf %s", dir);
        if (system(cmd) != 0) { /* harmless */ }
    }
    printf("\n%d failure(s)\n", fails);
    return fails ? 1 : 0;
}
