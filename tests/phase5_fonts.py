#!/usr/bin/env python3
"""Phase 5 TheDraw font test over the real DOOR32.SYS socket path.

Synthetic fonts with known glyphs check the exact cells and colors that
land on the canvas; the real font pack (if given) gets a smoke test.

Usage: tests/phase5_fonts.py path/to/anetdraw [path/to/fontpack/fonts]
"""
import os
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from phase1_session import check, FAILS  # noqa: E402
from phase2_tools import Door, ctrl, ESC, ENTER, BKSP, DOWN  # noqa: E402

MAGIC = b"\x13TheDraw FONTS file\x1a"


def font(name, ftype, spacing, glyphs):
    """glyphs: {char: (w, h, rows)}; rows are lists of cells, each a
    byte (block/outline) or (byte, attr) (color)."""
    data = bytearray()
    offs = [0xFFFF] * 94
    for ch, (w, h, rows) in glyphs.items():
        offs[ord(ch) - 33] = len(data)
        data += bytes([w, h])
        for r, row in enumerate(rows):
            for cell in row:
                data += bytes(cell) if isinstance(cell, tuple) else bytes([cell])
            if r < len(rows) - 1:
                data += b"\r"
        data += b"\0"
    head = b"\x55\xaa\x00\xff" + bytes([len(name)]) + name.encode().ljust(12, b"\0") + bytes(4)
    head += bytes([ftype, spacing]) + len(data).to_bytes(2, "little")
    head += b"".join(o.to_bytes(2, "little") for o in offs)
    return head + bytes(data)


def screen_has(d, text):
    return any(text in d.row(y) for y in range(d.screen.lines))


