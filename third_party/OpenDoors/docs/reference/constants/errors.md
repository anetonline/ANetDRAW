# Errors and exit levels

OpenDoors reports API failures through a function's return value and
[`od_control.od_error`](../control/runtime.md#od_error). Process exit levels are
a separate mechanism used when the session terminates.

## Reading API errors

Check the function's documented success result first. If it reports failure,
save [`od_control.od_error`](../control/runtime.md#od_error) before making another OpenDoors call. A successful
call is not required to clear an earlier error, and a later failure can replace
the value.

The error constants identify an OpenDoors condition. They are not C `errno`
values, Win32 error codes, socket errors, or process exit levels. Code which
needs all of those diagnostics must capture each one at the point where its
own subsystem failed.

### `ERR_NONE`

No OpenDoors error has been recorded. This is the initial value and does not,
by itself, prove that an operation whose return value was ignored succeeded.

### `ERR_MEMORY`

OpenDoors could not allocate the memory required to complete the operation.
The request may be small even when the application has free memory: a 16-bit
DOS memory model can also fail because an object or contiguous segment cannot
be represented. Reduce the requested object, free application storage, and
verify that the correct DOS memory model or modern architecture is linked.

The current multiline editor does not set this error merely because its
application reallocation callback returns null; that failure is reflected in
the editor result instead. Consult the individual function rather than
assuming that every allocation-like callback maps to `ERR_MEMORY`.

### `ERR_NOGRAPHICS`

The operation requires a supported ANSI, AVATAR, or RIP terminal mode and the
current caller does not have one enabled. This commonly applies to cursor,
window, and block-screen operations whose result cannot be represented on a
plain-text terminal.

Check the caller capability fields after initialization or use
[`od_autodetect()`](../api/od_autodetect.md) where probing is appropriate.
Forcing the flag does not add capabilities to the remote terminal.

### `ERR_PARAMETER`

One or more arguments are invalid. Examples include a null required pointer,
an inverted or out-of-range rectangle, a buffer smaller than the required
snapshot, an invalid option combination, or a saved object whose header does
not match the operation.

This error describes the call contract, not remote data. Consult the function
page for exact bounds and permitted null arguments. When the API offers a size
query, call it rather than guessing a capacity.

### `ERR_FILEOPEN`

A required file could not be opened. The path may not exist, may refer to a
directory, may be inaccessible to the current user, or may be locked in a mode
incompatible with the requested operation. Record the door's working directory
when diagnosing relative paths.

`ERR_FILEOPEN` says that an open or initial file-discovery step failed. The
current display-file functions do not distinguish a later `fgets()` failure
from end of file and therefore do not change the error to `ERR_FILEREAD`.
[`od_exit()`](../api/od_exit.md) assigns `ERR_FILEOPEN` when it cannot open a
supported text door-information file for rewriting or cannot reopen a binary
`EXITINFO.BBS` record for updating.

### `ERR_LIMIT`

An established implementation, platform, or representation limit would be
exceeded. Current examples include too many registered personalities, a screen
snapshot whose size cannot be represented, an AVATAR block coordinate above
255, a file specification longer than 99 characters passed to
[`od_list_files()`](../api/od_list_files.md), a FILES.BBS filename token longer
than 79 characters, a FILES.BBS directory component longer than 69 characters,
an assembled FILES.BBS path longer than 99 characters, or formatted output
beyond an internal representable size. The command-line splitter silently
stops at its argument limit and does not set this error.

The limit is not necessarily available system memory. Read the individual
function reference for the applicable maximum. Repeating the same request
without changing it will not resolve this error.

### `ERR_FILEREAD`

This constant is defined as “unable to read from file” for source compatibility.
No current library source path assigns it to [`od_control.od_error`](../control/runtime.md#od_error). In
particular, the display-file functions treat a failed `fgets()` like end of
file. Applications must not currently expect `ERR_FILEREAD` from those APIs.

### `ERR_NOREMOTE`

The requested operation only makes sense for a remote caller, but the current
session is local. The current public functions which assign this value are
[`od_carrier()`](../api/od_carrier.md) and
[`od_set_dtr()`](../api/od_set_dtr.md). Test the session mode before using those
operations.

### `ERR_GENERALFAILURE`

The operation failed but OpenDoors has no more specific public diagnosis. This
value should be reported together with the function, arguments, platform,
transport, and any operating-system diagnostic captured at the failure point.
It is not interchangeable with `ERR_NONE`. In a multithreaded build,
[`od_chat()`](../api/od_chat.md) assigns this error when the chat thread cannot
be started. Activity-log shutdown also assigns it when the final entry cannot
be written or flushed, or when the log stream cannot be closed. An oversized
configurable final-entry template uses the more specific
[`ERR_LIMIT`](#err_limit). [`od_exit()`](../api/od_exit.md) assigns
`ERR_GENERALFAILURE` when output to a supported text door-information file or
binary `EXITINFO.BBS` record fails, or when either file cannot be closed
successfully.

### `ERR_NOTHINGWAITING`

This constant describes a request made when nothing was ready, but no current
library source path assigns it to [`od_control.od_error`](../control/runtime.md#od_error). Nonblocking
[`od_get_input()`](../api/od_get_input.md),
[`od_get_key()`](../api/od_get_key.md), and
[`od_key_pending()`](../api/od_key_pending.md) report an empty queue through
their return values without setting `ERR_NOTHINGWAITING`.

### `ERR_NOMATCH`

This constant denotes “no match was found,” but no current library source path
assigns it to [`od_control.od_error`](../control/runtime.md#od_error). In particular,
[`od_set_personality()`](../api/od_set_personality.md) currently reports a
missing personality as `ERR_LIMIT`, not `ERR_NOMATCH`.

### `ERR_UNSUPPORTED`

The current target or build does not provide the operation. Current public
uses are the personality functions and [`od_set_statusline()`](../api/od_set_statusline.md)
when the DOS/DOS32 text-mode personality system is unavailable. Unlike
`ERR_PARAMETER`, changing an argument within the same unsupported build is not
expected to help.

## Configured process exit levels

[`od_control.od_errorlevel`](../control/customization.md#od_errorlevel) is an
eight-byte table. The constants below are array indexes, not the exit values
themselves. Assign the desired process result to the corresponding element
before initialization.

Element zero enables or disables the table for kernel-forced exits and the
non-Windows normal `atexit` path. The remaining seven elements describe
termination reasons. Initialization failures are an implementation exception:
they call `exit(od_errorlevel[ERRORLEVEL_CRITICAL])` directly without checking
`ERRORLEVEL_ENABLE`. Because the array elements are bytes, configured exit
levels range from 0 through 255 even on systems whose process-status interface
can represent a wider value.

### `ERRORLEVEL_ENABLE`

Index of the enable setting. A nonzero value tells OpenDoors to use the
configured reason-specific values for kernel-forced and normal at-exit paths.
Zero makes those paths use their built-in values. It does not gate the
`ERRORLEVEL_CRITICAL` value used directly by initialization failures. Do not put
the desired “success” exit value in this element.

### `ERRORLEVEL_CRITICAL`

Index of the exit level used when a fatal OpenDoors error prevents the session
from completing initialization. The current initialization paths use this byte
directly whether or not `ERRORLEVEL_ENABLE` is nonzero. A zero-initialized
control structure therefore exits with status zero for such a failure unless
the application sets this element before initialization. This is distinct from
an application choosing a value in a normal call to
[`od_exit()`](../api/od_exit.md).

### `ERRORLEVEL_NOCARRIER`

Index of the exit level used when automatic carrier monitoring detects that the
remote connection has been lost. Socket disconnects and physical modem carrier
loss are normalized to the active transport's connection state.

### `ERRORLEVEL_HANGUP`

Index of the exit level used when the local operator invokes the hangup action.
It distinguishes an intentional operator disconnect from unexpected loss of
carrier.

### `ERRORLEVEL_TIMEOUT`

Index of the exit level used when the caller's available session time expires.
This concerns the BBS/door time limit, not an individual input call returning
after its millisecond timeout.

### `ERRORLEVEL_INACTIVITY`

Index of the exit level used after the configured inactivity limit expires.
The warning interval does not use this level; it is selected only when the
inactivity action terminates the session.

### `ERRORLEVEL_DROPTOBBS`

Index of the exit level used when the local operator requests an immediate
return from the door to the BBS without the normal application path.

### `ERRORLEVEL_NORMAL`

Index of the configured normal-termination level. An explicit `error_level`
argument supplied by the application to [`od_exit()`](../api/od_exit.md) is used
as supplied and does not consult this element. On non-Windows targets, the
OpenDoors `atexit` callback uses this element when custom levels are enabled and
uses `6` otherwise.

## Application exit values

The numerical exit value is part of the contract between a door and its BBS or
launcher. There is no universal meaning for nonzero values: one BBS may treat
an exit code as an event request while another merely records it. Document the
values used by the door and do not repurpose an established value without
considering existing launch scripts.

Call [`od_exit()`](../api/od_exit.md) rather than the C runtime `exit()` for
normal termination. OpenDoors must run its cleanup, drop-file update, callback,
and optional hangup processing before the process status is returned.
