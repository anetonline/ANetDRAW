# ANetDRAW Door (C / OpenDoors)

An ANSI/ASCII art editor for BBS callers, in the spirit of TheDraw,
ACiDDraw, Moebius, and PabloDraw. Built in C against
[OpenDoors](https://github.com/RealDeuce/OpenDoors), the same library
the RDQ3, ANetCHESS, and ANetCRAFT doors use.

**Status: Phase 4 (saving and loading).** You can draw with text, F-key
glyphs, lines, boxes, ellipses, flood fill, a shading brush, and
half-block "pixel" painting, with the mouse or the keyboard, on any screen
size. You can also mark blocks to copy, cut, move, flip, and paste,
undo and redo, and save and open `.ans` files with SAUCE. Downloading
(ZMODEM) and a shared gallery come next.

## Registering with your BBS

This is a standard OpenDoors door. It reads `DOOR32.SYS`, `DOOR.SYS`, or
`DORINFO1.DEF`, whichever your BBS drops:

```
/path/to/anetdraw -D%Pdoor32.sys
```

To test locally without a BBS session, use `-L`.

### Screen size

ANetDRAW works on any screen from 80x10 up to 400x200, including
SyncTERM's 132x37 mode. It finds the caller's screen size in this order:

1. `--cols N` / `--rows N`, if the sysop sets them
2. the dropfile, for **CHAIN.TXT** and **BBSDEV.DRP** (both carry the
   caller's real terminal size; DOOR32.SYS and DOOR.SYS don't)
3. in local mode, the size of the terminal window
4. asking the terminal (a cursor-position report)
5. 80x24 if nothing else answers

On screens with 39 or more columns to spare (132x37, for example), a
**toolbox sidebar** docks at the right edge, 38 columns wide. Every
column between it and the canvas stays drawing area. Past the canvas's
right edge it shows a dim dotted "desk", and **Canvas size → E Fit the
screen** makes the canvas fill that space. The toolbox has:

- FG/BG color palettes (left-click sets the foreground, right-click the
  background)
- the tools (and **T** Text) in two columns, with the current tool option
- F-key glyphs with set arrows
- mirror mode and undo/redo counts
- in the Select tool, the marked block's **Copy Cut Del Fill Move
  Recolor** buttons (or **Paste** and **All**)
- on screens 33 or more rows tall, a **File** section (New, Open, Save,
  Save as, Gallery, Publish, Download) and a **Canvas** section (its
  size with a **Size** button, Undo, Redo, Fonts, Colors, Chars)

Everything on it is clickable. The status bar shows the cursor as
**(column,row)**, e.g. `(23,6)`.

In `-L` local mode on Linux you can resize or maximize the terminal
window at any time, and the editor lays itself out again right away.
On Windows, `-L` opens ANetDRAW's own window instead of OpenDoors'
console. It draws with the IBM VGA font, so it looks exactly like
SyncTERM, and it supports the mouse. You can resize or maximize the
window, or press **Alt+Enter** for full screen. **Ctrl+Plus** /
**Ctrl+Minus** (or Ctrl+mouse wheel) step the zoom through 50, 75, 100,
125, 150, 175, 200, 250, 300 and 400%, and **Ctrl+0** goes back to
100%. The title bar shows the zoom. Whole multiples stay pixel-sharp;
the steps between are smoothed. Zooming out fits more columns and rows.
Full screen and maximize pick the biggest zoom that still leaves room
for the toolbox (150% on a 1600x900 screen, which gives 133x37).
Closing the window asks to save unsaved work, like **Q** does.
`--console` brings back the old console window.
For BBS callers, **^L** re-checks the screen size (for example after
switching SyncTERM to 132x37 mid-session).

### Canvas size

Press **Esc** until the main menu opens, then **C Canvas size**. Type a
width (1-400) and a height (1-2000), or pick a preset: 80x25, 80x50,
132x37, 160x50, or **E Fit the screen**, which fills all the space
beside the toolbox. If the new size would cut off part of your drawing,
ANetDRAW warns you and needs a second Enter before it crops. Resizing
clears the undo history.

On a screen wide enough for the toolbox, each new drawing (at the
start, and with **New**) first asks which canvas you want:

- **A** 80x25, the classic size every caller's screen can show (Enter or
  Esc picks it)
- **B** the space beside the toolbox (93x37 on a 132x37 screen)
- **C** your whole screen (132x37), with the toolbox tucked away; **^W**
  brings it back

A classic drawing starts at 80x25 and grows downward as you move below the
bottom. Once you set a size, the canvas stays that size, like in other
drawing programs. `--width N` starts a wider canvas. When the canvas is
wider than the screen, the view scrolls sideways.

### Toolbox

On wide screens, **^W** (or the **hide** button at the top of the
toolbox) folds the toolbox away. A "TOOLS" tab on the divider brings it
back, and clicking the divider line also works. The toolbox also steps
aside when the canvas is too wide to leave room for it.

```
/path/to/anetdraw -D%Pdoor32.sys --cols 132 --rows 37
```

