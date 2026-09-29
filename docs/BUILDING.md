# Building ANetDRAW from source

ANetDRAW is C99 built with CMake. OpenDoors and stb are vendored in
`third_party/`, so no other libraries are needed.

## Native (development)

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/anetdraw -L
```

## Release targets

| Target | Configure |
|---|---|
| Linux x86-64, static | `cmake -S . -B build-static -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXE_LINKER_FLAGS=-static` |
| Raspberry Pi / ARM64, static | `cmake -S . -B build-pi -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXE_LINKER_FLAGS=-static -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-toolchain.cmake` |
| Windows 64-bit | `cmake -S . -B build-win -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-toolchain.cmake` |
| Windows 32-bit | `cmake -S . -B build-win32 -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-i686-toolchain.cmake` |

Then `cmake --build <dir> -j` for each. The cross builds need
`aarch64-linux-gnu-gcc` and the `x86_64-w64-mingw32` /
`i686-w64-mingw32` MinGW compilers.

The Windows builds link `OpenDoors::StaticConsole` (a console program,
not a GUI one); linking plain `OpenDoors::Static` compiles fine but
OpenDoors refuses to run. Local mode opens ANetDRAW's own window
(`src/wingui.c`).

## Packaging

```
python3 util/make_fontpack.py path/to/tdf-collection.zip   # once: build-fonts/fonts
util/make_release.sh anetdraw_v0.2.0
```

This makes one archive per built target under `release/<name>/`, each
with its own `FILE_ID.DIZ`, plus `ANetDRAW-fonts.zip`. A target whose
build folder is missing is skipped. `util/make_samples.py` regenerates
the sample gallery pieces, and `util/make_font.py` the built-in VGA font.

## Tests

End-to-end tests drive the real program over a socket or a PTY with a
terminal emulator (`pip install pyte`):

```
python3 tests/phase1_session.py build/anetdraw
python3 tests/phase8_formats.py build/anetdraw
...
```

The ZMODEM tests need lrzsz (`lrz`, `lsz`):

```
python3 tests/phase6_zmodem.py build/anetdraw /path/to/lrz
python3 tests/phase8_upload.py build/anetdraw /path/to/lsz
cc -O1 -g -fsanitize=address,undefined -o test_zmodem tests/test_zmodem.c src/zmodem.c
./test_zmodem /path/to/lrz
```

The other C unit tests (`tests/test_*.c`) build the same way; each
file's header comment gives its command line.

## OpenDoors

`third_party/OpenDoors` is upstream OpenDoors at the commit in
`third_party/OPENDOORS_COMMIT`, with the local fixes listed (and marked
in the source) in `third_party/OPENDOORS_LOCAL_PATCHES.md`. Re-apply
them after updating OpenDoors.
