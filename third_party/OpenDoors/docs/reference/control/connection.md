# Connection and door-information fields

These members identify the door-information source and describe the
communications object used for the current session. Unless stated otherwise,
they begin as zero because [`od_control`](index.md) has static storage.

Settings which influence communications initialization must be assigned
before [`od_init()`](../api/od_init.md), either directly, through
[`od_parse_cmd_line()`](../api/od_parse_cmd_line.md), or through the optional
configuration component. Fields which report the detected format or selected
communications method are valid only after initialization.

## Door-information source

### `info_path`

```c
char od_control.info_path[60];
```

`info_path` specifies where OpenDoors should search for a door-information
file. Its static-storage default is an empty string, which causes the current
directory and the supported BBS environment-variable directories to be
searched.

The value may name a directory or a particular file. A directory is searched
for the supported formats in OpenDoors' normal preference order. A filename
restricts selection to the named file; special handling also recognizes a
node-specific `DORINFO?.DEF` name. The array has room for 59 characters plus
the terminating null byte.

The application may assign this field before initialization. The
[`-D`](../api/od_parse_cmd_line.md#recognized-options) and `-DROPFILE`
command-line options copy their following argument into it, and the
configuration component's `BBSDir` setting can also assign it. Values supplied
by either interface are truncated to fit the array. OpenDoors reads the field
during discovery but does not replace it with the path ultimately selected. It
may therefore remain empty even when a file was found elsewhere.

### `od_info_type`

```c
BYTE od_control.od_info_type;
```

After initialization, `od_info_type` identifies the format from which the
caller and system fields were obtained:

| Value | Source |
| --- | --- |
| [`DORINFO1`](../constants/session.md#dorinfo1) | `DORINFO?.DEF` |
| [`EXITINFO`](../constants/session.md#exitinfo) | Normal `EXITINFO.BBS` |
| [`RA1EXITINFO`](../constants/session.md#ra1exitinfo) | RemoteAccess 1.x extended `EXITINFO.BBS` |
| [`RA2EXITINFO`](../constants/session.md#ra2exitinfo) | RemoteAccess 2.x `EXITINFO.BBS` |
| [`QBBS275EXITINFO`](../constants/session.md#qbbs275exitinfo) | QuickBBS 2.75 extended `EXITINFO.BBS` |
| [`CHAINTXT`](../constants/session.md#chaintxt) | `CHAIN.TXT` |
| [`SFDOORSDAT`](../constants/session.md#sfdoorsdat) | `SFDOORS.DAT` |
| [`CALLINFO`](../constants/session.md#callinfo) | `CALLINFO.BBS` |
| [`DOORSYS_GAP`](../constants/session.md#doorsys_gap) | GAP/PCBoard-style `DOOR.SYS` |
| [`DOORSYS_DRWY`](../constants/session.md#doorsys_drwy) | DoorWay-style `DOOR.SYS` |
| [`DOORSYS_WILDCAT`](../constants/session.md#doorsys_wildcat) | Wildcat-style `DOOR.SYS` |
| [`TRIBBSSYS`](../constants/session.md#tribbssys) | `TRIBBS.SYS` |
| [`DOOR32SYS`](../constants/session.md#door32sys) | `DOOR32.SYS` |
| [`CUSTOM`](../constants/session.md#custom) | Application/configuration-defined record |
| [`NO_DOOR_FILE`](../constants/session.md#no_door_file) | No door-information file is in use |

The uninitialized value is zero, which is numerically equal to [`DORINFO1`](../constants/session.md#dorinfo1) but
does not mean that such a file has been found. The field must not be inspected
as a result until initialization is complete.

OpenDoors writes this member while selecting and parsing the input format and
reads it later for status presentation and for format-specific updates in
[`od_exit()`](../api/od_exit.md). An application normally treats it as
read-only. A custom door-information callback is the exception: it sets
`od_info_type` to [`CUSTOM`](../constants/session.md#custom) and supplies the fields required by the session.
Changing the value after initialization can select the wrong shutdown rewrite
and corrupt an unrelated file.

### `od_extended_info`

```c
BYTE od_control.od_extended_info;
```

This Boolean report is [`TRUE`](../types.md#bool) when OpenDoors successfully
read one of the binary `EXITINFO.BBS` variants represented by [`EXITINFO`](../constants/session.md#exitinfo),
[`RA1EXITINFO`](../constants/session.md#ra1exitinfo), [`RA2EXITINFO`](../constants/session.md#ra2exitinfo), or [`QBBS275EXITINFO`](../constants/session.md#qbbs275exitinfo). It is explicitly reset to
[`FALSE`](../constants/general.md#false) before that detection and remains false for the text formats.

OpenDoors reads the value to decide whether extended caller attributes are
available, whether `EXITINFO.BBS` must be updated at exit, how status
personalities should present several fields, whether file-list pausing follows
the caller's attribute bit, and whether the caller's screen-clearing
preference is authoritative. Applications may inspect it but should not alter
it after initialization.

### `od_ra_info`

```c
BYTE od_control.od_ra_info;
```

This Boolean report is set to [`TRUE`](../constants/general.md#true) only for the RemoteAccess 1.x and 2.x
extended `EXITINFO.BBS` layouts. It is reset to [`FALSE`](../constants/general.md#false) before format
detection. QuickBBS and normal `EXITINFO.BBS` set `od_extended_info` but leave
`od_ra_info` false.

The current OpenDoors implementation writes this member during parsing but
does not otherwise read it. It is provided for the door application to
distinguish RemoteAccess records from the other extended variants.

### `od_node`

```c
WORD od_control.od_node;
```

`od_node` is the one-based BBS node number. Its static-storage value is zero.
During initialization OpenDoors chooses the first available source in this
order:

1. the `TASK` environment variable;
2. the `SBBSNNUM` environment variable;
3. the configuration component's `Node` value;
4. a nonzero value already assigned to `od_control.od_node`, including the
   value assigned by the `-N` or `-NODE` command-line option;
5. the default node number 1.

Some door-information formats subsequently provide their own node number and
replace that value: `SFDOORS.DAT`, GAP/PCBoard and Wildcat `DOOR.SYS`,
`TRIBBS.SYS`, and `DOOR32.SYS`. For `DORINFO?.DEF`, the value is used before
the file is opened: nodes 1 through 9 select the corresponding digit, nodes
10 through 35 use the traditional letter suffix, and higher values initially
fall back to `DORINFO1.DEF`.

The application may assign this field before initialization. Afterward,
OpenDoors reads it for the local status display and writes it back to those
text formats which contain a node field. A door may also inspect it when
selecting node-specific application data.

## Connection speed and port

### `baud`

```c
DWORD od_control.baud;
```

`baud` is the speed OpenDoors supplies to the communications implementation
and the principal legacy indicator that a remote byte stream is active. A
nonzero value assigned before initialization is retained as an explicit speed
override and takes precedence over the speed read from a door-information
file. The [`-B`](../api/od_parse_cmd_line.md#recognized-options) and `-BPS`
options provide the same override.

On DOS and Windows, zero after initialization denotes conventional local mode.
Unix-like builds may use standard input and output as the session transport
even for an explicitly local launch. When the local-login prompt is used,
OpenDoors selects [`COM_STDIO`](../constants/session.md#com_stdio) and records
the terminal's nonzero input speed, or its output speed when no distinct input
speed is available. POSIX `speed_t` values are translated to numeric BPS. If
the terminal attributes or speeds are unavailable, the field receives the
nominal value 19,200. Portable code should use the configured session behavior
rather than assuming that every Unix terminal with nonzero `baud` is attached
to a modem.

Door-information parsers populate this field from their reported port speed.
When a drop file uses `COM0` or another local-mode marker, the result is zero
on DOS and Windows. OpenDoors reads `baud` throughout the library to decide
whether to send remote output, check carrier, clear communications buffers,
and perform disconnect handling. [`od_exit()`](../api/od_exit.md) temporarily
restores the value originally read from the file when rewriting supported
formats.

When [`DIS_BPS_SETTING`](../constants/session.md#dis_bps_setting) is present
in [`od_control.od_disable`](customization.md#od_disable), the value continues
to describe the session but is not applied to the serial hardware.

### `od_connect_speed`

```c
DWORD od_control.od_connect_speed;
```

`od_connect_speed` is the best available modem-to-modem or peer connection
speed, as distinct from a locked computer-to-modem `baud` rate. It begins at
zero. GAP/PCBoard `DOOR.SYS` and `TRIBBS.SYS` can supply a separate value. If
no parser or application has supplied one by the end of initialization,
OpenDoors copies `baud` into this field.

The library reads the value for status lines and for modem-speed simulation in
[`od_disp_emu()`](../api/od_disp_emu.md) and the display-file functions. It is
written back to `TRIBBS.SYS`. Applications may inspect it after
initialization; a custom session may assign it before initialization.

### `port`

```c
INT16 od_control.port;
```

`port` is the zero-based serial-port number: 0 selects `COM1`, 1 selects
`COM2`, and so forth. A value of `-1` is the local/no-serial-port marker used
by several door-information formats. The static-storage default is zero.

Most supported drop files populate the field. The [`-P`](../api/od_parse_cmd_line.md#recognized-options)
and `-PORT` command-line options accept either a zero-based integer or a
`COMn` spelling and mark the result as an explicit override. An application
may also assign a nonzero value before initialization. Because zero is both
the static default and the value for `COM1`, use
[`od_set_port(0)`](../api/od_set_port.md) to make that override explicit. The
same function may be used for every numbered port from 0 through 255. The -1
local-port marker continues to be supplied by supported door-information
formats.

After communications initialization, OpenDoors continues to expose the chosen
number and uses it when rewriting supported text drop files. Changing it does
not move an already open connection to another port.

### `od_open_handle`

```c
DWORD_PTR od_control.od_open_handle;
```

`od_open_handle` supplies a communications object which the BBS or launcher
has already opened. Zero means that no object was supplied. On supported
Windows and Unix-like builds, OpenDoors adopts a nonzero value instead of
opening a numbered serial port itself; it does not close a caller-supplied
object during normal communications shutdown.

The [`-HANDLE`](../api/od_parse_cmd_line.md#recognized-options) option accepts
an unsigned decimal value through the full width of [`DWORD_PTR`](../types.md#dword_ptr).
[`-SOCKET`](../api/od_parse_cmd_line.md#recognized-options) stores a socket
descriptor in the same fashion and also enables `od_use_socket`. Invalid or
out-of-range values leave both fields unchanged and set
[`od_control.od_error`](runtime.md#od_error) to
[`ERR_PARAMETER`](../constants/errors.md#err_parameter). `DOOR.SYS` and
`DOOR32.SYS` can supply a handle or descriptor in their supported extended
forms.

This is an initialization input and later a report of the adopted value. DOS
does not adopt communications objects through this field; its serial and
FOSSIL connections use the numbered-port settings.

### `od_use_socket`

```c
BOOL od_control.od_use_socket;
```

When true, this field requests that the nonzero `od_open_handle` be treated as
a connected socket rather than as a serial or other native handle. Its
default is [`FALSE`](../constants/general.md#false). The `-SOCKET` command-line option sets it; there is no
`-S` alias. Supported `DOOR.SYS` and `DOOR32.SYS` connection descriptions may
also set it during parsing.

OpenDoors reads the value while selecting the preferred communications method
and then records the method actually obtained in `od_com_method`. It should
not be changed after initialization.

## Communications implementation

### `od_com_method`

```c
BYTE od_control.od_com_method;
```

After a remote communications object has been opened, this read-only report
contains the method actually selected:

- [`COM_FOSSIL`](../constants/session.md#com_fossil);
- [`COM_INTERNAL`](../constants/session.md#com_internal);
- [`COM_WIN32`](../constants/session.md#com_win32);
- [`COM_DOOR32`](../constants/session.md#com_door32);
- [`COM_SOCKET`](../constants/session.md#com_socket); or
- [`COM_STDIO`](../constants/session.md#com_stdio).

The initial value is zero, which is not one of the defined method constants.
It may remain zero in a conventional DOS or Windows local-mode session where
no communications object is opened. Applications select behavior through
`od_no_fossil`, `od_use_socket`, `od_open_handle`, and the other initialization
fields; they should not assign `od_com_method` directly. OpenDoors reads the
final value for format-specific shutdown and diagnostics.

### `od_com_flow_control`

```c
BYTE od_control.od_com_flow_control;
```

This initialization setting selects
[`COM_DEFAULT_FLOW`](../constants/session.md#com_default_flow),
[`COM_RTSCTS_FLOW`](../constants/session.md#com_rtscts_flow), or
[`COM_NO_FLOW`](../constants/session.md#com_no_flow). The zero default is
[`COM_DEFAULT_FLOW`](../constants/session.md#com_default_flow), which leaves the communications implementation's default
policy in effect. `TRIBBS.SYS` can explicitly select RTS/CTS or no flow
control.

OpenDoors reads this field immediately before opening a numbered port. Socket,
Door32, standard-I/O, and local sessions do not use UART flow control. Changing
the field after initialization has no effect on the open object.

### `od_no_fossil`

```c
BOOL od_control.od_no_fossil;
```

When true on DOS or DOS32, this initialization setting prevents automatic use
of an installed FOSSIL driver and requests the internal UART implementation.
It defaults to [`FALSE`](../constants/general.md#false) and can be enabled by the `-NOFOSSIL` option or the
configuration file's `NoFossil` keyword.

Other platforms do not select between these DOS implementations. The field is
read only while a numbered communications port is opened.

### `od_com_address`

```c
INT16 od_control.od_com_address;
```

This optional initialization setting supplies the hexadecimal base I/O
address of a direct DOS UART. Zero requests the normal address associated with
the selected port. The `-ADDRESS` command-line option parses hexadecimal, and
the configuration `PortAddress` keyword supplies the same setting. The
configuration parser skips leading non-hexadecimal characters and reads the
first hexadecimal number as a 16-bit value. A setting which contains no
hexadecimal digits leaves the existing value unchanged. An extended
`EXITINFO.BBS` record can also populate it.

OpenDoors applies a nonzero value before opening the internal UART. FOSSIL,
Windows, socket, Door32, and standard-I/O methods do not use it. The library
may write it back to the applicable extended record at exit.

### `od_com_irq`

```c
BYTE od_control.od_com_irq;
```

This optional initialization setting supplies the interrupt request line for
a direct DOS UART. Zero requests the normal IRQ for the selected port. Values
1 through 14 are applied; zero and values 15 or greater are left to the
communications implementation. The `-IRQ` option, `PortIRQ` configuration
keyword, and extended `EXITINFO.BBS` record can populate the field.

It is not used by FOSSIL, Windows, socket, Door32, or standard-I/O methods.
The applicable extended record receives the exposed value when rewritten.

### `od_com_rx_buf`

```c
WORD od_control.od_com_rx_buf;
```

This initialization setting requests the receive-buffer size for a numbered
port. Zero is replaced with 256 immediately before the port is opened. A
nonzero application value or the configuration `ReceiveBuffer` setting is
retained. The resulting value remains available after initialization.

The field does not resize the common OpenDoors input-event queue; that is
controlled by [`od_control.od_in_buf_size`](customization.md#od_in_buf_size).
Its effect depends on the active serial implementation and does not apply to
an already supplied handle.

### `od_com_tx_buf`

```c
WORD od_control.od_com_tx_buf;
```

This initialization setting requests the transmit-buffer size for a numbered
port. Zero is replaced with 3,072 bytes; this is the current implementation's
default, replacing the 1,024-byte value described by older manuals. A nonzero
application value or the configuration `TransmitBuffer` setting is retained.

Its effect depends on the active serial implementation and does not apply to
an already supplied handle. Changing the field after the port is open does
not resize that port's buffer.

### `od_com_fifo_trigger`

```c
BYTE od_control.od_com_fifo_trigger;
```

For a direct DOS UART with its 16550 FIFO enabled, this setting selects a
receive trigger of 1, 4, 8, or 14 bytes. It begins at zero. Any value other
than the four supported choices is replaced with 4 during initialization.
The `FIFOTriggerSize` configuration keyword may assign it.

If `od_com_no_fifo` is true, OpenDoors disables the FIFO and does not replace
the trigger value. FOSSIL, Windows, socket, Door32, and standard-I/O methods
do not use this direct-UART setting.

### `od_com_no_fifo`

```c
BOOL od_control.od_com_no_fifo;
```

When true, this initialization setting disables use of a 16550-compatible FIFO
by the direct DOS UART implementation. It defaults to [`FALSE`](../constants/general.md#false). The `-NOFIFO`
option and `NoFIFO` configuration keyword set it to true.

This field does not disable buffering elsewhere in OpenDoors and has no effect
on FOSSIL, Windows, socket, Door32, or standard-I/O communications.

### `od_cp437_to_utf8_out`

```c
BOOL od_control.od_cp437_to_utf8_out;
```

When true on supported byte-stream methods, OpenDoors converts outgoing CP437
characters to UTF-8 before writing them. It defaults to [`FALSE`](../constants/general.md#false). The
`-CP437UTF8` command-line option enables it. A Unix-like forced-local session
also enables it automatically when the active locale name contains `UTF-8`.

OpenDoors reads the field in its communications send paths. It does not change
the characters stored in screen snapshots or application buffers, and it does
not convert incoming UTF-8 into CP437. The setting must be established before
output begins; changing it in the middle of a session can produce a stream
containing both encodings.

## See also

[Door-information formats](../../guides/door-information-formats.md),
[`od_init()`](../api/od_init.md),
[`od_parse_cmd_line()`](../api/od_parse_cmd_line.md),
[Session constants](../constants/session.md)
