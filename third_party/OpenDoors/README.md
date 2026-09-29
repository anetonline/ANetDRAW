# OpenDoors

[![Build and smoke integration](https://github.com/RealDeuce/OpenDoors/actions/workflows/build.yml/badge.svg?branch=master)](https://github.com/RealDeuce/OpenDoors/actions/workflows/build.yml)
[![Unit Tests](https://github.com/RealDeuce/OpenDoors/actions/workflows/unit-tests.yml/badge.svg?branch=master)](https://github.com/RealDeuce/OpenDoors/actions/workflows/unit-tests.yml)
[![Regression tests](https://github.com/RealDeuce/OpenDoors/actions/workflows/regression.yml/badge.svg?branch=master)](https://github.com/RealDeuce/OpenDoors/actions/workflows/regression.yml)
[![Documentation](https://github.com/RealDeuce/OpenDoors/actions/workflows/docs.yml/badge.svg?branch=master)](https://github.com/RealDeuce/OpenDoors/actions/workflows/docs.yml)
[![Extended acceptance](https://github.com/RealDeuce/OpenDoors/actions/workflows/acceptance.yml/badge.svg?branch=master)](https://github.com/RealDeuce/OpenDoors/actions/workflows/acceptance.yml)

OpenDoors 6.30 is a C/C++ toolkit for writing online software such as BBS
doors. It transparently interfaces with most BBS systems, displays output on
both local and remote screens, creates ANSI/AVATAR/RIP control sequences, and
provides a sysop interface. The 6.2x changes by Rob Swindell added TCP socket
(Telnet) and Door32.sys support.

## Documentation

The current API and integration documentation is published as the
[OpenDoors Reference](https://realdeuce.github.io/OpenDoors/). Its Markdown
source is maintained under `docs/`. To preview it locally:

```sh
python -m pip install -r docs/requirements.txt
python tools/check_api_docs.py
mkdocs serve
```

The original CP437 OpenDoors 6.00 manual is retained under `historic/` for
historical reference.

Release and package versions use `6.3.0` for the traditional OpenDoors
version `6.30`. The [versioning guide](https://realdeuce.github.io/OpenDoors/guides/versioning/)
documents the compatibility guarantees and the `OD_VERSION` encoding.

## Source layout

The public headers are in `include/`, library implementation files and native
resources are in `src/`, and sample programs are in `examples/`. Example
runtime files live in `examples/data/` and are copied beside built examples.
Release-only metadata is under `release/`; superseded distribution artifacts
are retained under `historic/`.

## CMake builds

CMake 3.20 or newer can build the shared library, static library, examples,
and link smoke tests on Windows, Linux, and macOS:

```sh
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cmake --install build --prefix /path/to/prefix
```

Both library variants are built by default. Set `OPENDOORS_BUILD_SHARED` or
`OPENDOORS_BUILD_STATIC` to `OFF` to omit that variant; at least one must
remain enabled. An installed package can be consumed with:

```cmake
find_package(OpenDoors 6.3 CONFIG REQUIRED COMPONENTS Shared)
target_link_libraries(mydoor PRIVATE OpenDoors::Shared)
```

On Unix-like systems the libraries are `libODoors.so`/`libODoors.a` (or the
macOS `.dylib` equivalent). With MSVC, the shared build produces
`ODoors63.dll` and `ODoorW.lib`; the static build produces
`ODoors-static.lib`. `OpenDoors::Static` supplies
[`OD_WIN32_STATIC`](https://realdeuce.github.io/OpenDoors/guides/building/)
automatically; consumers linking the Windows static library without the CMake
target must define it themselves.
The `OpenDoors::Shared` and `OpenDoors::Static` CMake targets are available
both from `add_subdirectory()` and from the installed package when their
corresponding variants were built.

## Testing

The test automation has four distinct layers:

- Unit tests are narrow white-box cases selected from the source and function
  owners affected by a change.
- Smoke integration tests use only the public API and run on every push as
  part of the normal build matrix.
- Extended acceptance tests use only the public API and exercise the deeper
  socket, direct-UART, and FOSSIL scenarios nightly when there have been new
  pushes, on request, and for a release.
- Regression tests may use private component boundaries and mocks. They run
  on every push; reported bugs should gain a reproducer here when public-only
  acceptance cannot express one.

The Actions select the public `acceptance.*` and link-smoke cases separately
from the private implementation regressions. The acceptance inventory covers
every exported declaration and every `od_control` field, while its executables
use only the installed headers and library interface. See
[`tests/acceptance/README.md`](tests/acceptance/README.md) for standalone
installed-package and extended socket-driven test commands. Behavioral and
design problems discovered while characterizing the current implementation
should be reported through the project issue tracker.

Tests require a Python 3 interpreter. When `BUILD_TESTING` is not specified,
CMake disables it automatically if Python 3 is unavailable, so Python is not
required to build or install the libraries and examples. Explicitly setting
`BUILD_TESTING=ON` makes Python 3 a required dependency.

MSVC builds may also enable `OPENDOORS_BUILD_MSVC_STATIC_MT` to produce
`ODoors-static-mt.lib` and the `OpenDoors::StaticMT` target. This additional
library uses Microsoft's static C runtime; `OpenDoors::Static` continues to
use the runtime selected for the main build.

MinGW builds provide the same `OpenDoors::Shared` and `OpenDoors::Static`
targets, but their import and static libraries are not interchangeable with
MSVC libraries. Release SDKs identify their MINGW32, MINGW64, or UCRT64
runtime explicitly.

## MSVC builds

The PowerShell wrapper uses separate build trees for each architecture:

```powershell
.\build-msvc.ps1 -Architecture x64 -Configuration Release
.\build-msvc.ps1 -Architecture x86 -Configuration Debug
```

Use `-Clean` to recreate an architecture's build tree. The script requires
CMake and a compatible Visual Studio/MSVC installation. The included
`odoors.props` property sheet points Visual Studio projects at these default
output locations; set `OpenDoorsLibraryDir` before importing it to override
the library directory.

## Vote example

The `ex_vote` example is self-contained and builds with the other examples.
Maintained builds enable its multi-node sidecar locking. Its fixed-size,
little-endian data files can be shared by DOS, Windows, and Unix-like builds;
see the programming guide for the file and lock compatibility rules.

## Open Watcom DOS builds

The DOS builds have their own CMake project under `dos/`, separate from the
modern host builds. With Open Watcom 2.0 configured in the environment, select
the 16-bit large-model target with:

```sh
cmake -S dos -B build/dos -G "Watcom WMake" \
  -D CMAKE_SYSTEM_NAME=DOS \
  -D CMAKE_SYSTEM_PROCESSOR=I86 \
  -D CMAKE_BUILD_TYPE=Release
cmake --build build/dos
```

This produces `ODoorl.lib`, links `dos_link_smoke.exe`, and builds the
DOS-compatible `odtest.exe`. The normal GitHub Actions Build workflow runs the
latter under DOSBox with a strict timeout, exercising the 16-bit calling
convention and size-limit checks instead of merely linking them. It publishes
the library, test executables, and emulator log as workflow artifacts.

For a 32-bit flat-model DOS build, select the I386 processor and use the
32-bit compiler driver:

```sh
CC=wcl386 cmake -S dos -B build/dos32 -G "Unix Makefiles" \
  -D CMAKE_SYSTEM_NAME=DOS \
  -D CMAKE_SYSTEM_PROCESSOR=I386 \
  -D CMAKE_BUILD_TYPE=Release
cmake --build build/dos32
```

The default links DOS/4GW-compatible LE executables. To link native DOS/32A
LX executables with the extender embedded, add
`-D OPENDOORS_DOS32_EXTENDER=DOS32A` and use a separate build directory.

This produces `ODOOR32R.lib` for Open Watcom's `-3r` register convention and
`ODOOR32S.lib` for its `-3s` stack convention. The DOS32 examples use `-3r`.
CI builds and runs both library conventions with the native DOS/4GW and
DOS/32A linker systems. It exercises both protected-mode direct UART access
and the FOSSIL serial path through a DPMI conventional-memory buffer.

## Legacy builds

The existing GNU make, Windows make, and DOS build files remain available for
older toolchains. New builds should generally use CMake so generated binaries
stay outside the source tree.
