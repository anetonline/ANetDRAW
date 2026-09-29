#!/usr/bin/env bash
set -euo pipefail

# Packages one archive per platform from the shippable build-*/
# directories. Run from the repo root after building whichever
# targets you want packaged -- a target whose build directory doesn't
# exist is skipped, not an error, so this also works for a partial
# release. Adapted directly from ANetCRAFT_opendoors/util/make_release.sh (itself from rdq3_opendoors).

VER="${1:-anetdraw_release}"
RELROOT="release/$VER"
rm -rf "$RELROOT"
mkdir -p "$RELROOT"

package_platform() {
    local platform="$1" builddir="$2" gamebin="$3" archive_ext="$4"
    if [ ! -f "$builddir/$gamebin" ]; then
        echo "Skipping $platform (no $builddir/$gamebin)"
        return
    fi
    local pkgname="${VER}_${platform}"
    local pkgdir="$RELROOT/$pkgname"
    mkdir -p "$pkgdir/docs"

    cp -f "$builddir/$gamebin" "$pkgdir/"
    cp -f README.md "$pkgdir/" 2>/dev/null || true
    cp -f docs/*.md docs/*.txt "$pkgdir/docs/" 2>/dev/null || true
    # the gallery starts with the sample pieces (util/make_samples.py)
    mkdir -p "$pkgdir/anetdraw_data/gallery"
    cp -f samples/*.ans "$pkgdir/anetdraw_data/gallery/" 2>/dev/null || true
    # Per-platform FILE_ID.DIZ, same reasoning as RDQ3's own script:
    # some BBS auto-file-tagging scanners only look at the archive's
    # true root, not inside the packaged subfolder.
    local diz="FILE_ID.DIZ.$platform"
    [ -f "$diz" ] || diz="FILE_ID.DIZ"
    cp -f "$diz" "$pkgdir/FILE_ID.DIZ" 2>/dev/null || true
    cp -f "$diz" "$RELROOT/FILE_ID.DIZ" 2>/dev/null || true

    ( cd "$RELROOT" && case "$archive_ext" in
        tar.gz) tar -czf "$pkgname.tar.gz" "$pkgname" "FILE_ID.DIZ" ;;
        zip) zip -qr "$pkgname.zip" "$pkgname" "FILE_ID.DIZ" ;;
      esac )
    echo "Packaged $RELROOT/$pkgname.$archive_ext"
}

package_platform "linux-x64-static" "build-static" "anetdraw"     "tar.gz"
package_platform "linux-arm64-pi"   "build-pi"     "anetdraw"     "tar.gz"
package_platform "win64"            "build-win"    "anetdraw.exe" "zip"
package_platform "win32"            "build-win32"  "anetdraw.exe" "zip"

# The TheDraw font pack is its own download (see README / fonts/FONTS.md).
# Build it first with: python3 util/make_fontpack.py <TDF collection zip>
if [ -f build-fonts/fonts/FONTS.md ]; then
    fontzip="$PWD/$RELROOT/ANetDRAW-fonts.zip"
    rm -f "$fontzip"
    ( cd build-fonts && zip -qr "$fontzip" fonts )
    echo "Packaged $RELROOT/ANetDRAW-fonts.zip"
else
    echo "Skipping the font pack (run util/make_fontpack.py first)"
fi

rm -f "$RELROOT/FILE_ID.DIZ"
echo "Release archives prepared under $RELROOT/"
