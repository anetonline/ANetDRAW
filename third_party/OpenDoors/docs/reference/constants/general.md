# General and platform definitions

These definitions identify the OpenDoors interface, selected target, linkage,
logical values, timeout convention, and child-process mode. Applications obtain
them by including [`OpenDoor.h`](../api/index.md); they should not reproduce the platform
selection logic themselves.

## Version and path definitions

### `OD_VERSION`

`OD_VERSION` is the hexadecimal API version encoded as one hexadecimal digit
for the major version followed by two digits for the minor version. The current
value is `0x630`, representing OpenDoors 6.30.

Use this macro for conditional source compatibility with the installed header:

```c
#if OD_VERSION >= 0x630
    size = od_save_screen_size();
#endif
```

This is a compile-time interface value. It does not inspect a DLL found at run
time and is not a substitute for the operating system's binary loader checks.
The project follows the compatibility rules in
[Versioning and compatibility](../../guides/versioning.md).

### `DIRSEP`

`DIRSEP` is the native directory separator as a character constant. It expands
to `'\\'` on Windows and DOS and `'/'` on Unix-like systems. Use it while
constructing a character buffer one element at a time.

### `DIRSEP_STR`

`DIRSEP_STR` is the same native separator as a nul-terminated string literal.
It is useful in adjacent string literals and formatting operations:

```c
const char *name = "data" DIRSEP_STR "scores.dat";
```

Neither macro normalizes an existing path, supplies a volume name, or checks
the resulting length. Prefer the platform or CMake path facilities when more
than simple source portability is required.

## Platform selection

Exactly one of the following `ODPLAT_*` macros describes the target selected by
[`OpenDoor.h`](../api/index.md). Test these macros with `#if defined(...)`; their mere presence is
the interface and their replacement text is not meaningful.

### `ODPLAT_WIN32`

