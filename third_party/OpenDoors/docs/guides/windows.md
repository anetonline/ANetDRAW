# Windows programming

OpenDoors supports native Windows doors built with current 32-bit or 64-bit
MSVC and MinGW toolchains. A Windows door can use a socket inherited from a
BBS, a caller-supplied communications handle, or local mode. The public API is
the same API used on DOS and Unix-like systems, so most door logic should not
need Windows-specific source code.

See [Building and linking](building.md) for the supported CMake options and
commands. This page explains the decisions a Windows application must make at
the source and process-integration level.

## Choosing an application entry point

A console program normally begins in `main()` and receives an `argc`/`argv`
array. A graphical Windows program may instead begin in `WinMain()` and receive
the command line as one string. OpenDoors supports both arrangements.

For a console entry point, pass the argument vector to the explicit
[`od_parse_cmd_line_cons()`](../reference/api/od_parse_cmd_line_cons.md) interface:

```c
#include <OpenDoor.h>

int
main(int argc, char **argv)
{
    od_control.od_prog_name = "Example Door";
    od_parse_cmd_line_cons(argc, argv);
    od_init();

    od_printf("Welcome, %s!\n\r", od_control.user_name);
    od_get_key(TRUE);
    od_exit(0, FALSE);
    return 0;
}
```

As an equally supported alternative, define `OD_WINDOWS_CONSOLE` before
including [`OpenDoor.h`](../reference/api/index.md); the header then aliases
[`od_parse_cmd_line()`](../reference/api/od_parse_cmd_line.md) to
[`od_parse_cmd_line_cons()`](../reference/api/od_parse_cmd_line_cons.md),
allowing the usual portable spelling:

```c
#define OD_WINDOWS_CONSOLE
#include <OpenDoor.h>

int main(int argc, char **argv)
{
    od_parse_cmd_line(argc, argv);
    /* ... */
}
```

The installed `OpenDoors::SharedConsole`, `OpenDoors::StaticConsole`, and
`OpenDoors::StaticMTConsole` CMake targets propagate that definition. They
link the same library binaries as their non-Console counterparts; the target
only selects the source-level parser spelling. Explicitly calling
[`od_parse_cmd_line_cons()`](../reference/api/od_parse_cmd_line_cons.md) works
with either target.

