/* TheDraw font engine test.
 *   test_tdf all <fontdir>                 load + render every font (run under ASan)
 *   test_tdf dump <fontdir> <index> <text>  print the render as "ch attr" rows
 *
 *   cc -O1 -g -fsanitize=address,undefined -o test_tdf tests/test_tdf.c \
 *      src/tdf.c src/tools.c src/files.c src/canvas.c src/undo.c
 */
#include "../include/tdf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    AdTdfIndex ix;
    int n, i, ok = 0, empty = 0, bad = 0;
    if (argc < 3) return 2;
    n = ad_tdf_scan(argv[2], &ix);
    if (strcmp(argv[1], "all") == 0) {
        static const char *const TEXTS[] = { "ANetDRAW", "Hello, World! 123", "gjpqy_$", "~}|{`^]\\[",
                                              "", "   ", "\x01\x7f\xff" };
        for (i = 0; i < n; i++) {
            AdTdfFont f;
            AdClipboard cb = { 0, 0, NULL };
            size_t t;
            if (!ad_tdf_load(&ix, i, &f)) { bad++; continue; }
            for (t = 0; t < sizeof(TEXTS) / sizeof(TEXTS[0]); t++) {
                if (ad_tdf_render(&f, TEXTS[t], 0x07, &cb)) {
                    if (cb.w <= 0 || cb.h <= 0 || cb.h > 12) bad++;
                } else if (t == 0) {
                    empty++;
                }
            }
            if (cb.cells) ok++;
            ad_clip_free(&cb);
            ad_tdf_free(&f);
        }
        printf("%d fonts in %d files: %d rendered, %d had none of \"ANetDRAW\", %d failed\n",
               n, ix.nfiles, ok, empty, bad);
        ad_tdf_free_index(&ix);
        return bad ? 1 : 0;
    }
    if (strcmp(argv[1], "dump") == 0 && argc >= 5) {
        AdTdfFont f;
        AdClipboard cb = { 0, 0, NULL };
        int x, y;
        i = atoi(argv[3]);
        if (!ad_tdf_load(&ix, i, &f)) return 1;
        printf("%s\t%s\t%d\n", ix.files[ix.e[i].file], ix.e[i].name, (int)ix.e[i].offset);
        if (ad_tdf_render(&f, argv[4], 0x07, &cb)) {
            for (y = 0; y < cb.h; y++) {
                for (x = 0; x < cb.w; x++)
                    printf("%02x%02x ", cb.cells[y * cb.w + x].ch, cb.cells[y * cb.w + x].attr);
                printf("\n");
            }
        }
        ad_clip_free(&cb);
        ad_tdf_free(&f);
        ad_tdf_free_index(&ix);
        return 0;
    }
    return 2;
}