### Saving, and the sysop

Press **Esc** until the main menu opens, then choose **N**ew, **O**pen,
**S**ave, **A** (Save as), or **I** (SAUCE info: title, author, group).
The same menu has **D**ownload, **B**rowse gallery, and **P**ublish,
described below.
If you have unsaved changes, ANetDRAW asks whether to save them before
you start a new drawing, open another, or quit.

**Save as** (and the first save of a new drawing) asks for TheDraw's
save options:

- **Clear the screen first.** The file starts by clearing the screen,
  so it shows on a clean page.
- **Display speed.** Full speed, or 300 up to 115200 bps, so the art
  draws itself like it would over a modem. It uses the standard ANSI
  speed sequence (DECSCS, `CSI 0;n*r`), not ANetBBS's @-codes, so it works
  on any BBS. SyncTERM and most BBS terminals honor it; others just show
  the art at once. The file puts the terminal back to full speed at the
  end.

The options are kept in the file. Reopening it and pressing plain
**Save** keeps them without asking.

- **Callers** save into their own private folder,
  `<data>/users/<number>_<name>/`. They only type a plain file name
  (letters, digits, space, `_ - .`; `.ans` is added). They can't pick
  another folder or reach anything outside their own.
- **The sysop** gets a real file browser: any folder (`..` goes up),
  any file name. The sysop is anyone running locally (`-L`), plus any
  caller whose dropfile security level is at least `--sysop-level N`.
  ANetBBS writes 200 for admins, so on ANetBBS use `--sysop-level 200`.
  Without that option, nobody on the BBS gets the file browser.

Files are standard `.ans` with a SAUCE record (width, height, iCE flag,
font, title, author, group, date), written the way Moebius writes them,
so they open in Moebius, PabloDraw, and SyncTERM exactly as drawn.
Saves write a temporary file and then rename it, so a hangup mid-save
can't leave a half-written file. ANetDRAW can open `.ans` files from
other editors too, including older TheDraw-style files.

### TheDraw fonts (big text)

**^T** then **T** opens the TheDraw font browser. Type your text, pick a
font (**Tab** moves to Find, which searches font and file names), and
watch the preview. **Enter** picks it up to place like a paste: move it
with the arrows or mouse, press **Enter** to stamp it, **T** to switch
transparency, and **Esc** when you're done. Color fonts keep their own
colors. Block and outline fonts use your current colors. Fonts with no
lower-case letters use their capitals.

The fonts come as a **separate download**, the ANetDRAW font pack
(`ANetDRAW-fonts.zip`): 3,716 TheDraw fonts. Unzip it in the
door's folder so they land in `fonts/`, or point to them with
`--fonts DIR`. Any `.tdf` file you add there shows up too. See
`fonts/FONTS.md` for where the fonts come from and the credits. The pack
is built from a TDF collection by `util/make_fontpack.py`, which
removes duplicates and leaves out any font whose data reserves its
rights.

### Downloading (ZMODEM)

**D** in the main menu sends the current drawing straight to the
caller's terminal by ZMODEM, without leaving the door. SyncTERM, NetRunner,
Qodem, and other ZMODEM terminals start the download by themselves. On a
terminal without ZMODEM, press **Esc** to cancel. The sender is built into
the door (no `sz` needed) and works over telnet, SSH, and ANetBBS's PTY
doors. It uses CRC-32 and 1K blocks, resumes after line errors, and
stays patient while the terminal asks its user a question, such as
SyncTERM's "Overwrite?" prompt. `--trace` logs each ZMODEM step.

### The gallery

The gallery is one shared folder, `<data>/gallery/`, that every caller
can browse.

- **P** (Publish to gallery) asks for a title, artist, and group, then
  adds the drawing for everyone to see. Publishing the same title again
  asks before it replaces the earlier one.
- **B** (Browse gallery) lists every piece, newest first, with its
  title, artist, size, and date. **Enter** (or a second click) views a
  piece full screen. **D** downloads it by ZMODEM. **O** opens a copy to
  work on, and saving the copy goes to your own folder. **R** removes a
  piece, but only for its owner and the sysop.

A published piece is stored as `<number>_<name>-<title>.ans`, which
records whose it is. To add pieces yourself, just copy `.ans` files
into the folder: titles and artists come from each file's SAUCE record.
Releases ship with two sample pieces in `anetdraw_data/gallery/`
(`util/make_samples.py` makes them). If you use `--data` somewhere
else, copy those samples across.

### Options

| Option | Meaning |
|---|---|
| `--cols N` / `--rows N` | Force the screen size |
| `--width N` | Start new drawings N columns wide (default 80) |
| `--data DIR` | Where callers' folders and the gallery live (default `anetdraw_data`) |
| `--sysop-level N` | Callers at this security level or above get the sysop file browser |
| `--no-mouse` | Don't turn on terminal mouse reporting |
| `--fonts DIR` | Where the TheDraw fonts are (default `fonts`) |
| `--console` | Windows `-L`: use OpenDoors' console window instead of ANetDRAW's own |
| `--trace FILE` | Log every input event and screen update with millisecond timestamps (for diagnosing lag) |

