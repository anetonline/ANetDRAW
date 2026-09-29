# Troubleshooting

OpenDoors sits between a door, a BBS launcher, a drop file, a local operator
display, and a remote byte stream. A useful diagnosis identifies which of
those boundaries first differs from the expected behavior. Start with a small,
repeatable case and record the complete launch environment before changing
code.

## Collect a reproducible case

Record the following information for every report:

- the OpenDoors version or commit, build type, target platform, architecture,
  and compiler;
- whether the door links `OpenDoors::Static` or `OpenDoors::Shared`;
- the door executable's exact command line and working directory;
- the drop-file name, directory, and contents, with passwords and other private
  caller data removed;
- whether the session is local, serial, FOSSIL, or socket based;
- terminal type, terminal dimensions, and whether CP437-to-UTF-8 conversion is
  enabled;
- the first failing API operation and the value of
  [`od_control.od_error`](../reference/control/runtime.md#od_error) immediately
  afterward; and
- the smallest input or sequence of local and remote actions which reproduces
  the problem.

Do not begin by copying the entire door into a new test harness. First check
whether one of the shipped examples fails under the same launcher. If an
example succeeds, reduce the application toward the first differing setup or
API call. If it also fails, the BBS handoff, OpenDoors configuration, or build
is a more likely starting point.

When the failure is intermittent, add timestamps and node identifiers to the
application log. Preserve the first failure rather than repeatedly restarting
until the evidence has been overwritten.

## Read `od_error` at the point of failure

Most API failures set [`od_control.od_error`](../reference/control/runtime.md#od_error)
to an [`ERR_*`](../reference/constants/errors.md) value. Read it immediately:
a later successful or unsuccessful call may replace the diagnostic.

```c
if (!od_send_file("welcome.ans")) {
    int open_doors_error = od_control.od_error;
    od_log_write("welcome display failed: OpenDoors error %d",
                 open_doors_error);
}
```

An OpenDoors error is not necessarily a C `errno` or Windows `GetLastError()`
value. Capture each error facility immediately after the operation which owns
it. If application code performs another library or system call before saving
the value, the original cause may be lost.

[`ERR_PARAMETER`](../reference/constants/errors.md#err_parameter) normally
means that a pointer is null, a coordinate rectangle is invalid, a supplied
buffer is too small, or an option combination is not supported by the call.
Compare the arguments with that function's complete reference rather than
assuming the transport failed.

[`ERR_FILEOPEN`](../reference/constants/errors.md#err_fileopen) reports that a
documented API file-open operation failed. Drop-file discovery does not assign
a separate public error constant: if initialization cannot read a required
door-information file and the no-file callback does not recover, it displays
an initialization error and exits with the configured critical error level.
Record the working directory and
[`od_control.info_path`](../reference/control/connection.md#info_path); relative
paths are resolved in that context, not relative to the executable's location.

## Build and link failures

If the compiler cannot find [`OpenDoor.h`](../reference/api/index.md), use the installed CMake package or
add the directory containing the header to the compiler's include search path.
Do not copy a header from a different version into the application source
tree. The header and library form one interface and must be upgraded together.

An undefined public API symbol at link time usually means that the OpenDoors
target is absent, the wrong architecture was selected, C++ linkage was
redeclared incorrectly, or a Windows static build omitted [`OD_WIN32_STATIC`](../reference/constants/general.md#od_win32_static).
Prefer linking the imported targets `OpenDoors::Static` or
`OpenDoors::Shared`; they propagate include directories, platform libraries,
and required compile definitions.

A duplicate [`od_control`](../reference/control/index.md) definition means application code defined the global
instead of using the declaration from [`OpenDoor.h`](../reference/api/index.md), or linked two OpenDoors
libraries. Include the header and use the exported object. Do not declare a
second unqualified `tODControl od_control` in application source.

For DOS, select a library matching the compiler, memory model, and calling
convention documented by the corresponding build. The 16-bit and DOS32
libraries are not interchangeable. A fixup overflow or truncated pointer is
usually evidence of a model mismatch, an object larger than the selected
segment permits, or an incorrect near/far function-pointer declaration. Build
one shipped DOS example with the same options before changing OpenDoors.

Warnings about implicit function declarations, incompatible pointers, or
integer-to-pointer conversions are not harmless portability noise. They can
change the generated calling sequence and become crashes only on one
architecture. Include [`OpenDoor.h`](../reference/api/index.md), use its callback types, and resolve the
warning at its source.

## The door starts and immediately exits

Launch the door from a terminal or capture its standard error so an operating
system or loader message remains visible. On Windows, a missing or wrong-width
DLL prevents the application entry point from running. On Unix-like systems,
inspect the dynamic loader's dependency report. In both cases, confirm that the
application and OpenDoors library have the same architecture.

If OpenDoors starts but terminates the session, inspect the drop-file path,
transport selection, carrier state, and available time. Automatic carrier and
time enforcement occurs during API calls and
[`od_kernel()`](../reference/api/od_kernel.md). Temporarily masking a check with
[`od_control.od_disable`](../reference/control/customization.md#od_disable) can
isolate the cause, but it is not a substitute for correcting the launcher or
session data.

Use local mode to separate core door logic from the remote handoff. If the door
runs locally but not under the BBS, compare the BBS working directory, command
line, inherited handle, and drop file. If it fails in both modes, reduce the
application's initialization settings and inspect the first API call.

[`od_control.od_force_local`](../reference/control/customization.md#od_force_local)
must be set before initialization. Setting it after a different transport is
open does not convert the session.

## Drop file is not found or is parsed incorrectly

OpenDoors searches according to
[`od_control.info_path`](../reference/control/connection.md#info_path), standard
command-line options, configuration, and the current directory. Supply a full
file name when a directory contains multiple recognized formats and selection
must be unambiguous. In multi-node installations, do not let several nodes
share one writable drop file.

After initialization,
[`od_control.od_info_type`](../reference/control/connection.md#od_info_type)
reports the selected format. It is an output describing what was parsed, not a
general request to reinterpret already loaded data. Check it before relying on
fields which only certain formats supply.

Examine the file as bytes as well as text. DOS line endings, fixed-width
records, embedded nul bytes, and legacy encodings can be hidden by a modern
editor. Do not save a binary `EXITINFO.BBS` through a text editor. For a custom
format, verify each declared line keyword and conversion against
[Custom door-information formats](door-information-formats.md).

Fields absent from a format retain defaults. An empty phone number or zero
message count does not by itself prove a parser failure. Compare the field with
the format's documented availability and, where practical, provide an
application default.

## No remote input or output

First determine the active communication method through
[`od_control.od_com_method`](../reference/control/connection.md#od_com_method),
[`od_control.od_use_socket`](../reference/control/connection.md#od_use_socket),
[`od_control.port`](../reference/control/connection.md#port), and
[`od_control.od_open_handle`](../reference/control/connection.md#od_open_handle).
Do not diagnose a socket session as though it were a UART or FOSSIL session.

For inherited sockets and handles, confirm that the launcher actually made the
object available to the child process. Handle values are process-local unless
the operating system's inheritance or duplication rules are followed. Confirm
whether the numeric command-line value is decimal or hexadecimal and whether
it denotes a socket or serial object.

Use [`od_carrier()`](../reference/api/od_carrier.md) to observe OpenDoors'
current connection state. A socket close, modem carrier loss, and forced local
mode have different meanings even though each can explain a lack of remote
traffic.

If input bytes arrive but function or cursor keys are delayed, capture the
actual received sequence and its byte timing. The current input parser waits up
to 250 milliseconds between bytes of a possible control sequence. That delay
is fixed by `MAX_CHARACTER_LATENCY` in `ODGetIn.c`;
[`od_control.od_max_key_latency`](../reference/control/runtime.md#od_max_key_latency)
is retained but not read by the implementation.

For output encoding problems, record both the application's source bytes and
the on-wire result. [`od_control.od_cp437_to_utf8_out`](../reference/control/connection.md#od_cp437_to_utf8_out)
converts outgoing CP437 where supported; enabling it for a CP437 terminal or
disabling it for a UTF-8 consumer produces visibly different corruption.
ANSI/AVATAR emulation and character encoding are separate layers.

## Local display differs from the remote terminal

The local window is a presentation of OpenDoors' virtual screen; it is not the
remote terminal itself. Verify the remote dimensions in
[`od_control.user_screenwidth`](../reference/control/caller.md#user_screenwidth)
and
[`od_control.user_screen_length`](../reference/control/caller.md#user_screen_length).
Do not infer them from the Windows console, Unix terminal, or DOS video mode.

If only direct application console output appears locally, the application is
bypassing OpenDoors. Use [`od_disp_str()`](../reference/api/od_disp_str.md),
[`od_printf()`](../reference/api/od_printf.md), or another OpenDoors output
operation for text intended for both endpoints. Direct `printf()`, Win32
console calls, and platform video-memory writes do not enter the common screen
and terminal path.

For a block-operation failure, validate that the rectangle is ordered, within
the active virtual screen, and represented in the format the function expects.
New code which saves a whole screen of arbitrary dimensions should use the
extended snapshot API. The established
[`od_save_screen()`](../reference/api/od_save_screen.md) contract is fixed for
source and binary compatibility.

If [`od_clr_scr()`](../reference/api/od_clr_scr.md) appears to do nothing, check
the caller's screen-clearing preference and
[`od_control.od_always_clear`](../reference/control/customization.md#od_always_clear).
Do not replace the call with a hard-coded terminal sequence unless the door
intentionally wants to override OpenDoors' terminal handling and caller policy.

## Input, menus, and editors behave unexpectedly

[`od_get_key()`](../reference/api/od_get_key.md) returns a logical key value;
extended keys are represented by the [`OD_KEY_*`](../reference/constants/input.md#function-keys) definitions rather than a
portable single-byte character. Compare against those constants and do not
truncate the result into `char` before checking it.

[`od_get_input()`](../reference/api/od_get_input.md) distinguishes character,
special-key, mouse, timeout, and other event results. Initialize the event
structure as documented, inspect its returned event type, and only read the
union member associated with that type.

For popup menus, keep selection indexes, hotkeys, and the [`MENU_*`](../reference/constants/display.md#popup-menu-flags) behavior
flags distinct. [`MENU_ALLOW_CANCEL`](../reference/constants/display.md#menu_allow_cancel) changes whether cancellation is accepted;
[`MENU_KEEP`](../reference/constants/display.md#menu_keep) and [`MENU_DESTROY`](../reference/constants/display.md#menu_destroy) control window lifetime; [`MENU_PULLDOWN`](../reference/constants/display.md#menu_pulldown) changes
navigation behavior. A value which happens to work as both a Boolean and a
flag is not necessarily valid in both parameter positions.

The single-line and multiline editors require buffer capacities, visible
widths, and result flags to agree. A displayed field width is not automatically
the allocation size. When an editor can grow a buffer, follow its ownership
rules and preserve the returned pointer rather than continuing to use the old
one.

## Time limits, inactivity, and carrier exits

OpenDoors enforces remaining time and carrier state while its kernel runs. Most
API calls run it automatically. Application code which performs long CPU work,
blocking file access, or its own wait loop without calling OpenDoors must call
[`od_kernel()`](../reference/api/od_kernel.md) periodically while holding the
application's OpenDoors serialization lock.

Compare the BBS-provided time limit with
[`od_control.od_maxtime`](../reference/control/customization.md#od_maxtime) and
[`od_control.od_maxtime_deduction`](../reference/control/runtime.md#od_maxtime_deduction).
The door-specific cap can reduce the effective time even when the drop file
contains a larger value.

Inactivity has both a total timeout and a warning interval. If a warning and
disconnect seem simultaneous, verify that
[`od_control.od_inactive_warning`](../reference/control/runtime.md#od_inactive_warning)
is smaller than the configured inactivity limit. Local and remote input may
affect activity accounting differently depending on whether local keyboard
input is enabled.

Do not permanently set [`DIS_CARRIERDETECT`](../reference/constants/session.md#dis_carrierdetect), [`DIS_TIMEOUT`](../reference/constants/session.md#dis_timeout), or related mask bits
merely to make a test pass. Use a temporary change to identify which mechanism
is firing, then correct the stale handle, incorrect drop-file value, or missing
kernel service.

## File display and process spawning

For [`od_send_file()`](../reference/api/od_send_file.md) and
[`od_send_file_section()`](../reference/api/od_send_file_section.md), verify the
resolved file name, access permissions, section markers, terminal format, and
whether the caller stopped or paused output. A file can open successfully and
still display incorrectly because its encoding or control language does not
match the selected mode.

When a spawned program cannot be found, remember that its search path and
working directory come from the door process. Use an absolute executable path
while diagnosing. Preserve arguments as separate strings with
[`od_spawnvpe()`](../reference/api/od_spawnvpe.md); do not build an unquoted
shell command unless shell interpretation is explicitly required.

On DOS, swapping and available conventional memory can determine whether a
child starts. On modern flat-memory platforms, a launch failure is more likely
to be path, permission, executable-format, or handle-inheritance related.
[`od_control.od_spawn_freeze_time`](../reference/control/customization.md#od_spawn_freeze_time)
controls OpenDoors time accounting during a synchronous child; it does not
alter the operating system's process timer.

## Multi-node corruption and locking

Give each node its own drop-file directory and other per-session files. Shared
application data must use an interprocess locking scheme recognized by every
writer. A thread mutex protects threads in one process only and is not a
multi-node file lock.

Log the node number, record identifier, lock attempt, and final write. If a
corrupt record appears only under load, test with two independently launched
door processes rather than two threads. See [Multi-node operation](multinode.md)
for the complete transaction and recovery model.

Never “repair” a shared file by rewriting it while other nodes remain active.
Make a copy, stop or exclude writers, determine the last known complete
transaction, and restore through the application's defined recovery procedure.

## Internal diagnostics and reporting a defect

[`od_control.od_internal_debug`](../reference/control/customization.md#od_internal_debug)
enables additional implementation diagnostics where the platform supports
them. Diagnostic output can contain paths, drop-file values, caller details,
and bytes from a session; review it before publishing.

A useful defect report includes the reproducible case, exact build identity,
expected and observed results, OpenDoors error value, relevant log excerpt, and
the smallest source example. For a crash, include a symbolic stack trace and
state whether sanitizers or compiler runtime checks reported an earlier error.
For a regression, identify the last known good release or commit if possible.

Submit current project reports through the repository's issue tracker. Search
existing reports first, but add a new reproducible case when the platform,
transport, or failure point differs materially. Historic telephone numbers,
BBS addresses, mailing lists, registration channels, and personal contact
addresses are not support interfaces for this repository.
