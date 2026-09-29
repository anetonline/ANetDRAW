#!/usr/bin/env python3
"""Generates ANetDRAW's intro splash screen.

Writes:
  src/splash_art.c   -- the splash as a C string of CP437 bytes (\\xNN
                        escapes only, so the source stays pure ASCII)
  util/splash.ans    -- the same bytes, for viewing in an ANSI viewer
  util/splash.png    -- a cell-accurate preview render (optional, --png)

Original artwork: a hand-drawn pixel wordmark in half blocks -- cyan
"ANet", one paint color per letter on "DRAW" with paint drips, a white
glint on every top edge, drop shadow down-right, light from the top.

Screen facts it's built around (see include/render.h): every row is
positioned with CUP, never a newline, so SyncTERM's immediate wrap
can't band it; column 80 is never painted; everything fits 24 rows.
Rerun after editing: python3 util/make_splash.py [--png]
"""
import os
import sys

from PIL import Image, ImageDraw, ImageFont  # preview only

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

W, H = 80, 24          # screen cells; column 80 is never written
WORD = "ANetDRAW"

# PC color numbers
BLACK, BLUE, GREEN, CYAN, RED, MAGENTA, BROWN, LGRAY = range(8)
DGRAY, LBLUE, LGREEN, LCYAN, LRED, LMAGENTA, YELLOW, WHITE = range(8, 16)


# ---------------------------------------------------------------- wordmark
#
# Hand-drawn 14-pixel-tall letters (7 character rows via half blocks),
# 2-pixel strokes, 2-pixel gaps. Rasterizing a real font down to this
# size came out mushy (e and t ran together), so every pixel is placed
# by hand instead.

LETTERS = {
    "A": ["..####..",
          ".######.",
          "###..###",
          "##....##",
          "##....##",
          "##....##",
          "##....##",
          "########",
          "########",
          "##....##",
          "##....##",
          "##....##",
          "##....##",
          "##....##"],
    "N": ["###...##",
          "###...##",
          "####..##",
          "####..##",
          "##.##.##",
          "##.##.##",
          "##..####",
          "##..####",
          "##...###",
          "##...###",
          "##....##",
          "##....##",
          "##....##",
          "##....##"],
    "e": [".......",
          ".......",
          ".......",
          ".......",
          ".#####.",
          "#######",
          "##...##",
          "##...##",
          "#######",
          "#######",
          "##.....",
          "##...##",
          "#######",
          ".#####."],
    "t": [".....",
          ".##..",
          ".##..",
          ".##..",
          "#####",
          "#####",
          ".##..",
          ".##..",
          ".##..",
          ".##..",
          ".##..",
          ".##.#",
          ".####",
          "..###"],
    "D": ["######..",
          "#######.",
          "##...###",
          "##....##",
          "##....##",
          "##....##",
          "##....##",
          "##....##",
          "##....##",
          "##....##",
          "##....##",
          "##...###",
          "#######.",
          "######.."],
    "R": ["######..",
          "#######.",
          "##...###",
          "##....##",
          "##....##",
          "##...###",
          "#######.",
          "######..",
          "##.###..",
          "##..###.",
          "##...###",
          "##....##",
          "##....##",
          "##....##"],
    "W": ["##......##",
          "##......##",
          "##......##",
          "##......##",
          "##......##",
          "##......##",
          "##..##..##",
          "##..##..##",
          "##..##..##",
          "##..##..##",
          "##.####.##",
          "##########",
          "####..####",
          "###....###"],
}
GAP = 2


def ramp14(glint, bright, dark):
    """Top edge glint, bright upper body, darker lower body."""
    return [glint] + [bright] * 7 + [dark] * 6

LETTER_RAMPS = [ramp14(WHITE, LCYAN, CYAN)] * 2 + [ramp14(WHITE, LCYAN, CYAN)] * 2 + [
    ramp14(WHITE, LRED, RED), ramp14(WHITE, YELLOW, BROWN),
    ramp14(WHITE, LGREEN, GREEN), ramp14(WHITE, LMAGENTA, MAGENTA)]