`util/syncterm_local.sh` runs the door locally inside SyncTERM, with no
BBS involved, for testing on your own machine.

## Controls

Press **^O** in the door for a help screen and **^T** for the tools menu.

### Everywhere

| Key | Action |
|---|---|
| Arrows, Home/End, PgUp/PgDn | Move the cursor (the canvas grows downward as you go) |
| F1-F10 | Place a glyph from the current F-key set (Draw tool), or pick it as the brush (other tools) |
| ^A then 1-9, 0 | Same as F1-F10, for terminals that don't send F-keys |
| ^N / ^P (or F12 / F11) | Next / previous F-key set (16 sets, the PabloDraw/Moebius defaults) |
| ^F / ^B | Next foreground / background color |
| ^E | Turn iCE colors on or off (16 background colors vs. blinking) |
| ^U | Pick up the colors under the cursor |
| ^R | Mirror mode: off, left/right, top/bottom, both |
| ^T | Tools menu |
| ^K | Color picker: foreground across, background down; arrows move, Enter picks both |
| ^G | Character picker (all 256 CP437 glyphs): Enter makes it the brush, F1-F10 put it in that slot of the current F-key set |
| ^Z / ^Y | Undo / redo. Each keypress is one step, so a whole fill, paste, or shape undoes at once. Moving the cursor doesn't clear redo; a new edit does |
| ^V | Paste the clipboard |
| Tab | Change the current tool's option (shown in the status bar) |
| ^L | Redraw the screen |
| Esc | Back out one step: paste, then mark, then tool, then the **main menu** (canvas size, toolbox, colors, characters, tools, help, quit) |
| ^W | Show or hide the toolbox (wide screens) |

### Mouse

ANetDRAW asks the terminal for mouse reporting (xterm button-event
tracking with SGR coordinates). SyncTERM, xterm, PuTTY, Windows
Terminal, and most modern terminals support it, over the BBS and in
Linux `-L` mode. Terminals without mouse support just ignore the
request.

| Mouse | Action |
|---|---|
| Left button | Draw, Pixel, Shade: paint as you drag (a whole drag undoes as one step). Line, Box, Ellipse: press at one corner, drag, release to draw. Select: drag to mark a block. Fill: click to fill. While pasting: click to stamp |
| Right button | Pick up the colors and glyph under the pointer. In the Shade tool, lighten instead. In the Select tool, open the **Block menu**: Copy, Cut, Paste, Delete, Fill with brush, Move, Select all |
| Wheel | Scroll the canvas |
| Status bar | Click the color sample for the color picker, the brush for the character picker, the tool name or `^T Tools` for the tools menu, an F-key glyph to make it the brush, `S06` for the next set, `^O Help` for help |
| Pickers and menus | Click a color, glyph, or tool to choose it. Right-click to cancel |

The Pixel tool paints the half (top or bottom) shown in the status bar;
Up/Down switch halves. A sysop can turn mouse reporting off with
`--no-mouse`. The mouse works in local mode too: in the terminal on
Linux, and in ANetDRAW's own window on Windows (see above).

### Tools (^T, then the letter)

| Tool | How it works | Tab option |
|---|---|---|
| **D** Draw | Type text; Enter starts a new line; Insert toggles insert mode | Pen: when on, moving the cursor paints the brush |
| **L** Line | Space sets one end, move, Space again to draw it (the line previews as you move) | Brush glyph, or half-block pixels |
| **B** Box | Same as Line | Brush outline, solid brush, single line, double line, pixel outline, pixel solid |
| **E** Ellipse | Same as Line (corner to corner) | Brush outline, solid brush, pixel outline, pixel solid |
| **F** Fill | Space flood-fills the area under the cursor | Fill glyph and color, or recolor only |
| **S** Shade | Space makes the cell denser (space, light, medium, dark shade, full block); Backspace makes it lighter | Pen |
| **P** Pixel | Paints half-block pixels (80x50 resolution); Up/Down move half a cell; Space paints, Backspace erases | Pen |
| **C** Colorize | Recolor without redrawing: drag the mouse (or press Space) over the art and it takes the current colors while the characters stay. **Tab**: FG+BG / FG only / BG only. Right-click picks up colors. Mirror mode applies | |
| **K** Select | Drag with the mouse to mark a block, or press Space at one corner and move to the other. Then **C** copy, **X** cut, **M** move, **D** or **Delete** delete, **F** fill with the brush, **R** recolor it (Colorize for the whole block). **A** marks everything, **P** or **^V** pastes. Right-click opens the same commands as a menu | |

In every tool except Draw, typing a character makes it the brush.
While pasting, arrows move the block, Enter stamps it (you can stamp it
more than once), **H**/**V** flip it, **T** turns transparency on or off
(blank cells don't overwrite), and Esc finishes.

^C, ^S, and ^Q are not used, because BBS software or flow control
between the caller and the door can swallow them.
