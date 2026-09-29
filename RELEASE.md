# ANetDRAW v0.2.0 — second private beta: formats, uploads, wall, docs (September 2026)

ANetDRAW, the ANSI/ASCII art editor that runs as a BBS door or stand-alone (C / OpenDoors; Linux, Raspberry Pi and Windows).

## New since v0.1.0

- Insert/delete lines and columns (^D).
- BBS color-code formats: PCBoard @X, Renegade/Mystic pipe codes,
  Synchronet Ctrl-A (save and open), plus ASCII, BIN, XBin and PNG.
- ZMODEM upload, working behind ANetBBS and Synchronet telnet.
- Image import (PNG/JPEG/GIF/BMP to half-block ANSI).
- Block extras: flips, FG/BG swap, color replace, centering.
- F-key set editor (saved per caller) and TheDraw's 19 outline styles.
- The shared wall: one canvas every node draws on together.
- Windows: double-clicking `anetdraw.exe` opens the editor.
- Full documentation in `docs/`: user guide, stand-alone use, sysop
  setup for Synchronet/Mystic/ANetBBS, building, credits.

## Everything in this build

- Drawing tools: text, F-key glyph sets (editable, saved per caller),
  line, box, ellipse, fill, shade, half-block pixels, Colorize; mirror
  mode; undo/redo.
- Mouse support over the BBS and locally; a docked, collapsible toolbox
  on wide screens; any screen from 80x10 to 400x200.
- Blocks: mouse selection, right-click block menu, copy/cut/move/delete/
  fill, flips, FG/BG swap, color replace, centering, transparent paste.
  Insert/delete lines and columns (^D).
- TheDraw fonts (separate font pack, 3,716 fonts) with TheDraw's 19
  outline styles.
- Formats: ANSI + SAUCE, ASCII, BIN, XBin, PNG, PCBoard @X, pipe codes,
  Synchronet Ctrl-A. TheDraw-style save options (clear screen, display
  speed).
- Image import (PNG/JPEG/GIF/BMP to half-block ANSI).
- ZMODEM download and upload built in. Uploads work behind ANetBBS
  (which drops 0x03 bytes) and Synchronet telnet (which drops LF/NUL
  after CR) by asking the terminal to escape control bytes.
- Shared gallery and a live multi-node shared wall.
- Windows: its own window with the IBM VGA font, zoom (50-600%), full
  screen; double-clicking `anetdraw.exe` opens the editor.

## Known limits

- The Raspberry Pi build hasn't been run on real hardware yet.

---

# ANetDRAW v0.1.0 — first private beta (September 2026)

First test build: drawing tools, mouse, TheDraw fonts, SAUCE save/open with TheDraw-style options, ZMODEM downloads, the gallery, and the Windows local window.
