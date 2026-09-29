# ANetDRAW on your own computer

ANetDRAW isn't only a BBS door. It's a complete ANSI art editor you can
run on your own Windows or Linux machine, with no BBS involved. Everything
works: the tools, fonts, formats, image import and gallery. You also get
a full file browser, since you're the sysop of your own machine.

## Windows

Unzip the win64 archive (or win32 for 32-bit Windows) anywhere and
**double-click `anetdraw.exe`**. With no arguments it opens ANetDRAW in
its own window (the same as `anetdraw.exe -L`).

- The window draws with the IBM VGA font, so art looks exactly as it does
  in SyncTERM. The mouse works.
- Resize or maximize it any time, or press **Alt+Enter** for full screen.
  Maximized and full screen pick the biggest zoom that still leaves room
  for the toolbox.
- **Ctrl+Plus** / **Ctrl+Minus** (or Ctrl+mouse wheel) zoom through 50%
  to 600%; **Ctrl+0** goes back to 100%. The title bar shows the zoom.
  Zooming out fits more columns and rows.
- Closing the window asks to save unsaved work.

For a shortcut that opens a wide canvas, use for example
`anetdraw.exe -L --width 132`.

## Linux and Raspberry Pi

Unpack the archive and run it in a terminal:

```
./anetdraw -L
```

ANetDRAW uses the terminal's size, and follows along if you resize or
maximize the window. Mouse support needs a terminal with SGR mouse
reporting (xterm, GNOME Terminal, Konsole, kitty, and most others).
The art shows best in a terminal using a DOS/VGA-style font.

To use SyncTERM as the screen instead (the most authentic look), the
source tree has `util/syncterm_local.sh`, which runs ANetDRAW inside
SyncTERM without a BBS.

## Fonts

Unzip `ANetDRAW-fonts.zip` next to the program, so the fonts land in
`fonts/`, or point at them with `--fonts DIR`. **^T T** opens the font
browser.

## Your files

Your drawings go wherever you save them: the file browser can reach any
folder. ANetDRAW also keeps `anetdraw_data/` in the folder you start it
from (your F-key sets, the gallery and the shared wall). `--data DIR`
puts it somewhere else.

See [USER_GUIDE.md](USER_GUIDE.md) for the editor itself.