DRIP_COLOR = [None] * 4 + [RED, BROWN, GREEN, MAGENTA]
DRIP_TIP = [None] * 4 + [LRED, YELLOW, LGREEN, LMAGENTA]


def build_wordmark():
    h = 14
    ranges, x = [], 0
    for ch in WORD:
        wl = len(LETTERS[ch][0])
        ranges.append((x, x + wl))
        x += wl + GAP
    w = x - GAP
    if w > 76:
        raise SystemExit("wordmark too wide: %d" % w)
    drip_rows = 6
    ph = h + drip_rows + 1
    pix = [[None] * (w + 1) for _ in range(ph)]

    for li, ch in enumerate(WORD):
        x0 = ranges[li][0]
        rows = LETTERS[ch]
        for y, line in enumerate(rows):
            for dx, c in enumerate(line):
                if c != "#":
                    continue
                ramp = LETTER_RAMPS[li]
                above = y > 0 and rows[y - 1][dx] == "#"
                # glint only on pixels lit from above (nothing over them)
                color = ramp[0] if not above else ramp[max(1, y)]
                pix[y][x0 + dx] = color

    # Paint drips off the bottom of D, R, A, W -- different lengths so
    # they read as poured, not stamped. (letter, column in letter, length)
    drips = [(4, 1, 5), (4, 5, 2), (5, 0, 2), (5, 7, 6), (6, 1, 3), (6, 6, 5),
             (7, 0, 2), (7, 2, 4), (7, 8, 3)]
    for li, col, length in drips:
        x = ranges[li][0] + col
        if pix[h - 1][x] is None:
            raise SystemExit("drip not under ink: letter %d col %d" % (li, col))
        for k in range(length):
            pix[h + k][x] = DRIP_TIP[li] if k == length - 1 else DRIP_COLOR[li]

    # drop shadow, down-right, only where nothing is painted
    shadow = [[False] * (w + 1) for _ in range(ph)]
    for y in range(ph - 1):
        for x in range(w):
            if pix[y][x] is not None and pix[y + 1][x + 1] is None:
                shadow[y + 1][x + 1] = True
    for y in range(ph):
        for x in range(w + 1):
            if shadow[y][x]:
                pix[y][x] = DGRAY
    return pix


# ---------------------------------------------------------------- screen

