# Session lifecycle

OpenDoors separates settings which must be supplied before startup from state
which is discovered while the door is running. A typical program performs the
following operations:

1. Set program identity and any initialization options in
   [`od_control`](../reference/control/index.md).
2. Parse the standard command line with
   [`od_parse_cmd_line()`](../reference/api/od_parse_cmd_line.md).
3. Call [`od_init()`](../reference/api/od_init.md).
4. Perform input and output through the OpenDoors API.
5. Call [`od_kernel()`](../reference/api/od_kernel.md) during long periods in
   which no other API function is called.
6. Test [`od_carrier()`](../reference/api/od_carrier.md) when the application
   needs explicit connection-state handling.
7. Finish with [`od_exit()`](../reference/api/od_exit.md).

Most API calls initialize OpenDoors automatically. That convenience is useful
for small programs, but it also means that changing initialization settings
after an arbitrary API call may be too late. Set them first and initialize
explicitly.

The thread which performs initialization owns the session. It must also make
all subsequent OpenDoors API calls, access
[`od_control`](../reference/control/index.md), and perform shutdown. See
[Threads and serialized API
access](windows.md#threads-and-serialized-api-access) for the callback and
synchronization rules which apply while the internal Windows UI worker is
active.

[`od_kernel()`](../reference/api/od_kernel.md) services time limits, connection
status, local function keys, and other housekeeping. Blocking OpenDoors input
and timed-wait functions invoke it as needed. A program which spends a long
time computing or has nothing to wait for should call it periodically, using
[`od_sleep()`](../reference/api/od_sleep.md) between calls when it should also
yield processor time.

[`od_exit()`](../reference/api/od_exit.md) performs the library shutdown work,
including connection and door information handling. Do not substitute the C
library's `exit()` where an orderly OpenDoors shutdown is required.

Each loaded OpenDoors instance supports one session. When
[`od_exit()`](../reference/api/od_exit.md) has completed, including when
[`od_noexit`](../reference/control/customization.md#od_noexit) lets the host
process continue, the
library remains terminal and cannot be initialized again. All later function
calls are rejected. The host may still directly inspect
[`od_control`](../reference/control/index.md), but must perform all further work
without calling OpenDoors.
