# OpenDoors V7 API Design

Status: Draft

Target: OpenDoors 7.0

Compatibility: Deliberate source and ABI break from OpenDoors V6

## Design Principles

- V7 deliberately breaks source and binary compatibility with V6.
- DOS remains a first-class platform. Serial/modem and socket I/O are core
  features on the platform families where their respective backends apply.
- Mutable library/session state is accessed through functions rather than an
  exposed global control structure. Small caller-owned value structures may be
  public where they naturally represent one operation's data.
- C99 sized types (`int32_t`, `uint8_t`, etc.) are the standard. A
  compatibility header (`od_compat.h`) provides them for C89 compilers
  (Turbo C). Plain `int` is used only for status values, small enumerations,
  and values whose documented range fits a 16-bit signed integer. No API
  assumes that plain `int` is wider than 16 bits.
- Every enumeration constant is representable by a 16-bit signed `int`, as
  required by the supported C implementations. Wider flags or numeric
  constants use explicitly sized integer macros rather than enum members.
- A single process serves a single session. There are no session handles;
  global state is appropriate.
- Initialization is explicit. [`od_init()`](../reference/api/od_init.md)
  returns an error code.
- Niche BBS-system-specific features from V6 are removed. The API surface
  covers what door authors actually need.

## Supported Platforms

V7 supports four platform families:

- 16-bit DOS
- 32-bit DOS
- Windows
- POSIX-like systems

Each platform family is first-class: it is built, tested, documented, and
included in releases. First-class support is a statement of quality and
support, not a promise of identical capabilities. Platform-specific backends
are provided where they are technically appropriate; features on other
platforms are not limited to the capabilities of 16-bit DOS.

## Threading Model

V7 has no application-thread affinity: a public function does not depend on
which application thread calls it. Platform backends do not create private
helper threads. Transport, timer, input, and presentation progress occurs
cooperatively inside public API calls. Application callbacks are invoked
synchronously on the thread executing the call that reaches their event.

Public API calls must not overlap. An application that uses multiple threads
is responsible for serializing all OpenDoors calls; sequential calls may come
from different threads. The library does not add internal locking to make
concurrent entry safe.

Application callbacks may call back into the public OpenDoors API
synchronously. Individual callback contracts may restrict operations that are
invalid in that lifecycle phase or would recursively dispatch the same event;
such restrictions are documented per callback rather than imposed globally.
Synchronous calls made by a callback are nested calls, not overlapping entry
from another application thread.

## Architectural Layers

V7 separates five responsibilities:

1. Session bootstrap and metadata.
2. Transport I/O and carrier state.
3. Character translation and decoded input events.
4. Terminal emulation and local presentation.
5. One-shot lifecycle and application callbacks.

The IPC path intentionally bundles session bootstrap and transport into one
all-or-nothing BBS channel. Raw APIs bypass character decoding or remote
terminal emulation where documented, but do not collapse the layer boundaries.
Character translation remains an OpenDoors responsibility on every path so
IPC, inherited-handle, and DOS sessions expose consistent application
semantics.

## Drop File Format: `opendoor.drp`

A new positional (line-number-based) drop file format. Lines are ordered by
importance so that a minimal BBS can write only the first few lines and a
door that reads fewer lines than the file contains simply uses defaults for
the rest.

No comments. No key=value. Extension is by appending new lines at the end;
old parsers stop at the last line they understand.

### Line Definitions

```
Line 1:  Transport type       0=LOCAL  1=SERIAL  2=SOCKET  3=STDIO
Line 2:  Handle               OS handle or socket descriptor; -1 if N/A
Line 3:  Time remaining       Minutes
Line 4:  User ID              String; opaque, BBS-assigned, persistent
Line 5:  User name            Real name
Line 6:  Screen rows          Terminal height (default 24)
Line 7:  Screen cols          Terminal width (default 80)
Line 8:  ANSI                 0 or 1 (default 0)
Line 9:  Node number          BBS node (default 1)
Line 10: Baud rate            DTE rate; 0 if N/A
Line 11: Encoding             0=CP437  1=UTF-8 (default 0)
Line 12: RIP version          0=none  1=RIPscrip 1.x  2=RIP 2.0
Line 13: Security level       Integer access level
Line 14: Connect speed        DCE rate; 0 if N/A
Line 15: User alias           Handle / nickname
Line 16: User location        City, State or similar
Line 17: BBS name
Line 18: Sysop name
```

### Design Rationale

The format fills three gaps present in every existing drop file format:

- **Screen width (columns)**: only CHAIN.TXT carries this; DOOR.SYS and
  DOOR32.SYS do not.
- **Character encoding**: no existing format specifies whether I/O is
  CP437 or UTF-8.
