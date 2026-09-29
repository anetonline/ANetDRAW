# Public integration acceptance tests

These tests are downstream consumers of the installed OpenDoors package. They
include only `OpenDoor.h` and `ODStat.h`; implementation headers and private
test hooks are deliberately unavailable.

`public-contract.json` inventories every public function, global, type,
constant, compatibility alias, personality helper, and `tODControl` field. Run
the checker after changing either public header:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 tools/check_public_contract.py
```

`generate_public_contract.py` prints a replacement manifest for review. It
does not modify the tree. Coverage assignments should be reviewed along with
the implementation and reference documentation rather than accepted merely
because they were generated.

To exercise an installed shared package, use:

```sh
cmake -S tests/acceptance -B build/acceptance \
  -DCMAKE_PREFIX_PATH=/path/to/opendoors \
  -DOPENDOORS_VARIANT=Shared \
  -DOPENDOORS_ACCEPTANCE_EXTENDED=ON
cmake --build build/acceptance
ctest --test-dir build/acceptance --output-on-failure
```

When cross-running Windows integration and regression binaries through Wine,
use `tools/wine-ctest` instead of invoking CTest directly. The runner creates
a private Xvfb display and temporarily configures Wine's crash debugger to
write noninteractive minidumps. Every new dump is moved to the requested
artifact directory under a unique name, analyzed with WineDbg, and reported
on standard error before the original crash-handler setting is restored.

```sh
tools/wine-ctest --display :91 \
  --wine /usr/local/bin/wine64.bin --prefix "$HOME/.wine" \
  --artifacts build/wine64/crashes \
  --test-dir build/wine64 --output-on-failure
```

On FreeBSD, run 32-bit tests through the packaged `/usr/local/bin/wine`
wrapper and set `WINE_i386_ROOT` when the i386 package tree is not in its
default location. Do not point the test emulator directly at the i386
`wine.bin`: the wrapper supplies the library mappings needed when a tested
program creates another Windows process.

```sh
WINE_i386_ROOT="$HOME/.i386-wine-pkg" \
tools/wine-ctest --display :92 \
  --wine /usr/local/bin/wine --prefix "$HOME/.wine" \
  --artifacts build/wine32/crashes \
  --test-dir build/wine32 --output-on-failure
```

The ordinary smoke-integration suite contains deterministic compile, ABI,
lifecycle, screen, configuration, spawn, and drop-file scenarios. The extended
setting adds eight transport-neutral public-API scenarios for input, interactive
prompts, formatted and multiline editing, display and files, terminal
emulation, `FILES.BBS` listings, popup/window behavior, and connection/session
behavior. Unix runs them
over an inherited socket, Windows uses loopback TCP, and the DOS build reuses
the same protocol over direct UART and FOSSIL serial paths. The DOS controller
places a modem emulator online before launching the door, then hangs up during
the session scenario so both UART and FOSSIL builds must observe carrier loss.
Every enabled transport must satisfy the same scenario assertions; only
endpoint setup and the selected `od_com_method` differ.

GitHub Actions runs the ordinary cases on pushes and pull requests. The
extended matrix runs nightly when the default branch has advanced since the
last successful scheduled run, or unconditionally on request, and is required
for releases. It mirrors the shipped SDK configurations: both architectures
and all three library/runtime variants for MSVC, shared and static libraries
for every MinGW SDK, shared and static libraries for both macOS release
architectures and deployment targets, both Open Watcom and Turbo C 2.01
16-bit libraries over direct UART and FOSSIL, and both Open Watcom 32-bit
calling conventions over UART and FOSSIL with DOS/4GW and DOS/32A. Linux
shared and static builds remain in the matrix as source-platform coverage even
though no Linux binary SDK is shipped. The extended workflow also publishes a
single Clang/Unix branch-coverage report. That report is directional and has
no percentage or delta gate; Windows and DOS runtime results remain
authoritative for their platform-specific paths.

Interactions which would need private mocks are recorded separately in
[`../COMPONENT_TEST_BACKLOG.md`](../COMPONENT_TEST_BACKLOG.md). Acceptance
tests must not add production hooks or substitutes to reach those branches.

On Windows, `acceptance.windows_screen` also creates the real local frame and
screen child, forces a published generation through `WM_PAINT`, and injects a
local key while the application waits in `od_get_input()`. The helper uses only
Win32 window operations. `acceptance.windows_thread_handoff` verifies that an
application lock can serialize API calls and shutdown across different caller
threads.

Acceptance expectations describe the behavior of the current implementation;
the test suite does not introduce an unreviewed behavior change to make a
preferred expectation pass.