`ODPLAT_WIN32` identifies both 32-bit and 64-bit native Windows builds. The
name describes the Win32 API family, not the pointer width. Windows types and
calling conventions are active, and the platform-specific members
[`od_app_icon`](../control/customization.md#od_app_icon) and
[`od_cmd_show`](../control/customization.md#od_cmd_show) are present.

### `ODPLAT_NIX`

`ODPLAT_NIX` identifies supported Unix-like builds, including macOS. These
targets use a flat memory model, POSIX-style paths, and the virtual local screen
implementation.

### `ODPLAT_DOS`

`ODPLAT_DOS` identifies the established 16-bit DOS implementation. Memory
model and far-pointer distinctions remain meaningful. Public declarations use
the compiler-specific conventions selected by the header.

### `ODPLAT_DOS32`

`ODPLAT_DOS32` identifies the Open Watcom 32-bit, flat-memory DOS target. The
header rejects non-Watcom compilers, non-386 compilation, a non-flat memory
model, and the unsupported `-ri` return convention. This platform is separate
from the 16-bit DOS implementation even though both run under DOS.

Do not define an `ODPLAT_*` macro manually to cross-compile a source file. Set
up the correct compiler and target so [`OpenDoor.h`](../api/index.md), the C runtime, object files,
and OpenDoors library all agree.

## Windows library selection

### `OD_WIN32_STATIC`

A Windows consumer linking the static OpenDoors library must define
`OD_WIN32_STATIC` before including [`OpenDoor.h`](../api/index.md). This prevents public objects
and functions from being declared as DLL imports. The installed
`OpenDoors::Static` CMake target supplies the definition automatically.

Do not define it when linking the DLL import library. Mixing translation units
compiled with different settings gives inconsistent declarations for the same
public interface.

### `OD_DLL`

On Windows, [`OpenDoor.h`](../api/index.md) defines `OD_DLL` unless `OD_WIN32_STATIC` was supplied.
It tells the header to select DLL import or export annotations. Application
code may test it when an unavoidable deployment distinction exists, but should
normally test behavior or use the CMake target instead.

### `BUILDING_OPENDOORS`

`BUILDING_OPENDOORS` is defined while compiling the library itself. It changes
public declarations from imports to exports and must not be defined by an
application. Defining it in consumer code can produce duplicate exports or
incorrect visibility.

### `OPENDOORS_USE_DEF_EXPORTS`

The Windows library build may define `OPENDOORS_USE_DEF_EXPORTS` when a module
definition file, rather than `__declspec(dllexport)`, supplies exported names.
It is a build-system implementation switch exposed because it affects the
header declarations. Applications neither define nor depend on it.

## Linkage and calling-convention macros

These macros are part of public declarations and are documented so callbacks,
foreign-function bindings, and established sources can reproduce the required
type. Ordinary C and C++ callers should use the declarations already present
in [`OpenDoor.h`](../api/index.md).

### `OD_NAMING_CONVENTION`

In C++, `OD_NAMING_CONVENTION` expands to `extern "C"` so exported names use C
linkage. In C it expands to nothing. It controls language linkage, not the
machine-level argument convention.

### `OD_EXPORT`

`OD_EXPORT` marks a library definition with the target's public symbol
visibility. On a Windows DLL build it normally expands to
`__declspec(dllexport)`; on a supported shared-object build it may use default
ELF/Mach-O visibility. Its exact spelling is compiler dependent.

### `OD_IMPORT`

`OD_IMPORT` marks a declaration imported from the OpenDoors library. On Windows
it uses the platform import annotation where applicable. On the current
non-Windows consumer builds it expands to no annotation.

### `ODCALL`

`ODCALL` is the calling convention for non-variadic public functions. It is
`WINAPI` on Windows. On DOS32 it selects `__watcall` by default and `__cdecl`
for a library built with Open Watcom's `-3s` convention. Other targets use the
compiler default represented by an empty macro.

An application callback whose field declaration includes the OpenDoors
callback convention must match it exactly. A mismatch may compile and still
corrupt arguments or the stack when invoked.

### `ODVCALL`

`ODVCALL` is the calling convention for variadic public functions such as
[`od_printf()`](../api/od_printf.md). Variadic functions require the C calling
convention on Windows and DOS32 because the callee cannot use a convention
which assumes a fixed argument list.

### `OD_GLOBAL_CONV`

`OD_GLOBAL_CONV` preserves the established platform convention attached to the
exported [`od_control`](../control/index.md) object where the toolchain requires one. Applications
should use the declaration in the header instead of placing this macro on a
new object.

### `ODAPIDEF`

`ODAPIDEF` combines C language linkage with the correct import or export
visibility for a public API function. It deliberately does not include
`ODCALL`, because each prototype places the appropriate fixed or variadic
calling convention separately.

### `OD_API_VAR_DEFN`

`OD_API_VAR_DEFN` is the linkage and visibility prefix used for a public global
definition inside OpenDoors. It is primarily relevant to building the library.

### `OD_API_VAR_DECL`

`OD_API_VAR_DECL` is the corresponding `extern` declaration prefix for public
global data. It selects import or export visibility according to whether the
library is being built.

### `ODFAR`

`ODFAR` expands to the compiler's `far` keyword on 16-bit DOS and to nothing on
flat-memory targets. It keeps declarations source-compatible without inventing
a `malloc_huge`-style allocation abstraction. A pointer is only far where the
actual target ABI requires it.

## Logical definitions

### `FALSE`

`FALSE` is the zero value for [`BOOL`](../types.md#bool). OpenDoors defines it
only when the platform headers have not already done so.

### `TRUE`

`TRUE` is the conventional true value for [`BOOL`](../types.md#bool). Code
receiving a logical result should still test for nonzero rather than equality
with `TRUE`, since C permits any nonzero value to mean true.

### `MAYBE`

`MAYBE` is the compatibility name for the value `2`. The current source does
not test the macro by name. Established code used it for fields such as
[`od_okaytopage`](../control/runtime.md#od_okaytopage), where 0 disables paging, 1 enables it unconditionally, and any
other value follows the configured hours. New code should use that field's
specific name [`PAGE_USE_HOURS`](session.md#page_use_hours). `MAYBE` is not an
indeterminate result returned by ordinary Boolean functions.

## Timeout definition

### `OD_NO_TIMEOUT`

`OD_NO_TIMEOUT` is the [`tODMilliSec`](../types.md#todmillisec) value requesting
an indefinite wait. On Windows it is the system `INFINITE` value; elsewhere it
is the equivalent unsigned 32-bit value. Pass it only to an API parameter which
accepts a millisecond timeout.

An indefinite OpenDoors input wait continues to service required session
processing. It does not promise that the call returns only with a character:
disconnect, time-limit, or other documented session termination can still end
processing.

## Child-process modes

### `P_WAIT`

`P_WAIT` tells [`od_spawnvpe()`](../api/od_spawnvpe.md) to wait until the child
finishes and return its result. This is the only child mode supported by the
DOS implementations. OpenDoors continues the platform-specific session policy
described by the spawn function and
[`od_spawn_freeze_time`](../control/customization.md#od_spawn_freeze_time).

### `P_NOWAIT`

`P_NOWAIT` requests asynchronous process creation on targets which support it.
[`od_spawnvpe()`](../api/od_spawnvpe.md) returns `-1` when the asynchronous
child cannot be launched; any other value reports a successful launch. The
successful value is platform dependent and is not a portable process
identifier. Passing this value on DOS, which supports only `P_WAIT`, returns
`-1` and sets [`od_control.od_error`](../control/runtime.md#od_error) to
`ERR_PARAMETER`.

On Windows and DOS32 these names are obtained from `<process.h>`; OpenDoors
defines compatible values where the C runtime does not provide them.

### `CURRENT` and `IRET`

`CURRENT` and `IRET` are established DOS swapping implementation markers with
values zero and one. They are exposed by the public header for source
compatibility, but they are not valid [`od_spawnvpe()`](../api/od_spawnvpe.md) modes and new application
code should not use them.