- **RIP version**: no text-based format distinguishes RIPscrip 1.x from
  RIP 2.0.

The user ID field is a string rather than a numeric record number. Numeric
record numbers can be recycled when users are deleted and recreated. A
string ID lets each BBS use whatever persistent identifier is natural: a
UUID, a database key, or a username that is guaranteed unique.

Handle value -1 means the handle is provided by other means (command-line
argument, stdio inheritance, etc.).

### What This Format Does Not Do

There is no return file. The audit of modern BBS software (Synchronet,
Mystic, ENiGMA 1/2, WWIV, Talisman, x/84) showed that only Synchronet
reads data back from doors in any meaningful way. Standardizing a return
format is not justified.

## Transport Layer

| Transport    | DOS | DOS/32 | Windows | POSIX     |
|--------------|-----|--------|---------|-----------|
| FOSSIL       | Yes | Yes    | --      | --        |
| Direct serial | -- | --     | Yes     | Yes (new) |
| Socket       | --  | --     | Yes     | Yes       |
| Door32 DLL   | --  | --     | Yes     | --        |
| StdIO        | --  | --     | --      | Yes       |
| Local        | Yes | Yes    | Yes     | Yes       |

StdIO is not supported on Windows. Anonymous pipes on Windows do not
support overlapped I/O, and a blocking `ReadFile` on a pipe is not
interrupted when the write end closes, making reliable carrier detection
impossible. POSIX ptys and signal delivery do not have this problem.
Windows doors should use socket transport instead.

Windows local presentation uses the console frontend. The V6 GUI frame is
removed: maintaining two Windows presentation frontends adds support burden
without adding useful door functionality. A console-based frontend also
allows increasing reuse of terminal-facing code between Windows and POSIX as
the Windows terminal environment evolves. Silent mode performs no local
presentation or console management.

The Windows `door32.dll` transport is retained for Wildcat! compatibility.
Its data-available and offline event handles are observed cooperatively during
public API processing; the backend does not create a helper thread.

POSIX serial support consumes an already-open descriptor supplied by the BBS.
OpenDoors does not open the device path or take ownership of serial-port
configuration; opening the same device a second time would conflict with the
BBS that owns the connection. Carrier state may be queried through the
existing descriptor where the platform supports it. Windows direct serial
uses the same inherited-handle ownership model. DOS instead addresses the
resident FOSSIL driver by its logical port number.

The internal UART ISR driver from V6 is removed. DOS serial I/O uses FOSSIL
exclusively. Freely available, mature FOSSIL drivers avoid duplicating
hardware-specific interrupt and UART handling in OpenDoors, while drivers such
as NetFoss permit configurations that are not tied to a physical serial port.

## Drop File Readers

V7 reads four formats plus a custom callback:

- `opendoor.drp` (V7 native)
- `DOOR.SYS` (52-line variant, widely supported)
- `DOOR32.SYS` (socket/handle passing, lean)
- `CHAIN.TXT` (commonly supported and carries the configured screen width)
- Custom callback (`od_on_no_drop_file`) for anything else

All other V6 formats are removed: DORINFO?.DEF, EXITINFO.BBS (all
variants), SFDOORS.DAT, CALLINFO.BBS, TRIBBS.SYS,
PCBOARD.SYS/USERS.SYS. These formats are niche and rarely used; retaining
their readers is not justified by their remaining compatibility value.

## Configuration

Configuration overrides are made through `od_cfg_*()` setter functions before
[`od_init()`](../reference/api/od_init.md). Initialization-only setters may
also be called from `od_on_session_loaded()`, before the resulting state is
frozen, except for bootstrap-only settings that initialization has already
consumed.

### Transport

```
od_cfg_transport(int type)
od_cfg_fossil_port(uint16_t port)
od_cfg_baud_rate(uint32_t baud)
od_cfg_handle(uintptr_t handle)
od_cfg_flow_control(method)
od_cfg_transport_encoding(encoding)
```

`UINTPTR_MAX` is the invalid in-memory handle value and corresponds to `-1` in
the textual drop-file format.

Transport type, FOSSIL port, and reported baud are configured separately. They
are initialization-only, may be supplied before
[`od_init()`](../reference/api/od_init.md) or amended by
`od_on_session_loaded()`, then freeze before the selected transport is opened.
The FOSSIL port occupies the full 16-bit `DX` register; `0x00ff` is reserved by
the FOSSIL specification and does not identify a communications port.
Baud is informational metadata for inherited Windows and POSIX handles;
OpenDoors does not reconfigure the BBS-owned device. On DOS, the configured
rate is supplied to FOSSIL initialization, although a driver may ignore it
(as is common with locked ports).

