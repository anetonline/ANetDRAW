#!/usr/bin/env python3
"""Builds the ANetDRAW TheDraw font pack (a separate download from the
door) from a collection of .TDF files -- a zip or a folder.

    python3 util/make_fontpack.py ../tdf-fonts.zip  [out_dir]

  - every font in every .TDF file is parsed (a file may hold several);
    files that aren't TheDraw font files are skipped
  - fonts whose data carries an explicit rights reservation ("All Rights
    Reserved") are left out -- see FONTS.md
  - duplicates (identical glyphs under the same or another name) are
    kept once; fonts from single-font files win over the big multi-font
    compilation sets (SETS/TDFONTS*.TDF), so they keep their own names
  - output: out_dir/fonts/ (default build-fonts/fonts/) with the kept
    fonts, the original file names where possible, EXTRAnn.TDF for fonts
    found only in compilations, plus FONTS.md (credits) and FONTLIST.txt

Unzip the result into the door's folder: ANetDRAW loads fonts/*.tdf.
"""
import hashlib
import io
import os
import re
import sys
import zipfile

MAGIC = b"\x13TheDraw FONTS file\x1a"
MARK = b"\x55\xaa\x00\xff"
TYPES = {0: "outline", 1: "block", 2: "color"}
RESERVED = re.compile(rb"all[\s-]*rights[\s-]*reserved", re.I)
CREDITS = re.compile(rb"\bby [A-Z]")
PER_EXTRA_FILE = 30


def read_sources(src):
    """Yields (relative path, bytes) for every .TDF in a zip or folder."""
    if os.path.isdir(src):
        for root, _, files in os.walk(src):
            for f in sorted(files):
                if f.lower().endswith(".tdf"):
                    p = os.path.join(root, f)
                    yield os.path.relpath(p, src), open(p, "rb").read()
    else:
        with zipfile.ZipFile(src) as z:
            for n in sorted(z.namelist()):
                if n.lower().endswith(".tdf"):
                    yield n, z.read(n)


def parse(data):
    """Fonts in one .TDF file: list of dicts with the raw font block."""
    fonts = []
    if not data.startswith(MAGIC):
        return fonts
    p = len(MAGIC)
    while p + 25 + 188 <= len(data) and data[p:p + 4] == MARK:
        n = min(data[p + 4], 12)
        name = data[p + 5:p + 5 + n].decode("cp437", "replace").strip("\0 ")
        ftype, spacing = data[p + 21], data[p + 22]
        size = int.from_bytes(data[p + 23:p + 25], "little")
        end = p + 25 + 188 + size
        if end > len(data):
            break
        block = data[p:end]
        # identity = everything but the name (renamed copies are still copies)
        ident = hashlib.sha1(block[21:]).hexdigest()
        fonts.append({"name": name or "(unnamed)", "type": ftype, "spacing": spacing,
                      "block": block, "ident": ident})
        p = end
    return fonts


