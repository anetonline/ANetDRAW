# ANetDRAW User Guide

ANetDRAW is an ANSI and ASCII art editor in the spirit of TheDraw,
ACiDDraw, Moebius and PabloDraw. It works the same whether you call in
to a BBS or run it on your own computer (see [STANDALONE.md](STANDALONE.md)).

Press **^O** in the editor for a help screen, **^T** for the tools menu,
and **Esc** (until the main menu opens) for everything else.

- [The screen](#the-screen)
- [Keys](#keys)
- [Mouse](#mouse)
- [Tools](#tools)
- [Blocks: select, copy, paste](#blocks-select-copy-paste)
- [Canvas size](#canvas-size)
- [Saving and opening](#saving-and-opening)
- [File formats and BBS color codes](#file-formats-and-bbs-color-codes)
- [TheDraw fonts (big text)](#thedraw-fonts-big-text)
- [F-key sets](#f-key-sets)
- [Downloading and uploading (ZMODEM)](#downloading-and-uploading-zmodem)
- [Image import](#image-import)
- [The gallery](#the-gallery)
- [The shared wall](#the-shared-wall)

## The screen

The canvas fills the screen. The bottom line is the **status bar**: the
cursor position as **(column,row)**, the current colors, the brush
glyph, the tool and its option, the F-key glyphs and messages.

On screens with room to spare (132x37 in SyncTERM, for example) a
**toolbox** docks at the right edge. It has:

- FG/BG color palettes (left-click sets the foreground, right-click the
  background)
- the tools in two columns, with the current tool's option
- the F-key glyphs, with arrows to change the set
- mirror mode, undo/redo counts, and who's here on the shared wall
- in the Select tool, the block buttons (**Copy Cut Del Fill Move
  Recolor**, or **Paste** and **All**)
- on screens 33 or more rows tall, a **File** section (New, Open, Save,
  Save as, Gallery, Publish, Download) and a **Canvas** section (its
  size with a **Size** button, Undo, Redo, Fonts, Colors, Chars)

Everything on it is clickable. **^W**, or the **hide** button, folds the
toolbox away; the "TOOLS" tab on the divider brings it back. Past the
canvas's right edge you see a dim dotted "desk".

If you change your terminal's size during a session (for example,
switching SyncTERM to 132x37), press **^L** and ANetDRAW lays itself out
again.

## Keys

| Key | Action |
|---|---|
| Arrows, Home/End, PgUp/PgDn | Move the cursor (a classic canvas grows downward as you go) |
| F1-F10 | Place a glyph from the current F-key set (Draw tool), or pick it as the brush (other tools) |
| ^A then 1-9, 0 | Same as F1-F10, for terminals that don't send F-keys |
| ^N / ^P (or F12 / F11) | Next / previous F-key set |
| ^F / ^B | Next foreground / background color |
| ^E | iCE colors on or off (16 background colors instead of blinking) |
| ^U | Pick up the colors under the cursor |
| ^R | Mirror mode: off, left/right, top/bottom, both |
| ^T | Tools menu |
| ^K | Color picker: foreground across, background down; Enter picks both |
| ^G | Character picker (all 256 CP437 glyphs): Enter makes it the brush; F1-F10 put it in that slot of the current F-key set |
| ^Z / ^Y | Undo / redo. A whole fill, paste, shape or mouse drag undoes as one step |
| ^V | Paste the clipboard |
| ^D | Lines and columns at the cursor: **I** insert a line, **Y** delete the line, **C** insert a column, **X** delete the column |
| Tab | Change the current tool's option |
| ^W | Show or hide the toolbox |
| ^L | Redraw the screen (and re-check its size) |
| ^O | Help |
| Esc | Back out one step: paste, then mark, then tool, then the **main menu** |

^C, ^S and ^Q are never used, because BBS software or flow control can
swallow them.

### The main menu (Esc)

| Key | Action | Key | Action |
|---|---|---|---|
| N | New drawing | O | Open |
| S | Save | A | Save as |
| I | SAUCE info (title, author, group) | C | Canvas size |
| B | Browse the gallery | P | Publish to the gallery |
| D | Download (ZMODEM) | U | Upload (ZMODEM) |
| M | Image import | E | Edit F-keys |
| K | Colors | G | Characters |
| T | Tools | H | Help |
| J | Shared wall (draw together) | W | Show/hide toolbox |
| Q | Quit | | |

If you have unsaved changes, ANetDRAW asks before a new drawing, opening
another, or quitting.

## Mouse

ANetDRAW turns on terminal mouse reporting. SyncTERM, xterm, PuTTY,
Windows Terminal and most modern terminals support it; terminals that
don't just ignore it.

| Mouse | Action |
|---|---|
| Left button | Draw, Pixel, Shade, Colorize: paint as you drag. Line, Box, Ellipse: press at one corner, drag, release. Select: drag to mark a block. Fill: click to fill. While pasting: click to stamp |
| Right button | Pick up the colors and glyph under the pointer. In Shade, lighten instead. In Select, open the **block menu** |
| Wheel | Scroll the canvas |
| Status bar | Click the color sample for the color picker, the brush for the character picker, the tool name for the tools menu, an F-key glyph to make it the brush, the set number for the next set, `^O Help` for help |
| Pickers and menus | Click to choose; right-click cancels |

## Tools

Press **^T**, then the letter, or click the tool in the toolbox.

| Tool | How it works | Tab option |
|---|---|---|
| **D** Draw | Type text; Enter starts a new line; Insert toggles insert mode | Pen: moving the cursor paints the brush |
| **L** Line | Space sets one end, move, Space again draws it (it previews as you move) | Brush glyph, or half-block pixels |
| **B** Box | Same as Line | Brush outline, solid brush, single line, double line, pixel outline, pixel solid |
| **E** Ellipse | Same as Line (corner to corner) | Brush outline, solid brush, pixel outline, pixel solid |
| **F** Fill | Space flood-fills the area under the cursor | Fill glyph and color, or recolor only |
| **S** Shade | Space makes the cell denser (space, ░, ▒, ▓, █); Backspace makes it lighter | Pen |
| **P** Pixel | Half-block pixels, twice the rows (80x50 on an 80x25 canvas). Up/Down move half a cell; Space paints, Backspace erases | Pen |
| **C** Colorize | Recolors without redrawing: drag (or press Space) over the art and it takes the current colors while the characters stay | FG+BG, FG only, BG only |
| **K** Select | Mark a block (see below) | |
| **T** Text | TheDraw fonts (see below) | |

In every tool except Draw, typing a character makes it the brush.
Mirror mode (**^R**) applies to all the painting tools.

## Blocks: select, copy, paste

In the **Select** tool, drag with the mouse to mark a block, or press
Space at one corner and move to the other. Then:

| Key | Action | Key | Action |
|---|---|---|---|
| C | Copy | X | Cut |
| M | Move | D or Delete | Delete |
| F | Fill with the brush | R | Recolor (Colorize the whole block) |
| H | Flip left-right | V | Flip upside-down |
| S | Swap FG/BG | L | Replace one color with another |
| N | Center each row | A | Select all |
| P or ^V | Paste | | |

Flips mirror the glyphs too, so ◄ becomes ► and ▀ becomes ▄.
Right-click opens the same commands as a menu.

While pasting, the arrows (or the mouse) move the block, **Enter** or a
click stamps it (as many times as you like), **H**/**V** flip it, **T**
turns transparency on or off (blank cells don't overwrite), and **Esc**
finishes.

## Canvas size

**Esc → C Canvas size.** Type a width (1-400) and a height (1-2000), or
pick a preset: 80x25, 80x50, 132x37, 160x50, or **E Fit the screen**. If
the new size would cut off part of your drawing, ANetDRAW warns you and
needs a second Enter. Resizing clears the undo history.

On a screen wide enough for the toolbox, each new drawing asks which
canvas you want:

- **A** 80x25, the classic size every screen can show (Enter or Esc)
- **B** the space beside the toolbox (93x37 on a 132x37 screen)
- **C** the whole screen, with the toolbox tucked away (**^W** brings it
  back)

A classic drawing starts at 80x25 and grows downward as you move below
the bottom. Once you set a size, the canvas stays that size. When the
canvas is wider than the screen, the view scrolls sideways.

## Saving and opening

**Save as** (and the first save of a new drawing) asks for:

- **Format.** See the next section. The file name's extension follows it.
- **Clear the screen first.** The file starts by clearing the screen.
- **Display speed.** Full speed, or 300 up to 115200 bps, so the art
  draws itself like it would over a modem. This is the standard ANSI
  speed sequence (DECSCS), which SyncTERM and most BBS terminals honor;
  others just show the art at once. The file puts the speed back to full
  at the end.

These options are kept in the file, so a plain **Save** later keeps them.
**I** edits the SAUCE record: title, author and group.

**Where files go.** On a BBS, your drawings are saved in your own private
folder. You type a plain file name (letters, digits, space, `_ - .`), and
you can't reach anything outside your folder. The sysop, and anyone
running ANetDRAW on their own computer, gets a full file browser instead.

Saves are safe: ANetDRAW writes a temporary file, then renames it, so a
dropped connection can't leave half a file. Files open in Moebius,
PabloDraw and SyncTERM exactly as drawn, and ANetDRAW opens files from
those editors and from TheDraw.

## File formats and BBS color codes

| Format | File | What it is |
|---|---|---|
| ANSI | `.ans` | ANSI art with SAUCE (the default) |
| Plain ASCII | `.asc` | the characters only, no colors |
| BIN | `.bin` | raw character/color pairs (SAUCE BinaryText) |
| XBin | `.xb` | XBin, with the size in its own header |
| PCBoard @X | `.pcb` | `@X<bg><fg>` codes, `@CLS@` to clear |
| Pipe codes | `.pip` | Renegade/Mystic `\|00`-`\|31` codes, `\|CL` to clear |
| Synchronet Ctrl-A | `.msg` | Synchronet's Ctrl-A codes, Ctrl-A L to clear |
| PNG image | `.png` | a picture of the art in the IBM VGA font |

The three BBS formats make display files for a BBS's menus and screens.
They carry no SAUCE record and stop at the last row that has anything on
it. **Open** reads every format except PNG, including `.asc`/`.txt`
files that use BBS color codes instead of ANSI. A plain **Save** and
**Download** keep the drawing's format.

## TheDraw fonts (big text)

**^T T** opens the font browser. Type your text, pick a font (**Tab**
moves to Find, which searches font and file names), and watch the
preview. **Enter** picks it up to place like a paste: move it, press
**Enter** to stamp it, **T** for transparency, **Esc** when you're done.

- Color fonts keep their own colors. Block and outline fonts use your
  current colors.
- Outline fonts can be drawn in any of **TheDraw's 19 outline styles**:
  **Left/Right** step through them in the browser.
- Fonts without lower-case letters use their capitals.

The fonts are a separate download, the ANetDRAW font pack (3,716 fonts).
If the browser is empty, ask your sysop to install it (or see
[STANDALONE.md](STANDALONE.md)).

## F-key sets

F1-F10 place glyphs from the current set, and **^N**/**^P** change sets.
**Esc → E Edit F-keys** shows the set's 10 slots above all 256 glyphs.
Pick a glyph, then press F1-F10 (or 1-0, or click a slot) to put it
there. PgUp/PgDn change sets, and **R** resets a set to its default.
Your sets are saved and come back next time.

## Downloading and uploading (ZMODEM)

**D Download** sends the current drawing to your terminal by ZMODEM, in
its own format, without leaving ANetDRAW. SyncTERM, NetRunner, Qodem and
other ZMODEM terminals start the download by themselves. On a terminal
without ZMODEM, press **Esc** to cancel.

**U Upload** takes one file from your terminal (up to 8 MB). In SyncTERM
the upload box opens by itself; pick the file.

- **A drawing** (any format Open reads) is kept in your folder, never over
  an existing file (it becomes `name-2.ans`), and opens right away.
- **An image** (PNG, JPEG, GIF or BMP) goes to image import.

## Image import

**M Image import** turns a picture into ANSI. Each character holds two
square pixels (a half block) in the 16 colors, with optional dithering.
Choose the width (40 to 160 columns, or the space beside the toolbox),
dithering and iCE colors. On a BBS you upload the picture; on your own
computer you pick it from disk.

## The gallery

The gallery is one shared collection every caller can browse.

- **P Publish** asks for a title, artist and group, then adds your
  drawing. Publishing the same title again asks before it replaces it.
- **B Browse** lists every piece, newest first. **Enter** (or a second
  click) views it full screen, **D** downloads it, **O** opens a copy to
  work on (saved to your own folder), and **R** removes it (its owner and
  the sysop only).

## The shared wall

**J Shared wall** puts you on one canvas that everyone on the BBS can
draw on at the same time, on any node:

- You see each other's work as it happens, and each other's cursors as a
  colored initial.
- The status bar says who joins and leaves.
- The wall is always saved, so there's no "save?" when you leave.
  **Save as** keeps a copy in your own folder.
- New, Open, Upload, Image import and Canvas size are off while you're
  on the wall. **J** again leaves.