The inherited handle is also initialization-only and follows the same
ordering. `od_on_session_loaded()` may replace a handle supplied by the drop
file before OpenDoors begins using it.

`od_cfg_handle()` borrows the supplied operating-system handle, descriptor, or
socket; it does not transfer ownership.
[`od_exit()`](../reference/api/od_exit.md) releases OpenDoors'
internal wrapper state but does not call `close()`, `CloseHandle()`, or
`closesocket()` on the borrowed value. The application remains responsible for
closing it when appropriate, while ordinary process teardown retains the
platform's usual responsibility for any handles still open at process exit.

OpenDoors does not automatically alter persistent state on a borrowed object,
including serial timeouts or configuration, termios settings, descriptor
flags, and socket options. Backends use per-call polling or nonblocking
operations where available. A mutation occurs only through an explicit public
request whose contract identifies it; if a backend cannot operate without an
undeclared mutation, initialization fails rather than silently changing the
caller's object.

Flow-control selection, if retained, is initialization-only and freezes before
OpenDoors starts using the serial transport. It never reconfigures an inherited
Windows or POSIX port.

**Open question:** Reconsider whether `od_cfg_flow_control()` belongs in the
public V7 API at all. A door normally should not alter communications policy
owned by the BBS. If retained, inherited Windows/POSIX handles must not be
reconfigured; the remaining possible use is requesting a mode from a DOS
FOSSIL driver during initialization.

### Drop File

```
od_cfg_drop_file(path)
od_cfg_drop_type(type)
```

Drop type constants: `OD_DROP_AUTO`, `OD_DROP_V7`, `OD_DROP_DOORSYS`,
`OD_DROP_DOOR32`, `OD_DROP_CHAIN`, `OD_DROP_CUSTOM`, `OD_DROP_NONE`.

Drop-file path and type are bootstrap-only. Calls to either setter are rejected
once [`od_init()`](../reference/api/od_init.md) has begun, including from
`od_on_session_loaded()`, because
the selected input has already been consumed by that point.

### Program Identity

```
od_cfg_program(name, version)
```

Program identity is initialization-only. Changing it after
[`od_init()`](../reference/api/od_init.md) is rejected; runtime status-line
content is controlled through the status-line API rather than by changing the
door's identity.

### Behaviour

```
od_cfg_local_mode(enable)
od_cfg_inactivity(int warn_sec, int timeout_sec)
od_cfg_max_time(uint16_t minutes)
```

Local mode is initialization-only. It may be set before
[`od_init()`](../reference/api/od_init.md) as a default or amended by
`od_on_session_loaded()`, then is frozen before transport setup begins.

Maximum time is likewise initialization-only: it caps the session during
loading and freezes after `od_on_session_loaded()`. Runtime grants or penalties
change the caller's remaining time through `od_user_set_time_remaining()`.

Inactivity warning and timeout settings remain live. A door may change or
disable them during an active session, and the new policy applies to subsequent
idle processing. Both values are nonnegative seconds no greater than
`INT_MAX`; this range is valid even when `int` is 16 bits.

## Callback Registration

```
od_on_exit(func)
od_on_idle(func)              -- called from od_kernel()
od_on_hangup(func)            -- carrier loss
od_on_time_warning(func)      -- receives minutes_left
od_on_no_drop_file(func)      -- custom drop file or local prompting
od_on_session_loaded(func)    -- validate/amend state after drop-file loading
od_on_statusline(func)        -- custom status line renderer
od_on_chat_start(func)
od_on_chat_end(func)
```

`od_on_exit` appends a handler to an exit-callback list; registering a new
handler does not replace handlers already registered. Registration is
available both before initialization and during an active session. The list
freezes when shutdown begins; later registration attempts return an error, so
exit callbacks cannot mutate the list being dispatched. Handlers run in
reverse registration order, matching `atexit()`-style resource cleanup.
They run before transport and presentation teardown while the session object
still exists. The library does not simulate a healthy transport during this
phase: after carrier loss, remote operations return `OD_ERR_NOCARRIER`, while
local-only operations and stored-value accessors remain available.

`od_on_idle` controls a single callback slot; registering another function
replaces the previous idle callback rather than accumulating handlers. It may
be replaced or cleared during an active session. A change made from inside the
idle callback takes effect on the next idle event.

`od_on_hangup` is also a single live callback slot. It may be replaced or
cleared until shutdown begins, at which point the slot freezes.
Carrier loss invokes the registered handler exactly once on the thread
executing the public call that reaches a cooperative processing point. It
wakes a blocking public call, which returns `OD_ERR_NOCARRIER`. OpenDoors does
not automatically shut down; the application may save its state and then call
[`od_exit()`](../reference/api/od_exit.md) itself.

`od_on_time_warning` follows the same single-slot, live-replacement rule.