def main():
    binary = os.path.abspath(sys.argv[1])
    pack = os.path.abspath(sys.argv[2]) if len(sys.argv) > 2 else None
    root = tempfile.mkdtemp(prefix="anetdraw_fonts_")
    fonts = os.path.join(root, "fonts")
    os.makedirs(fonts)
    # a color font: 'A' is 3x2, 'B' 2x2 with a short second row (transparent end)
    color = font("Testcolor", 2, 1, {
        "A": (3, 2, [[(0xDB, 0x0C), (0xDF, 0x4E), (0xDB, 0x0C)], [(0xDB, 0x0C), (0x20, 0x10), (0xDB, 0x0C)]]),
        "B": (2, 2, [[(0xDB, 0x0A), (0xDC, 0x0A)], [(0xDB, 0x0A)]]),
    })
    block = font("Testblock", 1, 0, {"A": (2, 1, [[0xB1, 0xB2]]), "b": (1, 1, [[0xDB]])})
    # two fonts in one file, as TheDraw stores them
    open(os.path.join(fonts, "TEST.TDF"), "wb").write(MAGIC + color + block)
    open(os.path.join(fonts, "junk.tdf"), "wb").write(b"not a font")

    # ---------------- no fonts installed
    n = Door(binary, extra_args=("--data", os.path.join(root, "data"), "--fonts", os.path.join(root, "none")))
    n.read(n.startup_secs)
    n.send(b" ", 1.0)
    n.send(ctrl("T"), 0.8)
    n.send(b"t", 1.0)
    check("No TheDraw fonts found" in n.status(), "no fonts: a clear message: %r" % n.status()[16:70])
    n.sock.close()
    n.proc.wait(5)

    # ---------------- the synthetic fonts
    d = Door(binary, extra_args=("--data", os.path.join(root, "data"), "--fonts", fonts))
    d.read(d.startup_secs)
    d.send(b" ", 1.0)
    d.send(ctrl("T"), 0.8)
    check(screen_has(d, "T Text"), "^T menu offers T Text")
    d.send(b"t", 1.2)
    check(screen_has(d, "TheDraw fonts") and screen_has(d, "2 fonts"), "font browser lists both fonts in the file")
    check(screen_has(d, "Testblock") and screen_has(d, "Testcolor"), "both font names show (sorted)")
    d.send(BKSP * 20, 0.4)
    d.send(b"AB", 0.8)
    # Testblock is first (sorted): 'A' then 'b' via... 'B' isn't in it, 'b' is -- upper->lower isn't a fallback
    d.send(b"\t", 0.3)
    d.send(b"color", 0.8)
    check(screen_has(d, "1 font") and not screen_has(d, "Testblock"), "Find narrows the list to Testcolor")
    prev = [y for y in range(d.screen.lines) if "Preview" in d.row(y)]
    check(prev, "a preview area is shown")
    if prev:
        y = prev[0] + 1
        row = d.row(y)
        x = row.index("█")  # first full block of the preview
        check(row[x:x + 6] == "█▀█ █▄", "preview draws 'AB' in the font: %r" % row[x:x + 6])
    d.send(ENTER, 1.0)
    check("Place it" in d.status(), "Enter goes to placing it: %r" % d.status()[16:70])
    d.send(ENTER, 0.8)      # stamp at the cursor (0,0)
    d.send(ESC, 0.8)        # done placing
    c = [d.cell(x, 0) for x in range(6)]
    check("".join(x.data for x in c) == "█▀█ █▄", "stamped on the canvas: %r" % "".join(x.data for x in c))
    # 0x4E = bright yellow (pyte: brown + bold) on red
    check(c[0].fg == "red" and c[0].bold and c[1].fg == "brown" and c[1].bold and c[1].bg == "red",
          "the font's own colors are kept: %s/%s %s/%s" % (c[0].fg, c[0].bold, c[1].fg, c[1].bg))
    check(d.cell(4, 1).data == "█" and d.cell(4, 1).fg == "green", "second row of 'B' too")
    # the spacing column (x=3 row 0) and B's missing second cell stay untouched
    check(d.cell(3, 0).data == " " and d.cell(5, 1).data == " ", "spacing and transparent cells stay blank")

    # block font uses the current colors; lower case falls back to upper case
    d.send(ctrl("T"), 0.8)
    d.send(b"t", 1.2)
    d.send(b"\t", 0.3)
    d.send(BKSP * 10, 0.3)
    d.send(b"block", 0.6)
    d.send(b"\t", 0.3)
    d.send(BKSP * 20, 0.3)
    d.send(b"ab", 0.6)
    d.send(ENTER, 1.0)
    d.send(b"\x1b[B" * 4, 0.6)   # move the placement down 4 rows
    d.send(ENTER, 0.8)
    d.send(ESC, 0.8)
    row = "".join(d.cell(x, 4).data for x in range(3))
    check(row == "▒▓█", "block font 'ab': 'a' uses the 'A' glyph, 'b' its own: %r" % row)
    d.send(ctrl("Z"), 0.8)
    check(d.cell(0, 4).data == " ", "one ^Z undoes the whole stamp")
    d.send(ESC, 1.0)
    d.send(b"q", 1.0)
    d.send(b"n" if "Save your drawing" in d.status() else b"y", 1.5)
    d.proc.wait(5)

    # ---------------- the real pack
    if pack:
        p = Door(binary, extra_args=("--data", os.path.join(root, "data"), "--fonts", pack))
        p.read(p.startup_secs)
        p.send(b" ", 1.0)
        p.send(ctrl("T"), 0.8)
        p.send(b"t", 4.0)
        check(screen_has(p, "3716 fonts"), "the real pack lists all 3716 fonts")
        p.send(b"\t", 0.3)
        p.send(b"1911", 1.0)
        prev = [y for y in range(p.screen.lines) if "Preview" in p.row(y)]
        filled = sum(1 for y in range(prev[0] + 1, prev[0] + 8) for x in range(80)
                     if p.cell(x, y).data not in (" ", "")) if prev else 0
        check(filled > 40, "a real color font renders a preview (%d cells)" % filled)
        p.send(ESC, 1.0)
        p.sock.close()
        p.proc.wait(5)

    print("\n%d failure(s)" % len(FAILS))
    sys.exit(1 if FAILS else 0)


if __name__ == "__main__":
    main()
