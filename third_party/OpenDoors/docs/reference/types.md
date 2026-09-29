# Types and callbacks

[`OpenDoor.h`](api/index.md) defines the complete application-facing type
system. Include that header rather than copying these declarations: the header
selects the correct Windows, Unix, 16-bit DOS, or 32-bit DOS representation and
applies the calling conventions required by the selected library.

## Fixed-width integer types

### `BYTE`

`BYTE` is an unsigned 8-bit integer. It stores raw bytes, color attributes,
small counters, and values read directly from door-information records. On
Windows it is supplied by the Windows headers; on other targets OpenDoors
defines an equivalent type.

### `INT8`

`INT8` is a signed 8-bit integer. Its range is -128 through 127. Use it when an
external representation specifically requires a signed byte; use `char` for C
text.

### `WORD`

`WORD` is an unsigned 16-bit integer. Its range is 0 through 65,535. Several
established counters, key codes, and record fields use this width because it is
part of the OpenDoors ABI or a BBS file format.

### `INT16`

`INT16` is a signed 16-bit integer with a range of -32,768 through 32,767. It
must not be assumed to have the same width as native `int`: `int` is 16 bits on
the supported 16-bit DOS compilers and normally 32 bits on modern targets.

### `DWORD`

`DWORD` is an unsigned 32-bit integer. Its range is 0 through 4,294,967,295.
It is used for connection speeds, persistent counters, flags whose external
width is fixed, and millisecond intervals.

### `INT32`

`INT32` is a signed 32-bit integer. It is suitable for values whose stored or
binary-interface representation must remain 32 bits across all targets.

### `DWORD_PTR`