`od_on_statusline` is a single live callback slot. Registering `NULL` restores
the built-in status-line renderer.

`od_on_chat_start` is a single live callback slot.
`od_on_chat_end` follows the same rule independently; the two chat hooks do not
have to be registered as a pair.

`od_on_session_loaded` runs after the standard drop-file reader or
`od_on_no_drop_file` callback has populated session state, but before the
transport is opened and initialization-only values are frozen. It may inspect
and amend the resulting configuration. Returning a negative error aborts
[`od_init()`](../reference/api/od_init.md).

`od_on_no_drop_file` is bootstrap-only. It must be registered before
[`od_init()`](../reference/api/od_init.md) and cannot be installed or replaced
after initialization begins. `od_on_session_loaded` follows the same
registration rule because its only invocation point is within
[`od_init()`](../reference/api/od_init.md).

## Initialization

```
int  od_init(void)            -- returns 0 on success, negative on error
int  od_exit(int hangup)
```

[`od_exit()`](../reference/api/od_exit.md) shuts down the OpenDoors session and
returns control to its caller; it never terminates the host process. Process
exit status and application-level cleanup remain the door application's
responsibility. It returns
`OD_ERR_NOT_INITIALIZED` if no session was started and
`OD_ERR_ALREADY_EXITED` if shutdown already completed, in addition to errors
encountered while cleaning up the active session.

The library is process-lifetime one-shot: a process may call
[`od_init()`](../reference/api/od_init.md) only once. A second
[`od_init()`](../reference/api/od_init.md) while a session is active returns
`OD_ERR_ALREADY_INITIALIZED`, and
[`od_init()`](../reference/api/od_init.md) after
[`od_exit()`](../reference/api/od_exit.md) returns `OD_ERR_ALREADY_EXITED`.
V7 provides no reset or session-restart operation.

Calling [`od_init()`](../reference/api/od_init.md) consumes the process's single
initialization attempt even if initialization fails. OpenDoors cleans up any
partially acquired resources and enters a terminal failed state; configuration
and callback registration do not reopen, and another
[`od_init()`](../reference/api/od_init.md) cannot retry with altered data.

All `od_cfg_*()` and `od_on_*()` functions return an integer status. Each
setting documents whether it is initialization-only or may be changed during
an active session. Initialization-only settings reject late changes with an
error; live settings and callbacks are not frozen merely because
[`od_init()`](../reference/api/od_init.md) has completed.

The first call to any `od_cfg_*()` function initializes the complete internal
configuration to its defaults (screen rows 24, cols 80, encoding CP437, drop
type auto, etc.), after which the setter applies its value. If
[`od_init()`](../reference/api/od_init.md) is called without any prior
`od_cfg_*()` call, it performs the same defaults initialization first. This is
only lazy initialization of in-memory configuration values; it performs no
session initialization, I/O, transport setup, or other externally visible
work.

Because the complete default configuration is established before the first
setter applies its value, an application may explicitly set an option to the
same value as its default without needing a separate per-option "specified"
state. [`od_init()`](../reference/api/od_init.md) does not reapply defaults to
an already initialized configuration.

Initialization applies configuration sources in this order:

1. Compiled-in defaults.
2. Pre-init `od_cfg_*()` overrides.
3. Values supplied by the drop-file reader or `od_on_no_drop_file` callback.
4. Amendments made by `od_on_session_loaded()`.

Each layer replaces only the values it supplies. Thus a pre-init override can
provide a default for metadata absent from an older drop-file format, while
`od_on_session_loaded()` is the deliberate place to override a value supplied
by the BBS.

Initialization then reads the selected session source, invokes
`od_on_session_loaded()`, validates and freezes initialization-only state,
opens the transport, and starts local presentation and the status line.

Session initialization is never implicit. Operational APIs do not call
[`od_init()`](../reference/api/od_init.md) on the application's behalf. This
keeps fallible session setup and the freezing of initialization-only options
explicit, rather than allowing an unrelated operation to trigger them as a
call-order-dependent side effect. Calling an operational API before a
successful [`od_init()`](../reference/api/od_init.md) is a contract violation:
the common entry guard invokes `assert()` in assertion-enabled builds. Release
builds perform no operation and report `OD_ERR_NOT_INITIALIZED` through the
function's documented error channel.

Simple stored-value accessors remain expression-friendly and return their
fixed-width value directly. If one is called before successful initialization,
an assertion-enabled build asserts; a release build returns zero (or `NULL`
for a string accessor) and records `OD_ERR_NOT_INITIALIZED` for
`od_last_error()`.

## User Info Accessors

