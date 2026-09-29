#!/usr/bin/env python3
"""Builds the sample pieces the gallery starts with (samples/*.ans, with
SAUCE), and a PNG preview of each (--png):

  anetdraw_splash.ans  the intro splash, as a plain 80x25 iCE piece
  harbor_moon.ans      a half-block night scene

make_release.sh copies samples/ into the package's anetdraw_data/gallery.
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import make_splash  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "samples")
W, H = 80, 25

BLACK, BLUE, GREEN, CYAN, RED, MAGENTA, BROWN, LGRAY = range(8)
DGRAY, LBLUE, LGREEN, LCYAN, LRED, LMAGENTA, YELLOW, WHITE = range(8, 16)


def cp437(ch):
    return ch.encode("cp437")


def encode(cells, title, author, group, date="20260929"):
    """ANSI in the Moebius style (same as the door's encoder): full rows
    end without CR LF, bold = bright foreground, blink = bright background
    (shown as bright with the SAUCE iCE flag)."""
    out = bytearray(b"\x1b[0m")
    cur = None
    for row in cells:
        n = len(row)
        while n and row[n - 1][0] == " " and row[n - 1][2] == 0:
            n -= 1
        for ch, fg, bg in row[:n]:
            if (fg, bg) != cur:
                p = ["0"]
                if fg >= 8:
                    p.append("1")
                if bg >= 8:
                    p.append("5")
                p.append(str(30 + make_splash.PC_TO_ANSI[fg & 7]))
                p.append(str(40 + make_splash.PC_TO_ANSI[bg & 7]))
                out += ("\x1b[" + ";".join(p) + "m").encode()
                cur = (fg, bg)
            out += cp437(ch)
        if n < W:
            out += b"\r\n"
    art_len = len(out)
    rec = bytearray(b"SAUCE00")
    rec += title.encode("cp437")[:35].ljust(35)
    rec += author.encode("cp437")[:20].ljust(20)
    rec += group.encode("cp437")[:20].ljust(20)
    rec += date.encode()[:8]
    rec += art_len.to_bytes(4, "little")
    rec += bytes([1, 1])                      # Character / ANSi
    rec += W.to_bytes(2, "little") + H.to_bytes(2, "little") + bytes(4)
    rec += bytes([0])                         # no comment block
    rec += bytes([1 | (1 << 1) | (2 << 3)])   # iCE, 8px letter spacing, legacy aspect
    rec += b"IBM VGA".ljust(22, b"\0")
    assert len(rec) == 128
    return bytes(out) + b"\x1a" + bytes(rec)


class Cells:
    def __init__(self, cells):
        self.cells = cells


def from_pixels(pix):
    """80x50 palette pixels -> 80x25 half-block cells."""
    cells = []
    for y in range(H):
        row = []
        for x in range(W):
            t, b = pix[2 * y][x], pix[2 * y + 1][x]
            if t == b:
                row.append(("█", t, BLACK) if t else (" ", LGRAY, BLACK))
            elif t >= 8 and b < 8 or b == 0:
                row.append(("▀", t, b))
            else:
                row.append(("▄", b, t))
        cells.append(row)
    return cells


def bayer(x, y):
    m = [[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]
    return (m[y & 3][x & 3] + 0.5) / 16.0


class Scene:
    """Cells plus a half-block pixel layer on top: px() paints one of the
    two pixels of a cell, turning it into a ▀/▄ cell that keeps the
    other half's color (a shade-ramp cell counts as its background)."""
    def __init__(self):
        self.cells = [[(" ", LGRAY, BLACK) for _ in range(W)] for _ in range(H)]

    def put(self, x, y, ch, fg, bg=BLACK):
        if 0 <= x < W and 0 <= y < H:
            self.cells[y][x] = (ch, fg, bg)

    def halves(self, x, y):
        ch, fg, bg = self.cells[y][x]
        if ch == "▀":
            return fg, bg
        if ch == "▄":
            return bg, fg
        if ch == "█":
            return fg, fg
        return bg, bg  # space, shade glyphs, stars: the background shows

    def px(self, x, y, c):
        cx, cy = x, y // 2
        if not (0 <= cx < W and 0 <= cy < H):
            return
        top, bot = self.halves(cx, cy)
        if y & 1:
            bot = c
        else:
            top = c
        if top == bot:
            self.cells[cy][cx] = ("█", top, BLACK) if top else (" ", LGRAY, BLACK)
        elif bot < 8 or top >= 8 and bot >= 8 and False:
            self.cells[cy][cx] = ("▀", top, bot)
        else:
            self.cells[cy][cx] = ("▄", bot, top)


def harbor_moon():
    sc = Scene()
    horizon = 15  # cell row of the sea's far edge
    # sky: black overhead easing into blue, a violet glow on the horizon
    ramp = {7: ("░", BLUE, BLACK), 8: ("░", BLUE, BLACK), 9: ("▒", BLUE, BLACK), 10: ("▒", BLUE, BLACK),
            11: ("▓", BLUE, BLACK), 12: ("█", BLUE, BLACK), 13: ("░", MAGENTA, BLUE),
            14: ("▒", MAGENTA, BLUE)}
    for y, (ch, fg, bg) in ramp.items():
        for x in range(W):
            sc.put(x, y, ch, fg, bg)
    # stars, in the dark part of the sky
    for x, y, ch, c in [(3, 1, "·", LGRAY), (9, 4, "*", WHITE), (14, 2, "∙", WHITE), (21, 6, "·", DGRAY),
                        (26, 1, "+", LCYAN), (31, 4, "·", LGRAY), (37, 2, "*", WHITE), (43, 5, "∙", LGRAY),
                        (47, 1, "·", DGRAY), (6, 7, "·", LGRAY), (18, 8, "∙", LGRAY),
                        (33, 7, "·", WHITE), (74, 2, "*", WHITE), (78, 6, "·", LGRAY), (71, 8, "∙", LGRAY),
                        (40, 9, "·", LGRAY), (12, 10, "·", LBLUE), (76, 10, "·", LBLUE)]:
        if y < 7:
            sc.put(x, y, ch, c, BLACK)
    # a golden moon: bright upper right, soft brown maria, darker limb
    mx, my, r = 60.0, 9.0, 6.4  # pixel coordinates
    for y in range(0, 20):
        for x in range(51, 70):
            cov = 0
            for sy in range(4):
                for sx in range(4):
                    dx = (x + (sx + .5) / 4 - .5 - mx) / r
                    dy = (y + (sy + .5) / 4 - .5 - my) / r
                    cov += dx * dx + dy * dy <= 1
            if cov < 6:
                continue
            nx, ny = (x - mx) / r, (y - my) / r
            d2 = nx * nx + ny * ny
            spots = [(-0.35, 0.15, 0.24), (-0.05, 0.45, 0.14)]
            maria = any((nx - sx) ** 2 + (ny - sy) ** 2 < sr * sr for sx, sy, sr in spots)
            if cov < 12 or d2 > 0.80:
                c = BROWN
            elif maria:
                c = BROWN
            elif nx - ny > 0.55 and d2 < 0.62:
                c = WHITE
            else:
                c = YELLOW
            sc.px(x, y, c)
    # far hills, black against the glow, with a few harbour lights
    for x in range(W):
        h = 1.6 + 1.3 * math.sin(x * 0.10 + 0.6) + 0.9 * math.sin(x * 0.27 + 2.0) + 0.4 * math.sin(x * 0.61)
        top = int(round(2 * horizon - max(1.0, h)))
        for y in range(top, 2 * horizon):
            sc.px(x, y, BLACK)
        if x % 6 == 2 and (x * 7) % 5 < 2:
            sc.px(x, 2 * horizon - 1, YELLOW)
    # the sea: swells in blue and black, the moon's path glittering toward us
    for y in range(2 * horizon, 2 * H - 2):
        depth = (y - 2 * horizon) / (2 * H - 2 * horizon)
        spread = 1.8 + depth * 10.0
        for x in range(W):
            swell = math.sin(x * 0.30 + y * 1.9) + 0.5 * math.sin(x * 0.85 - y * 0.7)
            c = BLUE if swell > 0.35 - depth * 0.9 else BLACK
            d = abs(x - mx) / spread
            glint = math.sin(x * 1.37 + y * 2.9) + math.sin(x * 0.53 - y * 1.13)
            if d < 1.0 and glint > 0.3 + d * 1.2:
                c = WHITE if d < 0.25 and glint > 1.1 else (YELLOW if d < 0.65 else BROWN)
            elif d < 1.7 and glint > 1.3 + d * 0.5:
                c = LBLUE if y < 2 * horizon + 6 else BLUE
            sc.px(x, y, c)
    # a sailboat, dark against the moon's path; the moonlit edge of the sail
    sail = ["      #", "     ##", "    ###", "   ####", "  #####", " ######", "      #"]
    hull = ["############", " ########## "]
    bx, by = 54, 2 * horizon + 2
    for j, line in enumerate(sail):
        for i, ch in enumerate(line):
            if ch == "#":
                sc.px(bx + i, by + j, LGRAY if i == len(line) - 1 and j < 6 else DGRAY)
    for j, line in enumerate(hull):
        for i, ch in enumerate(line):
            if ch == "#":
                sc.px(bx - 3 + i, by + len(sail) + j, BLACK)
    # signature on the last row
    sig = " harbor moon \xb7 anetdraw "
    for i, ch in enumerate(sig):
        sc.put(W - 1 - len(sig) + i, H - 1, ch, DGRAY, BLACK)
    return sc.cells


def main():
    os.makedirs(OUT, exist_ok=True)
    splash = make_splash.compose().cells  # 80x24: one blank row below
    splash = splash + [[(" ", LGRAY, BLACK)] * W]
    make_splash.H = H  # (render_png sizes its image from these)
    pieces = [
        ("anetdraw_splash.ans", splash, "ANetDRAW", "StingRay", "A-Net Online"),
        ("harbor_moon.ans", harbor_moon(), "Harbor Moon", "ANetDRAW", "A-Net Online"),
    ]
    for name, cells, title, author, group in pieces:
        assert len(cells) == H and all(len(r) >= W - 1 for r in cells)
        cells = [list(r) + [(" ", LGRAY, BLACK)] * (W - len(r)) for r in cells]
        data = encode(cells, title, author, group)
        with open(os.path.join(OUT, name), "wb") as f:
            f.write(data)
        print("wrote samples/%s (%d bytes)" % (name, len(data)))
        if "--png" in sys.argv:
            png = os.path.join(OUT, name.replace(".ans", ".png"))
            make_splash.render_png(Cells(cells), png)
            print("wrote", os.path.relpath(png, ROOT))


if __name__ == "__main__":
    main()
