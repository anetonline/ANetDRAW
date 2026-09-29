# ANetDRAW Sysop Guide

ANetDRAW is a native door: one program, no runtime, nothing to install
beyond unpacking it. It reads `DOOR32.SYS`, `DOOR.SYS`, `DORINFO1.DEF`
or `CHAIN.TXT`, like any OpenDoors door.

## Pick the archive

| BBS machine | Archive |
|---|---|
| Linux x86-64 (any distro) | `*_linux-x64-static.tar.gz` |
| Raspberry Pi / ARM64 Linux | `*_linux-arm64-pi.tar.gz` |
| Windows 64-bit | `*_win64.zip` |
| Windows 32-bit | `*_win32.zip` |

The Linux and Pi builds are statically linked: no shared libraries and
no particular glibc. The Windows builds need only the standard system
DLLs.

Each archive has the program, `README.md`, `docs/`, `FILE_ID.DIZ`, and
`anetdraw_data/gallery/` with two sample pieces.

The **font pack**, `ANetDRAW-fonts.zip`, is a separate download. Unzip
it in the door's folder so the fonts land in `fonts/`.

## Install

1. Unpack the archive into its own folder, e.g. `/sbbs/xtrn/anetdraw`,
   `/mystic/doors/anetdraw` or `C:\BBS\doors\anetdraw`.
2. On Linux or Pi: `chmod +x anetdraw`.
3. Unzip the font pack in the same folder (optional, but the Text tool
   needs it).
4. Add the door to your BBS (below). **Start it in the door's own
   folder:** `anetdraw_data/` and `fonts/` are found relative to the
   working directory, unless you give `--data` and `--fonts`.
5. Try it once locally: `./anetdraw -L` (or double-click `anetdraw.exe`
   on Windows).

### Synchronet

In SCFG → External Programs → Online Programs (Doors), add a program:

| Setting | Value |
|---|---|
| Start-up Directory | `/sbbs/xtrn/anetdraw` |
| Command Line | `/sbbs/xtrn/anetdraw/anetdraw -D %f` |
| Native Executable | Yes |
| I/O Method | Socket |
| BBS Drop File Type | DOOR32.SYS |

`%f` is the path of the drop file Synchronet writes for the node. On
Windows use `anetdraw.exe` and a Windows path.

### Mystic

Linux Mystic runs doors with their input and output redirected, and
needs `DOOR.SYS` (a `DOOR32.SYS` door doesn't work there). Use menu
command **DD**, which writes `DOOR.SYS` in upper case:

```
CD</mystic/doors/anetdraw> /mystic/doors/anetdraw/anetdraw -D %PDOOR.SYS
```

Windows Mystic uses menu command **D3** (DOOR32.SYS, lower case):

```
CD<c:\mystic\doors\anetdraw> c:\mystic\doors\anetdraw\anetdraw.exe -D %Pdoor32.sys
```

`%P` is the node's temp folder, and `CD<...>` starts the door in its own
folder.

### ANetBBS

Admin → Door Games → Add Game:

| Field | Value |
|---|---|
| Type | `door_native` |
| Executable path | `/path/to/anetdraw` |
| Command line arguments | `-D %Pdoor32.sys --sysop-level 200` |

ANetBBS starts native doors in the program's folder. `--sysop-level 200`
gives ANetBBS admins the sysop file browser (see Security below).

### Other BBS software

Any door driver that writes one of the drop files works:

```
anetdraw -D path/to/door32.sys
```

## Options

| Option | Meaning |
|---|---|
| `-D FILE` | The drop file (OpenDoors) |
| `-L` | Local mode, no BBS (see [STANDALONE.md](STANDALONE.md)) |
| `--sysop-level N` | Callers at this security level or above are sysops |
| `--data DIR` | Where callers' folders, the gallery and the wall live (default `anetdraw_data`) |
| `--fonts DIR` | Where the TheDraw fonts are (default `fonts`) |
| `--cols N` / `--rows N` | Force the screen size |
| `--width N` | Start new drawings N columns wide (default 80) |
| `--wall WxH` | Size of a new shared wall (default `80x25`) |
| `--no-mouse` | Don't turn on terminal mouse reporting |
| `--console` | Windows `-L`: OpenDoors' console window instead of ANetDRAW's own |
| `--trace FILE` | Log input, screen updates and ZMODEM steps, with timestamps in milliseconds |

### Screen size

ANetDRAW works on screens from 80x10 to 400x200. It finds the caller's
size from, in order: `--cols`/`--rows`; the drop file (only CHAIN.TXT
and BBSDEV.DRP carry it); asking the terminal (a cursor-position
report); and 80x24 if nothing answers. Callers can press **^L** after
changing their terminal's size.

## Security

- **Callers never get file browsing.** Each caller saves only into their
  own folder, `anetdraw_data/users/<number>_<name>/`, with a plain file
  name that ANetDRAW cleans up. They can't choose a folder or reach
  anything outside their own.
- **The sysop** gets a real file browser (any folder, any file name) and
  picks images for import from disk. The sysop is anyone running locally
  (`-L`), plus any caller whose drop-file security level is at least
  `--sysop-level N`. **Without that option, nobody on the BBS is sysop.**
- Uploads are limited to 8 MB and one file, never overwrite anything, and
  land in the caller's own folder. Uploaded images are imported, not
  kept. Image sizes are checked before decoding (up to 8192x8192).

## Data folder

```
anetdraw_data/
  users/<number>_<name>/   each caller's drawings and F-key sets
  gallery/                 published pieces (plain .ans files)
  wall/                    the shared wall
```

Back up the whole folder to keep everything.

- **Gallery.** Each piece is an `.ans` file, named
  `<number>_<name>-<title>.ans` so ANetDRAW knows whose it is. You can
  copy `.ans` files in yourself; titles and artists come from each file's
  SAUCE record. If you use `--data` elsewhere, copy the samples from the
  archive's `anetdraw_data/gallery/`.
- **Shared wall.** Doors on different nodes share it through the files in
  `wall/`, with an OS file lock, so it works on every BBS. To start a
  fresh wall, delete `wall/` while nobody's on it. `--wall WxH` sets the
  size of the next new wall.
- **Fonts.** Any `.tdf` file you add to `fonts/` shows up in the browser.

## Transfers

Download and upload use ZMODEM built into the door; `sz`/`rz` aren't
needed. It works over telnet, SSH, rlogin, Synchronet sockets and PTY
doors. Uploads ask the caller's terminal to escape every control byte,
because some BBS software alters them on the way to a door: ANetBBS
drops 0x03 bytes, and Synchronet drops a line feed or NUL after a
carriage return. Keep that in mind for other doors that take uploads.

## Troubleshooting

| Problem | What to check |
|---|---|
| Black screen, nothing happens | The command line's `-D` path, and that the drop file type matches (DOOR.SYS for Linux Mystic) |
| "Unable to read door information (drop) file" | No drop file was found: check `-D` and the BBS's drop file settings |
| Toolbox missing | It needs 39 columns beside the canvas (119 for an 80-column drawing). Callers can switch to 132x37 and press ^L, or you can set `--cols`/`--rows` |
| Font browser empty | Unzip `ANetDRAW-fonts.zip` in the door's folder, or use `--fonts DIR` |
| Gallery empty | Copy the sample `.ans` files into `anetdraw_data/gallery/` |
| Mouse does odd things | The terminal doesn't support SGR mouse reporting: use `--no-mouse` |
| Download or upload never starts | The terminal needs ZMODEM (SyncTERM, NetRunner, Qodem...). Esc cancels |
| Anything else | Run with `--trace /tmp/anetdraw.log` and send the log |