```
const char *od_user_id(void)           -- opaque, persistent
const char *od_user_name(void)
const char *od_user_alias(void)
const char *od_user_location(void)
uint16_t    od_user_security(void)
uint16_t    od_user_time_remaining(void)
uint16_t    od_user_screen_rows(void)
uint16_t    od_user_screen_cols(void)
int         od_user_ansi(void)
int         od_user_rip(void)          -- 0, 1, or 2
```

Corresponding `od_user_set_*()` functions exist for all of the above.
Setters are used by the `od_on_no_drop_file` callback and for runtime
changes (e.g. granting bonus time).
`od_user_set_time_remaining()` accepts a `uint16_t` minute count.
`od_user_set_security()` accepts a `uint16_t` level.
The screen-dimension setters accept `uint16_t` values.

User/session setters modify only OpenDoors' local session state. They do not
rewrite a drop file or send an implicit IPC update back to the BBS.

## System / Session Accessors

```
const char   *od_system_name(void)
const char   *od_sysop_name(void)
uint16_t      od_node_number(void)
int           od_transport_type(void)
int           od_transport_encoding(void) -- OD_ENC_CP437 or OD_ENC_UTF8
uint32_t      od_baud_rate(void)
uint32_t      od_connect_speed(void)
int           od_local_mode(void)
int           od_drop_file_type(void)
```

Encoding is a property of the transport, not of the user. A custom drop-file
callback uses `od_cfg_transport_encoding()` to report it. Like the other
transport configuration, it cannot be changed after initialization-only state
is frozen.

## Terminal Output

```
int od_printf(fmt, ...)       -- formatted output with backtick colour codes
int od_print(str)             -- plain string
int od_putch(int32_t codepoint) -- single Unicode character
int od_write_emulated(buf, len, destinations)
int od_write(buf, len, destinations)

int od_set_color(fg, bg)
int od_set_attrib(attr)       -- IBM-PC attribute byte
int od_set_cursor(uint16_t row, uint16_t col)
int od_get_cursor(uint16_t *row, uint16_t *col)

int od_clr_scr()
int od_clr_line()
int od_draw_box(uint16_t left, uint16_t top,
                uint16_t right, uint16_t bottom)
int od_scroll(uint16_t left, uint16_t top,
              uint16_t right, uint16_t bottom, int distance)

int od_screen_save_size(*size)
int od_screen_save(buf, bufsize)
int od_screen_restore(buf, bufsize)

int od_send_file(filename)    -- display/send ASCII, ANSI, or RIP file
```

All public screen coordinates are zero-based: row 0 and column 0 identify the
upper-left cell. Rectangle APIs use inclusive `left`, `top`, `right`, and
`bottom` edges. Backends convert to one-based ANSI coordinates internally.

The OpenDoors screen state is the expected result of the terminal sequences
the library supports, used for local presentation and screen operations. It is
a best-effort mirror, not an authoritative statement about the remote
terminal; OpenDoors cannot observe terminal-specific behavior at the other end
of the connection.

**Open question:** Define the V7 color and attribute model before assigning
types to attribute parameters. An 8-bit IBM-PC attribute remains useful for
compatibility, but modern terminals may justify richer colors and styles; the
design must decide whether
[`od_set_attrib()`](../reference/api/od_set_attrib.md) is the core
representation or a legacy convenience layered over a broader model.

Text-oriented output arguments are UTF-8. The transport encoding is session
state obtained from the BBS or set by a custom drop-file callback; it is
reported by `od_transport_encoding()`, and OpenDoors performs the required
conversion.

The two counted-buffer functions are byte-oriented escape hatches. They use a
common destination mask:

```
#define OD_OUTPUT_REMOTE 0x01
#define OD_OUTPUT_LOCAL  0x02
```

`od_write_emulated()` treats its input as already encoded for the transport
and passes it through terminal emulation. `od_write()` performs no encoding
conversion and sends remote output byte-for-byte without routing it through
the emulator. When `OD_OUTPUT_LOCAL` is selected for a raw write, the local
copy is nevertheless passed through terminal emulation so it can be rendered
and tracked; this does not alter bytes sent to the remote destination. Either
function may target the remote transport, the local screen, or both. This
avoids the opposite-polarity Boolean arguments used by V6's
[`od_disp()`](../reference/api/od_disp.md) and
[`od_disp_emu()`](../reference/api/od_disp_emu.md).

RIP support remains at its limited V6 scope. OpenDoors obtains or detects the
remote RIP version, emits the RIP clear-screen and default text-window setup,
and lets [`od_send_file()`](../reference/api/od_send_file.md) select and pass
through a `.RIP` stream while using an ANSI or ASCII companion for local
presentation. It does not parse or render RIP graphics locally and does not
provide a general RIP drawing API.

### Colour Syntax