`DWORD_PTR` is an unsigned integer type capable of preserving a native pointer
or operating-system handle on supported modern targets. It is 32 bits in a
32-bit build and 64 bits in a 64-bit build. Unlike `DWORD`, its width is not a
portable file format. It is used by
[`od_control.od_open_handle`](control/connection.md#od_open_handle) because a
Windows handle or socket must not be truncated when compiling for 64-bit
Windows.

On older pre-C99 toolchains OpenDoors cannot obtain `uintptr_t`, so the header
uses the best pointer-sized unsigned type available to that compiler. Do not
serialize `DWORD_PTR` or exchange it between processes.

## Native integer and logical types

### `INT`

`INT` is a native signed integer which is at least 16 bits. Windows supplies
the type; OpenDoors defines it elsewhere. Its width follows the target ABI and
is therefore appropriate for counts, coordinates, and return values which are
used only by the running program. It is not appropriate for an unversioned
on-disk record shared between 16-bit and modern builds.

### `UINT`

`UINT` is the unsigned counterpart to `INT` and is also at least 16 bits. The
multiline editor uses it for buffer capacities. Check for narrowing before
converting a `size_t` allocation size to `UINT`.

### `BOOL`

`BOOL` stores a logical value. Use [`TRUE`](constants/general.md#true) and
[`FALSE`](constants/general.md#false), and test a Boolean as zero or nonzero
rather than assuming every true result has the bit pattern `1`. Windows
provides its native `BOOL`; non-Windows targets use an OpenDoors-compatible
definition.

[`MAYBE`](constants/general.md#maybe) is the compatibility value 2. The current
implementation does not test that macro directly. It is not a general Boolean
result; use a field's named constants, such as [`PAGE_USE_HOURS`](constants/session.md#page_use_hours), when the field
has a three-state policy.

### `tODMilliSec`

`tODMilliSec` is the unsigned 32-bit interval type used for millisecond waits.
A value of `1000` represents one second.
[`OD_NO_TIMEOUT`](constants/general.md#od_no_timeout) has the value
`0xffffffff` and requests an indefinite wait only in interfaces which document
that sentinel; it is not a general-purpose duration.

Intervals describe elapsed time, not a wall-clock timestamp. Do not store a
calendar time in `tODMilliSec`, and do not add intervals without considering
32-bit wraparound.

### `tODReserveResult`

```c
typedef enum {
    OD_RESERVE_ERROR = -1,
    OD_RESERVE_PENDING = 0,
    OD_RESERVE_ACQUIRED = 1
} tODReserveResult;
```

`tODReserveResult` reports the state of a named filesystem reservation.
`OD_RESERVE_ACQUIRED` means the caller owns the requested turn,
`OD_RESERVE_PENDING` means the request remains queued, and
`OD_RESERVE_ERROR` means synchronization failed or the call was invalid.
See [`od_reserve_wait()`](api/od_reserve_wait.md).

## Input event types

### `tODInputEventType`

```c
typedef enum {
    EVENT_CHARACTER,
    EVENT_EXTENDED_KEY
} tODInputEventType;
```

`tODInputEventType` identifies the interpretation of a returned input event.
[`EVENT_CHARACTER`](constants/input.md#event_character) describes an ordinary
input byte. [`EVENT_EXTENDED_KEY`](constants/input.md#event_extended_key)
describes one translated function, cursor, navigation, or Shift-Tab key.

### `tODInputEvent`

```c
typedef struct {
    tODInputEventType EventType;
    BOOL bFromRemote;
    char chKeyPress;
} tODInputEvent;
```

[`od_get_input()`](api/od_get_input.md) fills this structure.

#### `EventType`

Read this member first. It determines whether `chKeyPress` is an ordinary byte
or an [`OD_KEY_*`](constants/input.md#extended-key-codes) value. No other event
types are currently defined.

#### `bFromRemote`

`bFromRemote` is `TRUE` when input came from the remote caller and `FALSE` when
it came from the enabled local keyboard. This distinction allows a door to
reject locally entered data or implement operator-only shortcuts without
confusing origin with the key's value.

The field is intentionally false for local control-key input. Local keys which
OpenDoors consumes as operator commands do not become ordinary application
events at all.

#### `chKeyPress`

For [`EVENT_CHARACTER`](constants/input.md#event_character), this is the received character byte. It may be a control
character or have the high bit set; preserve it as an unsigned byte when its
numeric value matters. For [`EVENT_EXTENDED_KEY`](constants/input.md#event_extended_key), cast to `unsigned char` before
comparing with key definitions above 127, such as [`OD_KEY_F11`](constants/input.md#function-keys) and
[`OD_KEY_F12`](constants/input.md#function-keys), on implementations where plain `char` is signed.

The structure contains one event only. Its contents are replaced by the next
successful call.

## Multiline editor types

### `tODEditTextFormat`

This enumeration selects how the multiline editor interprets and stores line
boundaries:

- [`FORMAT_PARAGRAPH_BREAKS`](constants/input.md#format_paragraph_breaks)
  distinguishes paragraph boundaries from visual word wrapping.
- [`FORMAT_LINE_BREAKS`](constants/input.md#format_line_breaks) preserves
  explicit logical lines.
- [`FORMAT_FTSC_MESSAGE`](constants/input.md#format_ftsc_message) uses the line
  conventions expected by FTSC-style message text.
- [`FORMAT_NO_WORDWRAP`](constants/input.md#format_no_wordwrap) leaves wrapping
  under direct user control.

The selected value affects both editing behavior and the final buffer. Choose
it according to the consumer of the edited text, not merely according to how
the edit area should look.

### `tODEditMenuResult`

The optional editor menu callback returns
[`EDIT_MENU_DO_NOTHING`](constants/input.md#edit_menu_do_nothing) to resume
editing or [`EDIT_MENU_EXIT_EDITOR`](constants/input.md#edit_menu_exit_editor)
to finish. These are callback instructions, not the final return value of
[`od_multiline_edit()`](api/od_multiline_edit.md).

### `tODEditOptions`

`tODEditOptions` supplies optional settings and receives final buffer
information for [`od_multiline_edit()`](api/od_multiline_edit.md). Zero the
entire structure before setting selected input members. A null options pointer
uses all defaults; in a supplied structure, a zero coordinate is replaced by
that coordinate's default. [`FORMAT_PARAGRAPH_BREAKS`](constants/input.md#format_paragraph_breaks), null callbacks, and
[`EFLAG_NORMAL`](constants/input.md#eflag_normal) are themselves zero-valued defaults. The two final-buffer
members are outputs.

#### `nAreaLeft`, `nAreaTop`, `nAreaRight`, `nAreaBottom`

These inclusive coordinates define the screen rectangle occupied by the
editor. Their defaults are left 1, top 1, right 80, and bottom 23. OpenDoors
replaces any individual coordinate whose value is zero with its corresponding
default. After that substitution, all coordinates must be positive,
`nAreaRight` must be greater than `nAreaLeft`, and `nAreaBottom` must be greater
than `nAreaTop`. The right and bottom edges must fit within the authoritative
virtual session window, or within the current local output window when no
session window is present. In AVATAR mode they must not exceed 255. Invalid
coordinates cause [`od_multiline_edit()`](api/od_multiline_edit.md) to return
[`OD_MULTIEDIT_ERROR`](constants/input.md#od_multiedit_error) with
[`od_control.od_error`](control/runtime.md#od_error) set to
[`ERR_PARAMETER`](constants/errors.md#err_parameter).

#### `TextFormat`

`TextFormat` is one of the `FORMAT_*` values described above. It controls
logical breaks and wrapping in the resulting text.

#### `pfMenuCallback`

When non-null, this function is invoked for the editor's menu action. Its
argument is always passed as `NULL` by the current implementation and must not
be dereferenced. Escape or Control-Z invokes the callback synchronously. It
returns a `tODEditMenuResult`: [`EDIT_MENU_DO_NOTHING`](constants/input.md#edit_menu_do_nothing) resumes editing and
[`EDIT_MENU_EXIT_EDITOR`](constants/input.md#edit_menu_exit_editor) ends the editor successfully. With no callback, either
key ends the editor successfully.

#### `pfBufferRealloc`

When non-null, this callback allows the editor to request a larger text buffer.
It receives the current buffer and required new capacity. Its contract follows
`realloc()`: preserve existing contents up to the smaller size, return a pointer
to storage of at least the requested size on success, and return null without
invalidating the original allocation on failure.

The allocation and callback must use compatible ownership rules. In a Windows
DLL build, keeping allocation and reallocation in the application avoids
crossing incompatible C runtime heaps.

The application retains ownership of the text buffer. The editor's line index
and redraw workspace are separate internal allocations which OpenDoors
releases before [`od_multiline_edit()`](api/od_multiline_edit.md) returns,
including on an error return.

#### `dwEditFlags`

Set this member to [`EFLAG_NORMAL`](constants/input.md#eflag_normal). No other
public multiline editor flags are defined, and the current implementation does
not read `dwEditFlags`.

#### `pszFinalBuffer`

On success, this points to the buffer containing the final text. It may differ
from the initial pointer when `pfBufferRealloc` grew or moved the allocation.
Always use this returned pointer after editing and release it according to the
application allocator's rules.

#### `unFinalBufferSize`

This member reports the complete capacity associated with `pszFinalBuffer`,
including space for the terminating null byte; it is not the edited string
length. It equals the original capacity if the buffer was not grown, or the
size supplied to the most recent successful `pfBufferRealloc` call.

## Personality and component callbacks

### `OD_COMPONENT`

`OD_COMPONENT` is the platform-correct function type used by the component
selectors in [`od_control`](control/index.md). The values
[`INCLUDE_CONFIG_FILE`](constants/components.md#include_config_file),
[`INCLUDE_LOGFILE`](constants/components.md#include_logfile), and
[`INCLUDE_MPS`](constants/components.md#include_mps) name the corresponding
initializers through this type. Applications normally assign those macros and
do not call component initialization routines themselves.

On segmented DOS targets, the declaration preserves the required far function
pointer. On DOS32 it preserves the selected Open Watcom calling convention.

### `OD_PERSONALITY_PROC`

`OD_PERSONALITY_PROC` is the callback type for a DOS local-interface
personality:

```c
void personality(BYTE operation);
```

The actual declaration includes the far or DOS32 calling convention required
by the platform. `operation` is one of the
[`PEROP_*`](constants/components.md#personality-procedure-operations) values.
Personality modules should include [`ODStat.h`](personality/index.md) as their
SDK header and use this type rather than spelling a compiler-specific function
pointer.

## Control structure type

### `tODControl`

`tODControl` is the type of the exported [`od_control`](control/index.md)
object returned by [`od_control_get()`](api/od_control_get.md). [`OpenDoor.h`](api/index.md)
requests byte alignment for supported Turbo C versions, MSVC, and DOS32, then
restores the compiler's previous packing after the declaration. It does not
apply a universal packed attribute on every target. The resulting layout is
part of the ABI for that specific library build, and many members also contain
native pointers and callbacks; an instance is not portable between compilers,
processes, architectures, or library versions.

Applications may read and write documented members at the documented phase of
the session. A private copy does not replace the exported control object, and
its pointers make a byte-for-byte copy unsuitable as a persistent or
interprocess record. Applications must not assume that unused padding exists
for application data.

Function pointer fields in `tODControl` have the exact signatures shown in
[`OpenDoor.h`](api/index.md). Their calling thread, argument lifetime, and permitted API calls
depend on the individual callback. Those rules are documented with each field;
the structure declaration itself does not impose one universal callback
contract.
