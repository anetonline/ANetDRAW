# ANetDRAW

**The ANSI + ASCII art studio — as a BBS door, or on your own computer.**

ANetDRAW is a full ANSI/ASCII art editor in the spirit of TheDraw,
ACiDDraw, Moebius and PabloDraw. Callers draw right inside your BBS,
with the mouse or the keyboard, and it runs just as well stand-alone on
Windows, Linux and the Raspberry Pi, with no BBS at all.

- Website: <https://a-net-online.lol/ANetDRAW>
- Source: <https://github.com/anetonline/ANetDRAW>
- By StingRay of A-Net Online, <https://a-net.online>

Version 0.2.0, beta.

## Features

- **Drawing tools:** text, F-key glyph sets, lines, boxes, ellipses,
  flood fill, a shading brush, half-block pixels (double the rows) and
  Colorize, which recolors without redrawing. Mirror mode, and up to
  2,000 steps of undo/redo.
- **Mouse everywhere** in SyncTERM and other modern terminals, plus a
  docked toolbox on wide screens (132x37 and up).
- **Blocks:** select with the mouse, copy, cut, move, delete, fill,
  flip, swap colors, replace a color, center, paste with transparency.
  Insert or delete lines and columns.
- **3,716 TheDraw fonts** (a separate download) with a live preview and
  TheDraw's 19 outline styles.
- **Formats:** ANSI with SAUCE, plain ASCII, BIN, XBin, PNG, and BBS
  display files in PCBoard @X, Renegade/Mystic pipe codes and Synchronet
  Ctrl-A. TheDraw-style save options: clear screen and display speed.
- **Image import:** turn a PNG, JPEG, GIF or BMP into half-block ANSI,
  with dithering.
- **ZMODEM** download and upload, built in: no `sz`/`rz` needed.
- **Gallery:** callers publish their art for everyone to browse, view
  and download.
- **Shared wall:** a canvas every node draws on together, live, with
  each other's cursors.
- **Any screen size** from 80x10 to 400x200. On Windows, its own window
  with the IBM VGA font, zoom and full screen.
- **Safe for callers:** each caller has a private folder and never gets
  file browsing; sysop powers only for the security level you choose.

## Quick start

**On your own computer:** Windows, double-click `anetdraw.exe`. Linux
or Pi, run `./anetdraw -L` in a terminal. See
[docs/STANDALONE.md](docs/STANDALONE.md).

**On a BBS:** unpack into its own folder and add a door that starts in
that folder:

```
/path/to/anetdraw -D /path/to/door32.sys --sysop-level 200
```

[docs/SYSOP_GUIDE.md](docs/SYSOP_GUIDE.md) has step-by-step setup for
Synchronet, Mystic, ANetBBS and others.

For the fonts, unzip `ANetDRAW-fonts.zip` in the same folder.

## Documentation

| | |
|---|---|
| [docs/USER_GUIDE.md](docs/USER_GUIDE.md) | Using the editor: keys, mouse, tools, blocks, fonts, formats, transfers, gallery, wall |
| [docs/STANDALONE.md](docs/STANDALONE.md) | Running it on Windows, Linux or a Pi without a BBS |
| [docs/SYSOP_GUIDE.md](docs/SYSOP_GUIDE.md) | Installing it on a BBS: setup, options, security, data, troubleshooting |
| [docs/BUILDING.md](docs/BUILDING.md) | Building from source and running the tests |
| [docs/CREDITS.md](docs/CREDITS.md) | Credits and third-party licenses |
| [RELEASE.md](RELEASE.md) | Release notes |