Retained from V6. Backtick-delimited colour names within
[`od_printf()`](../reference/api/od_printf.md):

```
od_printf("`bright green`Welcome, `cyan`%s`white`!\n", od_user_name());
```

## Input

```
int  od_get_input(od_input_t *event, uint32_t timeout_ms)
int  od_get_key(int32_t *key, int wait)
int  od_read(buf, size, bytes_read, timeout_ms) -- raw transport bytes
int  od_get_answer(const char *options)
int  od_input_str(char *buf, int maxlen, int min_ch, int max_ch)
int  od_edit_str(char *buf, char *format, uint16_t row, uint16_t col,
                 int normal_attr, int highlight_attr, int flags)
int  od_key_pending(void)
int  od_clear_keybuffer(void)
```

The input event structure:

```
typedef struct {
    int     type;      -- OD_INPUT_CHAR or OD_INPUT_KEY
    int     remote;    -- from remote user?
    int32_t key;       -- Unicode codepoint or OD_KEY_* code
} od_input_t;
```

`int32_t` for the key value handles the full Unicode range on all
platforms including 16-bit DOS where `int` is only 16 bits.
This caller-owned result structure does not expose mutable library session
state.

`od_read()` is the consuming byte-stream counterpart to raw `od_write()`. It
returns transport bytes exactly as received, before character decoding or key
event interpretation.

Raw and decoded input may be mixed within one session and consume the same
ordered stream. Decoded input commits consumption only when it has formed one
complete input unit: every byte is a boundary in a legacy single-byte
encoding, while UTF-8 synchronizes at complete code-point boundaries. A
recognized terminal key sequence similarly commits as one key event. A
timeout with an incomplete unit leaves its bytes buffered. The decoder does
not consume later units speculatively merely to satisfy one event request.

## Session Control

```
int od_kernel()               -- cooperative processing tick
int od_carrier()              -- 1 alive, 0 disconnected, negative on error
int od_set_dtr(high)          -- DTR control (serial only)
int od_sleep(int ms)          -- 0 yields; positive values sleep, max INT_MAX
int od_chat()                 -- enter sysop chat mode
int od_autodetect(flags)      -- detect ANSI / RIP version
int od_statusline_update()    -- force status line refresh
```

## Popup Menus and Text Windows (require ANSI)

```
od_popup_menu(title, items, uint16_t left, uint16_t top, flags)
od_window_create(uint16_t left, uint16_t top,
                 uint16_t right, uint16_t bottom, title,
                 border_attr, title_attr, inside_attr)
od_window_remove(win)
```

## Command-Line Parser

Built-in parser with long and short options. Standard options:

```
-l, --local              Force local mode
-p, --port NUM           COM port number
-b, --baud NUM           Baud rate
-H, --handle NUM         Pre-opened handle
-d, --drop PATH          Drop file path
-n, --node NUM           Node number
-t, --time NUM           Max time (minutes)
-S, --socket             Handle is a socket
-h, --help               Show help
```

API:

```
int  od_parse_args(argc, argv)    -- parse and apply standard options
int  od_on_arg(func)              -- callback for door-specific arguments
int  od_cfg_help(text)            -- extra help text for --help output
```

The `od_on_arg` callback receives `(argc, argv, *i)` and may advance `*i`
to consume additional arguments. Returns 0 if handled, -1 if unrecognised.

## Error Handling

```
int         od_last_error(void)
const char *od_error_string(err)
```