def main():
    if len(sys.argv) < 2:
        raise SystemExit(__doc__)
    src = sys.argv[1]
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    out = sys.argv[2] if len(sys.argv) > 2 else os.path.join(root, "build-fonts")
    fdir = os.path.join(out, "fonts")
    os.makedirs(fdir, exist_ok=True)
    for f in os.listdir(fdir):
        if f.upper().endswith(".TDF") or f in ("FONTS.md", "FONTLIST.txt"):
            os.remove(os.path.join(fdir, f))

    sources = list(read_sources(src))
    # single files first, compilation sets last
    sources.sort(key=lambda s: ("/SETS/" in "/" + s[0].upper(), s[0].upper()))
    seen = {}
    excluded, credits, extras, kept_files = [], {}, [], []
    total = 0
    for rel, data in sources:
        fonts = parse(data)
        total += len(fonts)
        keep = []
        for f in fonts:
            if RESERVED.search(f["block"]):
                excluded.append((rel, f["name"], RESERVED.search(f["block"]).group().decode()))
                continue
            if f["ident"] in seen:
                continue
            seen[f["ident"]] = rel
            for m in CREDITS.finditer(f["block"]):
                # the credit text is broken up by non-printing bytes:
                # one joins words, a run of them ends the credit
                raw = f["block"][m.start():m.start() + 80]
                text = "".join(chr(b) if 0x20 <= b < 0x7f else "|" for b in raw).split("||")[0]
                text = re.sub(r"[|\s]+", " ", text).strip()
                credits.setdefault(text, set()).add(f["name"])
            keep.append(f)
        if not keep:
            continue
        if "/SETS/" in "/" + rel.upper():
            extras.extend(keep)
            continue
        name = os.path.basename(rel).upper()
        kept_files.append((name, keep))

    for i in range(0, len(extras), PER_EXTRA_FILE):
        kept_files.append(("EXTRA%02d.TDF" % (i // PER_EXTRA_FILE + 1), extras[i:i + PER_EXTRA_FILE]))

    listing = []
    nfonts = 0
    for name, fonts in kept_files:
        with open(os.path.join(fdir, name), "wb") as f:
            f.write(MAGIC + b"".join(x["block"] for x in fonts))
        for x in fonts:
            listing.append("%-14s %-13s %s" % (name, x["name"], TYPES.get(x["type"], "?")))
        nfonts += len(fonts)

    with open(os.path.join(fdir, "FONTLIST.txt"), "w") as f:
        f.write("ANetDRAW font pack -- %d fonts in %d files\n\n" % (nfonts, len(kept_files)))
        f.write("%-14s %-13s %s\n" % ("File", "Font", "Type"))
        f.write("\n".join(listing) + "\n")

    credit_lines = "\n".join("- %s (%s)" % (c, ", ".join(sorted(n))) for c, n in sorted(credits.items())) \
        or "- (no author credits were found inside the font files)"
    excl_lines = "\n".join("- `%s` font \"%s\": the font data says \"%s\"" % e for e in excluded) or "- none"
    with open(os.path.join(fdir, "FONTS.md"), "w") as f:
        f.write(FONTS_MD.format(nfonts=nfonts, nfiles=len(kept_files), total=total,
                                credits=credit_lines, excluded=excl_lines))
    print("%d fonts read, %d kept in %d files (%d duplicates, %d excluded) -> %s"
          % (total, nfonts, len(kept_files), total - nfonts - len(excluded), len(excluded), fdir))
    for e in excluded:
        print("  excluded: %s \"%s\" (%s)" % e)


FONTS_MD = """# ANetDRAW font pack: TheDraw fonts

This pack holds {nfonts} TheDraw (`.TDF`) banner fonts in {nfiles} files.
Unzip it into the ANetDRAW folder so the fonts land in `fonts/`, or point
the door at them with `--fonts DIR`. In the editor, **^T** then **T**
types text in any of them. `FONTLIST.txt` lists every font by file and
name.

## Where these fonts come from

TheDraw fonts were made by BBS-era ANSI artists in the late 1980s and
1990s and passed around freely on bulletin boards for decades. This pack
was built from the collection compiled and preserved by
**Roy/SAC** (Roy of Superior Art Creations,
<https://www.roysac.com/thedrawfonts-tdf.html>). The same fonts ship
with other BBS software, such as Synchronet (`ctrl/tdfonts`) and
tdfiglet.

The `.TDF` font format and the TheDraw editor are by **Ian E. Davis**
(TheSoft Programming Services).

## Credits found in the fonts

A TheDraw font file has no author field, so most fonts don't name their
creator. These credits appear inside the font data itself:

{credits}

If you drew one of these fonts, we'd be glad to add your name here.

## Status and removal

These fonts come with **no license**. As far as anyone knows, their
authors released them as freeware for BBS use, and they have circulated
openly ever since. Only one font in the source collection carries an
explicit rights reservation, and it has been **left out** of this pack:

{excluded}

**If you are the author of a font in this pack and want it credited
differently or removed, contact the ANetDRAW author and it will be done
in the next release.**

The pack was built by `util/make_fontpack.py`. It read {total} fonts,
kept one copy of each distinct font, and dropped the excluded ones.
"""


if __name__ == "__main__":
    main()