class Screen:
    def __init__(self):
        self.cells = [[(" ", LGRAY, BLACK) for _ in range(W)] for _ in range(H)]

    def put(self, x, y, ch, fg, bg=BLACK):
        if 0 <= x < W - 1 and 0 <= y < H:  # column 80 never painted
            self.cells[y][x] = (ch, fg, bg)

    def text(self, x, y, s, fg, bg=BLACK):
        for i, ch in enumerate(s):
            self.put(x + i, y, ch, fg, bg)

    def center(self, y, s, fg, bg=BLACK):
        self.text((W - 1 - len(s)) // 2, y, s, fg, bg)

    def pixels(self, x0, y0, pix):
        """Two pixel rows per cell via the half-block glyphs."""
        for cy in range((len(pix) + 1) // 2):
            for x in range(len(pix[0])):
                top = pix[2 * cy][x]
                bot = pix[2 * cy + 1][x] if 2 * cy + 1 < len(pix) else None
                if top is None and bot is None:
                    continue
                if top == bot:
                    self.put(x0 + x, y0 + cy, "█", top)
                elif bot is None:
                    self.put(x0 + x, y0 + cy, "▀", top)
                elif top is None:
                    self.put(x0 + x, y0 + cy, "▄", bot)
                elif bot < 8:
                    self.put(x0 + x, y0 + cy, "▀", top, bot)
                elif top < 8:
                    self.put(x0 + x, y0 + cy, "▄", bot, top)
                else:  # two bright colors: needs iCE (bright bg)
                    self.put(x0 + x, y0 + cy, "▀", top, bot)


def compose():
    s = Screen()
    rule = "─" * 6 + "═" * 10 + "─" * 62
    s.text(0, 0, rule[:W - 1], BLUE)
    s.text(6, 0, "═" * 10, LBLUE)
    s.center(1, "A-Net Online presents", CYAN)

    pix = build_wordmark()
    s.pixels((W - 1 - len(pix[0])) // 2, 3, pix)
    logo_rows = (len(pix) + 1) // 2

    y = 3 + logo_rows + 1
    s.center(y, "The ANSI + ASCII Art Studio", WHITE)
    s.center(y + 1, "TheDraw \xb7 ACiDDraw \xb7 Moebius \xb7 PabloDraw ─ all the greats in one", LGRAY)

    # paint swatches -- the 15 visible colors as little pots of paint
    order = [RED, LRED, BROWN, YELLOW, GREEN, LGREEN, CYAN, LCYAN,
             BLUE, LBLUE, MAGENTA, LMAGENTA, DGRAY, LGRAY, WHITE]
    sw = len(order) * 4 - 1
    x0 = (W - 1 - sw) // 2
    for i, c in enumerate(order):
        s.text(x0 + i * 4, y + 3, "▄▄▄", c)
        s.text(x0 + i * 4, y + 4, "▀▀▀", c)

    s.center(y + 6, "v" + version() + "  │  By StingRay of A-Net Online  │  https://a-net.online", DGRAY)
    s.text(0, H - 1, rule[:W - 1], BLUE)
    s.text(W - 1 - 16, H - 1, "═" * 10, LBLUE)
    prompt = " press any key to start drawing "
    px = (W - 1 - len(prompt) - 2) // 2
    s.text(px, H - 1, "╡", LBLUE)
    s.text(px + 1, H - 1, prompt, WHITE)
    s.text(px + 1 + len(prompt), H - 1, "╞", LBLUE)
    return s


def version():
    for line in open(os.path.join(ROOT, "include", "anetdraw.h")):
        if line.startswith("#define AD_VERSION"):
            return line.split('"')[1]
    return "?"


# ---------------------------------------------------------------- output

PC_TO_ANSI = [0, 4, 2, 6, 1, 5, 3, 7]


def sgr(fg, bg):
    """The splash runs in BLINK mode on purpose (Jerry: "it looked great
    doing that"): a cell whose background is a bright color -- the white
    glints on the letters' top edges and the shadow under them -- gets
    the real blink attribute over the normal-intensity background, so
    its foreground half sparkles. Same look on every terminal and after
    every previous door; the editor switches to iCE (bright backgrounds,
    no blinking) once the splash is gone."""
    blink = bg >= 8
    return "\x1b[0;%s%s%d;%dm" % ("1;" if fg >= 8 else "", "5;" if blink else "",
                                   30 + PC_TO_ANSI[fg & 7], 40 + PC_TO_ANSI[bg & 7])


# ?33l: blink attribute blinks (not iCE); ?35l: blinking enabled, in case
# a previous door turned it off (SyncTERM cterm.adoc modes 33 and 35)
SPLASH_PREFIX = "\x1b[?33l\x1b[?35l\x1b[0m\x1b[2J"
SPLASH_SUFFIX = "\x1b[0m"


def to_rows(s):
    """[(row, col, bytes)] -- each visible row's content without its
    cursor positioning, so the door can center the art on any screen."""
    rows = []
    for y, row in enumerate(s.cells):
        # trim trailing plain blanks
        last = max((x for x, (ch, fg, bg) in enumerate(row) if ch != " " or bg != BLACK), default=-1)
        if last < 0:
            continue
        first = min(x for x, (ch, fg, bg) in enumerate(row) if ch != " " or bg != BLACK)
        out, cur = "", None
        for x in range(first, last + 1):
            ch, fg, bg = row[x]
            if ch == " ":
                fg = cur[0] if cur else fg  # a space only needs the bg to match
            if cur != (fg, bg):
                out += sgr(fg, bg)
                cur = (fg, bg)
            out += ch
        rows.append((y, first, out.encode("cp437")))
    return rows


def to_bytes(rows):
    """The whole splash at the top-left of an 80x24 screen -- exactly
    what the door sends at that size (tests compare against it)."""
    out = SPLASH_PREFIX.encode()
    for y, x, data in rows:
        out += ("\x1b[%d;%dH" % (y + 1, x + 1)).encode() + data
    return out + SPLASH_SUFFIX.encode()


def c_literal(data):
    lines, cur = [], ""
    prev_hex = False
    for b in data:
        if b < 0x20 or b >= 0x7F or b in (0x22, 0x5C, 0x3F):
            piece, is_hex = "\\x%02X" % b, True
        else:
            ch = chr(b)
            # a hex escape swallows following hex digits -- break the string
            piece = ('" "' + ch) if prev_hex and ch in "0123456789abcdefABCDEF" else ch
            is_hex = False
        cur += piece
        prev_hex = is_hex
        if len(cur) > 70:
            lines.append('    "%s"' % cur)
            cur, prev_hex = "", False
    if cur:
        lines.append('    "%s"' % cur)
    return "\n".join(lines)


def write_c(rows):
    path = os.path.join(ROOT, "src", "splash_art.c")
    with open(path, "w") as f:
        f.write("/* GENERATED by util/make_splash.py -- edit that, not this. */\n")
        f.write('#include "../include/splash.h"\n\n')
        f.write("const AdSplashRow AD_SPLASH_ROWS[] = {\n")
        for y, x, data in rows:
            f.write("  { %d, %d,\n%s },\n" % (y, x, c_literal(data)))
        f.write("};\n")
        f.write("const int AD_SPLASH_NROWS = %d;\n" % len(rows))
    return path


def render_png(s, path):
    """Cell-accurate preview: 8x16 cells, VGA palette."""
    pal = [(0, 0, 0), (0, 0, 170), (0, 170, 0), (0, 170, 170), (170, 0, 0), (170, 0, 170),
           (170, 85, 0), (170, 170, 170), (85, 85, 85), (85, 85, 255), (85, 255, 85),
           (85, 255, 255), (255, 85, 85), (255, 85, 255), (255, 255, 85), (255, 255, 255)]
    cw, chh = 8, 16
    img = Image.new("RGB", (W * cw, H * chh))
    d = ImageDraw.Draw(img)
    font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 13)
    for y, row in enumerate(s.cells):
        for x, (ch, fg, bg) in enumerate(row):
            x0, y0 = x * cw, y * chh
            d.rectangle([x0, y0, x0 + cw - 1, y0 + chh - 1], fill=pal[bg])
            f = pal[fg]
            if ch == "█":
                d.rectangle([x0, y0, x0 + cw - 1, y0 + chh - 1], fill=f)
            elif ch == "▀":
                d.rectangle([x0, y0, x0 + cw - 1, y0 + chh // 2 - 1], fill=f)
            elif ch == "▄":
                d.rectangle([x0, y0 + chh // 2, x0 + cw - 1, y0 + chh - 1], fill=f)
            elif ch == "─":
                d.line([x0, y0 + 8, x0 + cw - 1, y0 + 8], fill=f)
            elif ch == "═":
                d.line([x0, y0 + 6, x0 + cw - 1, y0 + 6], fill=f)
                d.line([x0, y0 + 9, x0 + cw - 1, y0 + 9], fill=f)
            elif ch in ("╡", "╞"):  # double horizontal meeting a single vertical
                xa, xb = (x0, x0 + 4) if ch == "╡" else (x0 + 4, x0 + cw - 1)
                d.line([xa, y0 + 6, xb, y0 + 6], fill=f)
                d.line([xa, y0 + 9, xb, y0 + 9], fill=f)
                d.line([x0 + 4, y0, x0 + 4, y0 + chh - 1], fill=f)
            elif ch == "│":
                d.line([x0 + 4, y0, x0 + 4, y0 + chh - 1], fill=f)
            elif ch != " ":
                d.text((x0, y0 + 1), ch, font=font, fill=f)
    img = img.resize((img.width * 2, img.height * 2), Image.NEAREST)
    img.save(path)


def main():
    s = compose()
    rows = to_rows(s)
    data = to_bytes(rows)
    assert b"\n" not in data and b"\r" not in data and b"\x00" not in data
    with open(os.path.join(ROOT, "util", "splash.ans"), "wb") as f:
        f.write(data)
    print("wrote", write_c(rows), len(data), "bytes,", len(rows), "rows")
    if "--png" in sys.argv:
        render_png(s, os.path.join(ROOT, "util", "splash.png"))
        print("wrote util/splash.png")


if __name__ == "__main__":
    main()