Every fallible public operation returns an integer status directly. `OD_OK`
means success and negative values are errors. Functions whose primary result
cannot share that return value use an output parameter, as in
`od_screen_save_size()`. Simple stored-value accessors are not otherwise
fallible and return fixed-width values directly; contract violations use the
pre-init behavior described above. No operational function is `void` merely
because its V6 predecessor could report failure only through
[`od_control.od_error`](../reference/control/runtime.md#od_error).

Error codes are negative integers: `OD_OK` (0), `OD_ERR_MEMORY`,
`OD_ERR_NOANSI`, `OD_ERR_PARAM`, `OD_ERR_FILE`, `OD_ERR_NOREMOTE`,
`OD_ERR_GENERAL`, `OD_ERR_NOCARRIER`, `OD_ERR_TIMEOUT`,
`OD_ERR_NOT_INITIALIZED`, `OD_ERR_ALREADY_INITIALIZED`,
`OD_ERR_ALREADY_EXITED`.

## Status Line

The status line occupies one additional local-only row outside the emulated
remote screen; it never reduces or overwrites the caller's reported terminal
dimensions. The built-in renderer places it below the remote screen and shows
user name, time remaining, baud rate, and node number. A custom
`od_on_statusline` renderer may instead place the row above the remote screen,
in which case local presentation offsets the remote screen downward. The
default works without setup.

## Optional Modules

Linked only if used:

Process spawning remains supported on every first-class platform, including
16-bit DOS. The DOS16 implementation must release enough conventional memory
to run useful child programs and then resume the door after the child exits.
V7 retains the existing `ODSwap` implementation, which swaps to EMS when
available and otherwise uses a disk swap file.

```
od_editor(buffer, bufsize, ...)       -- multi-line editor
od_config_load(filename, handler)     -- config file parser
od_reserve_init(shared_path)          -- named reservation setup
od_reserve_acquire(name, timeout_ms)  -- acquire named lock
od_reserve_release()                  -- release named lock
od_log(message)                       -- log file entry
od_spawn(cmdline)                     -- shell out
```

## IPC Protocol

An alternative to drop files and handle passing for modern platforms. The
BBS and door communicate over a local transport (Unix domain socket, TCP
loopback, Windows named pipe) using a simple framing protocol. The BBS
handles the user's actual connection (serial, telnet, SSH) and relays I/O
to the door. The door never touches the raw user transport.

IPC is all-or-nothing. One IPC connection supplies session metadata, control
events, carrier/disconnect state, and user input/output. It cannot be used only
as a metadata source alongside a drop file or separate OpenDoors transport,
nor only as a byte transport alongside separately loaded session metadata.

For an IPC session, this replaces drop files, socket handle passing, and
carrier-detect polling. DOS sessions continue to use FOSSIL plus drop files.

### Framing

Raw I/O data flows in both directions with no framing. Control and
metadata messages are introduced by a NUL byte:

```
Raw I/O:    bytes that are not NUL, passed through as-is
Literal NUL: NUL NUL (escaped)
Message:     NUL + Type(1) + Length(2, big-endian) + Value(Length bytes)
```

NUL never appears in normal terminal I/O (ANSI sequences, text, user
input), so the common path has zero framing overhead. Both sides scan
outbound I/O for NUL bytes and double them.

Type 0x00 is not a valid message type: NUL followed by 0x00 is the
literal NUL escape sequence. The usable type space is 0x01-0xFF.

### Message Types

BBS to door:

| Type | Name           | Value                                 |
|------|----------------|---------------------------------------|
| 0x01 | SESSION        | Tagged values (sub-TLVs, see below)   |
| 0x02 | DISCONNECT     | Empty; user hung up                   |
| 0x03 | TIME_UPDATE    | New time remaining (minutes, 2 bytes) |
| 0x04 | TERM_RESIZE    | Rows (2 bytes) + Cols (2 bytes)       |

Door to BBS:

| Type | Name           | Value                                 |
|------|----------------|---------------------------------------|
| 0x81 | LOG            | Severity (1 byte) + message text      |

Severity values follow syslog levels (0=emergency through 7=debug).

Local changes made through `od_user_set_*()` do not generate protocol
messages; only explicitly defined door-to-BBS message types cross the IPC
boundary.

### Session Message Tagged Values

The SESSION message value is a sequence of sub-TLVs:

```
Tag(1) + Length(2, big-endian) + Value(Length bytes)
```

Tags not recognised by the door are ignored. Tags not sent by the BBS
get default values. This naturally handles missing/optional fields
without the ambiguity of empty lines or the namespace collision risk
of ad-hoc key=value extension.

| Tag  | Name           | Type    | Default |
|------|----------------|---------|---------|
| 0x01 | USER_ID        | string  | (none)  |
| 0x02 | USER_NAME      | string  | (none)  |
| 0x03 | USER_ALIAS     | string  | (none)  |
| 0x04 | USER_LOCATION  | string  | (none)  |
| 0x05 | TIME_LEFT      | uint16  | 0       |
| 0x06 | SCREEN_ROWS    | uint16  | 24      |
| 0x07 | SCREEN_COLS    | uint16  | 80      |
| 0x08 | ANSI           | uint8   | 0       |
| 0x09 | RIP            | uint8   | 0       |
| 0x0A | ENCODING       | uint8   | 0 (CP437) |
| 0x0B | NODE           | uint16  | 1       |
| 0x0C | BAUD           | uint32  | 0       |
| 0x0D | CONNECT_SPEED  | uint32  | 0       |
| 0x0E | SECURITY       | uint16  | 0       |
| 0x0F | BBS_NAME       | string  | (none)  |
| 0x10 | SYSOP_NAME     | string  | (none)  |
| 0x11 | LOG_LEVELS     | uint8   | 0 (bitmask of accepted syslog levels) |

Multi-byte integers are big-endian.

OpenDoors owns and advances the running time-remaining countdown on every
backend. An IPC `TIME_UPDATE` is an explicit authoritative correction and
replaces the current local value, including any earlier local adjustment.

### IPC Bindings

The protocol is transport-agnostic. Defined bindings:

- **Unix domain socket**: BBS listens, door connects. Path passed via
  command-line argument or environment variable.
- **TCP loopback**: BBS listens on localhost, door connects. Port passed
  via command-line argument.
- **Windows named pipe**: BBS creates pipe, door opens it. Pipe name
  passed via command-line argument.
- **Inherited fd (Unix)**: BBS forks door with an already-connected fd.
  Fd number passed via command-line argument.

### Relationship to Drop Files

IPC and the legacy drop-file/transport combination are mutually exclusive
session paths. The door library abstracts over both; the door author's code is
identical either way. `od_user_name()`,
[`od_printf()`](../reference/api/od_printf.md), and
[`od_carrier()`](../reference/api/od_carrier.md) all work the same regardless
of which path is active.

| Platform | Legacy path               | IPC path                      |
|----------|---------------------------|-------------------------------|
| DOS      | FOSSIL + drop file        | --                            |
| Windows  | Socket handle + drop file | Named pipe or TCP loopback    |
| POSIX    | Socket/stdio + drop file  | Unix socket or inherited fd   |

## What Is Removed from V6

- Drop file formats: DORINFO?.DEF, EXITINFO.BBS (QBBS/RA 1.x/RA 2.x/
  QuickBBS 2.75+), SFDOORS.DAT, CALLINFO.BBS, TRIBBS.SYS,
  PCBOARD.SYS/USERS.SYS
- ~90 user fields mapped from dead BBS systems (RA menu stacks, EMSI
  session data, combined records, timelog data, event scheduling, etc.)
- RA/QBBS control code parsing (`^F` / `^K` sequences)
- Avatar terminal emulation
- FILES.BBS listing ([`od_list_files`](../reference/api/od_list_files.md))
- BBS-specific status-display personalities (PCBoard, Remote Access, WildCat);
  the standard local status display is retained
- Registration key system
  ([`od_reg_key`](../reference/control/customization.md#od_reg_key),
  [`od_reg_name`](../reference/control/customization.md#od_reg_name))
- Internal UART ISR driver (replaced by FOSSIL-only on DOS)
- Windows GUI frame window
- ~60 backward-compatibility `#define` aliases
- Implicit session initialization (lazy in-memory configuration defaults are
  retained)
- Return file / drop file write-back

The removed user fields include the caller's birth date. Its only substantive
application use found in the historical-door audit was initializing game state,
principally for features such as a birthday bonus. That narrow use does not
justify exposing ambiguous legacy date formats through the core API; a door
that needs a birth date can collect and retain it in its own user data.

The caller's sex or gender is also omitted. Real-world gender is not session
state that OpenDoors needs to transport, and doors whose gameplay models a
character's gender can store that character property in their own data.

RA/QuickBBS `^F` and `^K` expansion is removed because it is BBS-specific
presentation-file markup, not a terminal or transport facility that belongs in
the core library.

AVATAR terminal emulation is removed. A retained legacy drop format that
reports AVATAR support is treated as ANSI-capable; OpenDoors no longer emits
the dedicated AVATAR command encoding.

[`od_list_files()`](../reference/api/od_list_files.md) and its `FILES.BBS`
parser are removed. The function is an unused, fixed-width listing renderer:
it expands filesystem wildcards and prints names, sizes, descriptions, and
offline markers, but provides no file selection or transfer facility. That
presentation belongs in an application that actually owns a file area.

The commercial registration-name/key system and all registration-dependent
presentation are removed because an open-source library has no registration
state to enforce or display.

V6 compatibility `#define` aliases are removed. V7 exposes one canonical name
for each public facility rather than preserving names for an API and control
structure that it intentionally replaces.

## BBS Software Audit (August 2026)

Audited: Synchronet (local source), ENiGMA 1/2, Mystic, WWIV, Talisman,
x/84, GameSrv.

Formats generated by all modern BBSes: DOOR.SYS and DOOR32.SYS are
universal. CHAIN.TXT is common. Everything else is niche.

Confirmed gaps in all existing formats:

- Screen width (columns): only in CHAIN.TXT
- Character encoding: in no format
- RIP version: only in EXITINFO.BBS (binary)
- DOOR32.SYS has neither rows nor cols

Socket handle passing is platform-fragile: Node.js (ENiGMA 1/2) and
Python (x/84) cannot share socket descriptors, writing placeholders (-1
or 0). Talisman warns against DOOR32.SYS on Linux due to stdio/socket
conflicts.

Command-line specifiers are a parallel channel: Synchronet (%H, %W, %>),
Mystic (%0), WWIV (%H, %Z).

Read-back is rare: only Synchronet does it thoroughly.
