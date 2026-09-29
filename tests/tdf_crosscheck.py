#!/usr/bin/env python3
"""Cross-checks src/tdf.c against an independent Python reading of the
TDF format (tdfiglet's glyph decoding: width, height, rows ended by 0x0D,
(char, attr) pairs, 0x00 at the end). For every color font that has all
the letters of the sample, both must produce identical cells.

Usage: tests/tdf_crosscheck.py path/to/test_tdf fontdir
"""
import subprocess
import sys

TEXT = "ANETDRAW"


def ref_render(path, offset):
    d = open(path, "rb").read()
    p = offset
    spacing = d[p + 22]
    offs = [int.from_bytes(d[p + 25 + i * 2:p + 27 + i * 2], "little") for i in range(94)]
    data = d[p + 213:]
    glyphs = []
    for ch in TEXT:
        o = offs[ord(ch) - 33]
        if o == 0xFFFF:
            return None
        w, h = data[o], data[o + 1]
        if not (1 <= w <= 30 and 1 <= h <= 12):
            return None  # out-of-spec glyph: the engine treats it as missing
        cells = {}
        q, row, col = o + 2, 0, 0
        while data[q]:
            c = data[q]; q += 1
            if c == 13:
                row, col = row + 1, 0
                continue
            a = data[q]; q += 1
            if row < h and col < w:
                cells[(row, col)] = (c if c >= 0x20 else 0x20, a)
            col += 1
        glyphs.append((w, h, cells))
    width = sum(g[0] for g in glyphs) + spacing * (len(glyphs) - 1)
    height = max(g[1] for g in glyphs)
    grid = [[(0, 0)] * width for _ in range(height)]
    x = 0
    for i, (w, h, cells) in enumerate(glyphs):
        if i:
            x += spacing
        for (r, c), v in cells.items():
            grid[r][x + c] = v
        x += w
    return [" ".join("%02x%02x" % v for v in row) for row in grid]


def main():
    exe, fontdir = sys.argv[1], sys.argv[2]
    i = checked = fails = 0
    while True:
        r = subprocess.run([exe, "dump", fontdir, str(i), TEXT], capture_output=True, text=True)
        if r.returncode != 0 or not r.stdout:
            break
        lines = r.stdout.splitlines()
        path, name, offset = lines[0].split("\t")
        head = open(path, "rb").read()[int(offset) + 21]
        if head == 2:
            want = ref_render(path, int(offset))
            if want is not None:
                checked += 1
                got = [l.strip() for l in lines[1:]]
                if got != want:
                    fails += 1
                    if fails <= 3:
                        print("MISMATCH font %d %s" % (i, lines[0]))
        i += 1
    print("%d fonts, %d color fonts cross-checked, %d mismatches" % (i, checked, fails))
    sys.exit(1 if fails or not checked else 0)


if __name__ == "__main__":
    main()