If the application uses `WinMain()`, pass the raw command-line string to the
single-argument form of [`od_parse_cmd_line()`](../reference/api/od_parse_cmd_line.md).
Also copy `nCmdShow` into
[`od_control.od_cmd_show`](../reference/control/customization.md#od_cmd_show)
before initialization so the local window honors the launcher's requested show
state.

```c
#include <OpenDoor.h>

int WINAPI
WinMain(HINSTANCE instance, HINSTANCE previous,
        LPSTR command_line, int show_command)
{
    (void)instance;
    (void)previous;

    od_control.od_cmd_show = show_command;
    od_control.od_prog_name = "Example Door";
    od_parse_cmd_line(command_line);
    od_init();

    od_disp_str("Press a key to return to the BBS.\n\r");
    od_get_key(TRUE);
    od_exit(0, FALSE);
    return 0;
}
```

Do not call both command-line forms for the same launch. The parser applies
standard OpenDoors switches and then invokes any application callbacks, so
parsing the same input twice can process an application option twice.

OpenDoors checks the executable subsystem at run time. Calling the GUI parser
from a console executable, or the console parser from a GUI executable, writes
a diagnostic to standard error and the debugger and terminates with failure
before processing any arguments. Calling
[`od_init()`](../reference/api/od_init.md) directly needs no compile definition:
it detects the subsystem and selects the matching local interface.

Portable sources can use `main()` on every current target. Code which has a
specific need for `WinMain()` can guard only the entry-point wrapper with
[`ODPLAT_WIN32`](../reference/constants/general.md#odplat_win32) and put the
door itself in an ordinary shared function.

## Static and shared libraries

The CMake build produces independent static and shared targets when their
corresponding build options are enabled. Installed CMake packages expose them
as `OpenDoors::Static` and `OpenDoors::Shared`.

When linking `OpenDoors::Shared`, Windows applications link against the import
library and distribute the matching OpenDoors DLL beside the executable or in
another directory on the DLL search path. The executable and DLL must have
compatible architectures: a 64-bit process cannot load a 32-bit DLL, and a
32-bit process cannot load a 64-bit DLL.

When linking `OpenDoors::Static`, the OpenDoors code is incorporated into the
door executable. CMake propagates the [`OD_WIN32_STATIC`](../reference/constants/general.md#od_win32_static) definition through the
target. If a non-CMake build links the static library directly, it must define
[`OD_WIN32_STATIC`](../reference/constants/general.md#od_win32_static) while compiling every translation unit which includes
[`OpenDoor.h`](../reference/api/index.md); otherwise the header declares imported DLL symbols and the final
link will fail or use the wrong linkage attributes.

Use one C runtime model consistently across all static objects in an
application. In particular, do not pass allocations across modules built with
incompatible MSVC runtime settings. The OpenDoors allocation APIs pair their
operations explicitly—for example, memory returned by
[`od_split_cmd_line()`](../reference/api/od_split_cmd_line.md) is released with
[`od_free_split_cmd_line()`](../reference/api/od_free_split_cmd_line.md)—which
also keeps ownership correct across a DLL boundary.

## 32-bit and 64-bit data

Windows uses a flat address space in both supported architectures. There are no
near, far, or huge application pointers, and ordinary allocations are not
limited to 64 KiB. Nevertheless, 64-bit compilation changes pointer-sized
types and the layout of structures containing pointers. Never serialize an
arbitrary in-memory C structure and assume that a different build will read it
correctly.

OpenDoors defines fixed-width types such as [`INT16`](../reference/types.md#int16),
[`INT32`](../reference/types.md#int32), [`WORD`](../reference/types.md#word), and
[`DWORD`](../reference/types.md#dword) for values whose width is part of an
external format. [`DWORD_PTR`](../reference/types.md#dword_ptr) is wide enough
to hold a native handle or pointer-sized integer. Use fixed-width fields in
files and network protocols, write a defined byte order, and version records
whose representation may change.

The public [`tODControl`](../reference/types.md#todcontrol) layout is produced
by [`OpenDoor.h`](../reference/api/index.md) for the selected compiler; MSVC receives explicit byte packing,
while the header does not apply a universal packed attribute to every Windows
compiler. Include the header rather than reproducing the layout. An application
built with a different ABI or architecture must use the matching OpenDoors
library, and no packing rule makes pointers or Windows handles interchangeable
between 32-bit and 64-bit processes.

## Communication handles and Door32 sessions

Most present-day Windows BBS software launches a door with a connected socket
described by `door32.sys` or with equivalent command-line information.
OpenDoors recognizes that session during initialization and uses the supplied
socket instead of opening a physical serial port. The numeric value is retained
in [`od_control.od_open_handle`](../reference/control/connection.md#od_open_handle),
and [`od_control.od_use_socket`](../reference/control/connection.md#od_use_socket)
selects socket semantics.

The handle belongs to the process according to the launch protocol. A parent
which supplies a live handle must make it inheritable or duplicate it into the
door process. A decimal value copied from another process is not useful by
itself. The BBS and door must also agree on whether the value denotes a Winsock
socket or a serial handle; the operations and shutdown rules are different.

Set a handle manually only when the launcher contract requires it. Do so before
calling [`od_init()`](../reference/api/od_init.md) or any API function which can
trigger automatic initialization. Once initialization has selected and opened
the transport, changing the field does not migrate the active connection.

For a socket session, outgoing bytes are sent to the connected peer and remote
input is read into OpenDoors' common input queue. Carrier state means that the
socket remains connected, not that a physical modem's carrier-detect pin is
high. [`od_set_dtr()`](../reference/api/od_set_dtr.md) has no useful modem line
to manipulate on such a connection.

Physical serial operation remains available for compatible configurations,
but current applications should prefer a live handle supplied by the BBS. It
avoids two processes competing to open the port and preserves the BBS's chosen
line settings. Historic Windows 95 `SYSTEM.INI` serial-port arbitration and
external DTR restoration utilities are not requirements of current Windows
systems and are not part of the supported setup.

## Local and remote displays

The remote terminal and the local Windows display are distinct endpoints.
OpenDoors sends door output through its terminal model, updates the virtual
screen, and presents that state locally. The local console or window size does
not define the remote caller's screen size. This distinction is important when
the caller uses a larger terminal than the local display.

Block operations such as [`od_gettext()`](../reference/api/od_gettext.md),
[`od_puttext()`](../reference/api/od_puttext.md), and
[`od_scroll()`](../reference/api/od_scroll.md) operate on the OpenDoors screen
model. They must not be replaced with direct Win32 console reads or writes:
doing that bypasses remote output, changes behavior according to local window
dimensions, and can expose local-only screen contents to the caller.

The established whole-screen snapshot functions preserve their original fixed
buffer contract. New code which needs arbitrary terminal dimensions should
use [`od_save_screen_size()`](../reference/api/od_save_screen_size.md),
[`od_save_screen_ex()`](../reference/api/od_save_screen_ex.md), and
[`od_restore_screen_ex()`](../reference/api/od_restore_screen_ex.md). The size
query lets the caller allocate exactly the storage required by the current
virtual screen and lets restoration validate the snapshot before using it.

[`od_control.od_silent_mode`](../reference/control/runtime.md#od_silent_mode)
suppresses the normal local user interface. It does not make the remote session
local, and it does not suppress output to the caller. Conversely,
[`od_control.od_force_local`](../reference/control/customization.md#od_force_local)
selects a local session and ignores a remote drop-file connection. These
settings solve different problems.

A console-subsystem application which requests the local interface first uses
its inherited Windows console. If it has none, OpenDoors allocates a console
and releases that console during shutdown. A non-silent local-mode session
terminates with the configured error level if no usable console can be
inherited or allocated; it is never allowed to continue with a missing local
display. A remote session may instead fall back to silent operation when
console creation fails.
[`od_control.od_silent_mode`](../reference/control/runtime.md#od_silent_mode)
continues to bypass console creation and console state management.

## Threads and serialized API access

The GUI-subsystem interface uses one Windows frame/UI worker. A
console-subsystem application starts no OpenDoors worker: console keyboard
input, status handling, communications, and timers all run in the same
cooperative application flow as DOS. Internal synchronization remains compiled
into both modes because the same library binary and input-queue implementation
also serve GUI producer/consumer operation. The public API and ABI are
process-global and are not internally serialized.

An application may hand the session between its threads, but only one thread
may use OpenDoors at a time. Protect every API call, direct access to
[`od_control`](../reference/control/index.md) or another public global, and the
complete useful lifetime of a returned OpenDoors pointer with the same
application lock. On Windows, a `CRITICAL_SECTION` is a convenient choice
because it permits recursive entry by the same thread. The lock must remain
held for the complete public call, including while that call waits internally.

In GUI mode, OpenDoors publishes the fields needed by its UI at API boundaries.
The private UI worker does not make concurrent application access safe.

Callbacks configured through [`od_control`](../reference/control/index.md)
normally run synchronously on the application thread making the active API
call and may call the API recursively. A wrapper which acquires the application
lock must therefore use recursive synchronization. The Windows help and
configuration callbacks are the exceptions: they retain their frame-thread
context and must not access any OpenDoors function, global, or returned
pointer. They should queue application-owned work and return promptly.

Long application computations should periodically call
[`od_kernel()`](../reference/api/od_kernel.md) from the serialized application
flow. Most API calls invoke the kernel themselves, so ordinary input/output loops require no
special pumping. A loop which spends several seconds doing only application
work must allow OpenDoors to observe disconnects, time limits, and local
operator commands.

## Application metadata and native resources

Set [`od_control.od_prog_name`](../reference/control/customization.md#od_prog_name),
[`od_control.od_prog_version`](../reference/control/customization.md#od_prog_version),
and
[`od_control.od_prog_copyright`](../reference/control/customization.md#od_prog_copyright)
before initialization. The Windows interface uses these strings when it labels
the application and constructs its About information.

[`od_control.od_app_icon`](../reference/control/customization.md#od_app_icon)
accepts an application-provided `HICON`. The application owns the icon and must
keep it valid while OpenDoors may display it. A resource icon can be loaded
before initialization with the normal Windows resource APIs. The field exists
only when [`ODPLAT_WIN32`](../reference/constants/general.md#odplat_win32) is selected, so portable source must guard direct
access to it.

[`od_control.od_help_callback`](../reference/control/customization.md#od_help_callback)
allows the local interface to invoke application help. The callback should
display or launch help without changing the active transport. It is a local
operator facility; remote users do not receive a Windows help window.

The library contains its own Windows version resources. An application should
also provide version information for its executable so Explorer and deployment
tools report the door's identity independently of the OpenDoors DLL.

## Shutdown and error reporting

Use [`od_exit()`](../reference/api/od_exit.md) for normal termination. It writes
back supported drop-file state, runs the configured before-exit callback,
shuts down OpenDoors components, and optionally disconnects the caller. Calling
the C runtime `exit()` directly skips those responsibilities.

If startup fails, inspect [`od_control.od_error`](../reference/control/runtime.md#od_error)
and the [`ERR_*`](../reference/constants/errors.md) reference. Preserve the
drop file and exact command line when reproducing launch failures. For handle
problems, also record the process architecture, whether the handle is a socket
or serial object, and how the parent made it available to the child.

Windows loader failures happen before `main()` or `WinMain()` and therefore
cannot be reported by OpenDoors. A missing DLL, architecture mismatch, or
missing dependent runtime must be diagnosed with the Windows event log, loader
diagnostic tools, or a terminal launch which keeps the error visible. Static
linking removes the OpenDoors DLL deployment step but does not remove the need
to deploy the runtime selected by the application's toolchain.
