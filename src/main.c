/* ANetDRAW Door -- ANSI/ASCII art editor (TheDraw + ACiDDraw +
 * Moebius + PabloDraw lineage), C/OpenDoors.
 *
 * main() only owns the session lifecycle: od_init() via ad_door_init(),
 * run the editor, restore the caller's terminal, od_exit().
 */
#include "../include/anetdraw.h"
#include "../include/editor.h"
#include "../include/splash.h"
#include "OpenDoor.h"

int main(int argc, char **argv) {
    AdDoor door;
    static AdEditor ed;  /* static: keeps a large struct off the stack */

    if (!ad_door_init(&door, argc, argv)) {
        od_exit(1, FALSE);
        return 1;
    }

    od_clear_keybuffer();

    /* Turn off SyncTERM's modem-speed emulation for this session (DECSCS,
       CSI 0;0 *r = unlimited -- SyncTERM cterm.adoc). A directory entry
       or an earlier door can leave the caller throttled to 9600 bps or
       slower, which makes every mouse stroke show up seconds late. Other
       terminals ignore it. */
    ad_dout("\x1b[0;0*r");

    if (!ad_splash_show(&door)) {
        ad_screen_restore_terminal();
        ad_door_close(&door);
        od_exit(0, FALSE);
        return 0;
    }

    /* after the splash: a local window resized while it was up has
       already updated door.cols/rows */
    if (!ad_editor_init(&ed, door.cols, door.rows, door.canvas_w)) {
        ad_dout("Out of memory allocating the canvas.\r\n");
        ad_door_close(&door);
        od_exit(1, FALSE);
        return 1;
    }

    ad_editor_run(&ed, &door);
    ad_editor_free(&ed);

    /* Hand the caller's terminal back the way we found it: blink mode
       reset (CSI ?33l), colors reset, cleared screen. */
    ad_screen_restore_terminal();
    ad_dout("\x1b[2J\x1b[HThanks for drawing with ANetDRAW!\r\n");

    ad_door_close(&door);
    od_exit(0, FALSE);
    return 0;
}
