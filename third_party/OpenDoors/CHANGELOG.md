# Changelog

OpenDoors release versions use dotted `major.minor.patch` notation. See the
[versioning guide](docs/guides/versioning.md) for its relationship to the
traditional OpenDoors notation.

## Unreleased

- Added an Open Watcom 32-bit flat-model DOS platform with separate register-
  and stack-convention libraries.
- Added DOS/32A- and DOS/4GW-tested runtime, ABI, personality, and FOSSIL
  coverage, including DPMI-buffered block calls with byte-call fallback.
- Added a DOS32 release SDK containing both libraries and native DOS/32A LX
  register-convention examples with the extender embedded.

## 6.3.0 - 2026-08-08

Traditional OpenDoors version: 6.30.

- Added maintained CMake builds for shared and static libraries on current
  Windows, macOS, and Unix-like systems.
- Added installable CMake package targets and a two-header public SDK.
- Added dynamically sized virtual screens and size-aware screen save and
  restore functions without changing existing screen interfaces.
- Added modern Markdown reference documentation and GitHub Pages publishing.
- Restored and tested 16-bit DOS builds with Open Watcom and Turbo C 2.01.
- Restored the documented DOS personality SDK and its standard personalities.
- Added warning-clean compatibility paths for modern, Watcom, Turbo C,
  Borland, and Microsoft compiler families.
