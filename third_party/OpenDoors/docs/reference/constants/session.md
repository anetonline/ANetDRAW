# Session and connection

These constants identify the parsed door-information format, active
communications implementation, flow-control policy, paging policy, disabled
automatic behavior, and BBS event status. Fields which report a selected mode
are normally read after initialization; configuration fields must be set before
initialization unless their individual reference permits a run-time change.

## Door-information formats

[`od_control.od_info_type`](../control/connection.md#od_info_type) is set to one
of the following values after OpenDoors selects and parses a drop file. Test the
constant before using a field available only from a particular format. Do not
assign a value merely to make format-specific caller fields appear valid.

### `DORINFO1`

The selected file is a `DORINFO?.DEF` record in the commonly implemented
RBBS/QuickBBS family. The question mark represents the node number. OpenDoors
uses [`od_node`](../control/connection.md#od_node) and its search rules to find
the appropriate numbered file.

### `EXITINFO`

The selected data is the normal QuickBBS 2.6-era `EXITINFO.BBS` representation,
used with its related `DORINFO?.DEF` information. Only fields actually present
in that representation are meaningful.

### `RA1EXITINFO`

The selected file is the extended RemoteAccess 1.x `EXITINFO.BBS` format.
OpenDoors exposes its additional caller, menu, event, and EMSI data through the
corresponding normalized control fields.

### `CHAINTXT`

The selected file is `CHAIN.TXT`, traditionally produced by WWIV-family
systems. It supplies a different subset and textual representation than
`DOOR.SYS`; check field availability instead of assuming a complete caller
record.

### `SFDOORSDAT`

The selected file is `SFDOORS.DAT`, used by Spitfire and compatible systems.
Node and caller information are normalized where supplied.

### `CALLINFO`

The selected file is `CALLINFO.BBS`, associated with older Wildcat! systems.
This format identifier does not select the Wildcat local personality.

### `DOORSYS_GAP`

The selected file is a GAP/PCBoard-style `DOOR.SYS`. Several BBS packages emit
compatible line-oriented records. OpenDoors parses the known layout rather than
identifying the exact BBS product.

### `DOORSYS_DRWY`

The selected file is the DoorWay-style `DOOR.SYS` representation. Despite the
shared filename, it has different field semantics from the GAP/PCBoard layout.

### `QBBS275EXITINFO`

The selected file is a QuickBBS 2.75-or-later `EXITINFO.BBS`. This constant
distinguishes its extended representation from `EXITINFO` and the RemoteAccess
formats.

### `CUSTOM`

The selected file was parsed using the application's custom door-information
definition configured through the built-in configuration component. The values
available are exactly those mapped by that definition. See
[Custom door-information formats](../../guides/door-information-formats.md).

### `DOORSYS_WILDCAT`

The selected file is a Wildcat! 3.x-style `DOOR.SYS`. The identifier concerns
the drop-file layout and is independent of the active personality or terminal
type.

### `RA2EXITINFO`

The selected file is a RemoteAccess 2.x `EXITINFO.BBS`. Its version-specific
fields are normalized into the current `user_*`, `event_*`, and related
members.

### `TRIBBSSYS`

The selected file is `TRIBBS.SYS`. OpenDoors reads the supported TriBBS session
information from this format.

### `DOOR32SYS`

The selected file is `DOOR32.SYS`, the modern line-oriented handoff which can
describe local operation or an inherited socket/handle session. Confirm
[`od_use_socket`](../control/connection.md#od_use_socket) and
[`od_open_handle`](../control/connection.md#od_open_handle) rather than treating
the connection number as a DOS COM port.

### `NO_DOOR_FILE`

No door-information file was selected. This is the expected result in an
explicit local or application-supplied session, but normally indicates a setup
problem when the BBS was meant to provide caller data. Inspect [`info_path`](../control/connection.md#info_path), the
working directory, command-line options, and `DIS_INFOFILE`.

## Communication methods

[`od_control.od_com_method`](../control/connection.md#od_com_method) selects or
reports the I/O implementation. Availability is platform dependent.

### `COM_FOSSIL`

Use a DOS FOSSIL driver. The driver owns the serial hardware interface and
OpenDoors calls its interrupt API for input, output, carrier, and line setup.
On DOS32, block operations use a DPMI conventional-memory transfer buffer when
possible and fall back to byte operations when necessary; application buffers
are not required to reside below one megabyte.

The FOSSIL driver must already be installed by the BBS or system setup.
Selecting the constant does not load one.

### `COM_INTERNAL`

Use OpenDoors' direct internal UART implementation. This DOS and DOS32 path
requires correct port address, IRQ, buffer, FIFO, and flow-control settings.
On DOS32, OpenDoors installs a protected-mode interrupt handler through the DOS
extender. This is not a synonym for an operating system serial API.

### `COM_WIN32`

Use the Windows serial communications implementation. The active object is a
Windows communications handle. OpenDoors either opens the selected `COMn`
device itself or uses [`od_open_handle`](../control/connection.md#od_open_handle) when the launcher supplies an existing
handle. It closes handles it opens itself; it does not close a caller-supplied
handle when the communications object is closed.

### `COM_DOOR32`

Use the Windows `DOOR32.DLL` interface. OpenDoors loads that DLL, resolves its
initialize, shutdown, read, write, available-event, and offline-event entry
points, and uses those callbacks for communications. This is distinct from the
socket method selected by [`od_use_socket`](../control/connection.md#od_use_socket).

### `COM_SOCKET`

Use a connected socket as the remote byte stream. Carrier state follows socket
connection state. Physical modem settings such as IRQ and FIFO trigger do not
apply. For compatibility with disconnect code written for modems,
[`od_set_dtr(FALSE)`](../api/od_set_dtr.md) closes the socket; raising DTR is
unsupported for this method.

### `COM_STDIO`

Use standard input and standard output as the remote byte stream. This is
useful for launchers which connect a session through pipes or a terminal-like
process interface. The local OpenDoors presentation and the remote standard
streams remain conceptually separate.

## Flow-control methods

[`od_control.od_com_flow_control`](../control/connection.md#od_com_flow_control)
uses one of these values when the active serial implementation supports the
choice. Socket, standard-stream, and local sessions do not use UART flow
control.

### `COM_DEFAULT_FLOW`

Do not override the communication implementation's default flow-control
policy. The DOS FOSSIL and internal-UART implementations choose RTS/CTS as
their default. The Win32 serial implementation leaves the existing DCB
flow-control settings unchanged when the default is selected.

### `COM_RTSCTS_FLOW`

Request RTS/CTS hardware flow control. Both the UART/driver and modem cabling
must support it. This does not enable software XON/XOFF processing.

### `COM_NO_FLOW`

Disable serial flow control. At high speeds, a receiver which cannot keep up
may lose bytes. Use this only when the surrounding serial configuration
requires it.

## Paging policy

[`od_control.od_okaytopage`](../control/runtime.md#od_okaytopage) determines
whether [`od_page()`](../api/od_page.md) may alert the local operator.

### `PAGE_DISABLE`

Paging is unavailable regardless of the configured start and end times. The
door can use this temporarily when the sysop must not be disturbed.

### `PAGE_ENABLE`

Paging is available regardless of configured hours. A caller can still cancel
the page by submitting an empty reason, and the page ends normally if the sysop
does not answer within [`od_page_len`](../control/customization.md#od_page_len) seconds.

### `PAGE_USE_HOURS`

OpenDoors compares the current local time with
[`od_pagestartmin`](../control/runtime.md#od_pagestartmin) and
[`od_pageendmin`](../control/runtime.md#od_pageendmin), expressed as minutes
after midnight. This is the normal tri-state “maybe” policy.

## Disabled automatic behavior

Combine these bits in
[`od_control.od_disable`](../control/customization.md#od_disable). A set bit
turns off one OpenDoors behavior. Preserve unrelated bits when changing the
mask:

```c
od_control.od_disable |= DIS_NAME_PROMPT;
```

### `DIS_INFOFILE`

Do not read or rewrite a door-information file. The application or command line
must supply all connection and caller values needed by the session. This is
stronger than merely setting [`info_path`](../control/connection.md#info_path) to an empty string.

### `DIS_CARRIERDETECT`

Do not automatically terminate when the active transport reports loss of
carrier/connection. [`od_carrier()`](../api/od_carrier.md) can still report the
state, allowing the application to implement its own policy.

Use this carefully: continuing to write after a disconnected socket or modem
does not restore the caller.

### `DIS_TIMEOUT`

Disable automatic termination when the caller's available BBS/door time
expires. It does not disable finite timeouts explicitly passed to input
functions and does not disable inactivity handling.

### `DIS_LOCAL_OVERRIDE`

Prevent a zero BPS value read from door information from overriding an
explicitly configured locked speed and forcing local mode. This is intended for
setups where zero has a format-specific meaning and the application has a more
reliable connection setting.

### `DIS_BPS_SETTING`

Prevent OpenDoors from passing the selected BPS rate to the communications
implementation when it opens the connection. The BBS, driver, or existing
device configuration remains responsible for the rate. Historically this was
used to stop OpenDoors from changing a FOSSIL driver's locked rate; in the
current Win32 serial path it likewise leaves the DCB baud setting unchanged.
This bit does not select a different communication method.

### `DIS_LOCAL_INPUT`

Prevent ordinary local keyboard input from entering the door's common input
queue while leaving recognized local operator function keys available. Remote
input continues normally.

### `DIS_SYSOP_KEYS`

Disable local operator function keys as well as local door input. Use this when
the application or host owns the complete local keyboard. It is broader than
`DIS_LOCAL_INPUT`.

### `DIS_DTR_DISABLE`

On Win32, disable the shutdown procedure which sends
[`od_disable_dtr`](../control/customization.md#od_disable_dtr) to prevent a
modem from responding to a later DTR transition. That procedure is considered
only for a serial port OpenDoors opened itself while carrier remains present.
It is skipped for caller-supplied handles, Door32, sockets, and local sessions.

### `DIS_NAME_PROMPT`

Do not prompt for the caller's name when OpenDoors automatically enters local
mode without user information. The application must tolerate or supply the
remaining default name.

## BBS event status

[`od_control.event_status`](../control/caller.md#event_status) uses these
values when the selected door-information format supplies a pending event.

### `ES_DELETED`

Value 0, the record-format status named “deleted.” The current library copies
this value from compatible `EXITINFO.BBS` records and writes it back. OpenDoors
does not otherwise distinguish it from any other status which is not
`ES_ENABLED`.

### `ES_ENABLED`

Value 1. This is the only status for which OpenDoors treats the accompanying
event information as valid. The RemoteAccess local personality displays the
next-event time and error level when this value is present.

### `ES_DISABLED`

Value 2, the record-format status named “disabled.” Like `ES_DELETED`, it is
copied through supported `EXITINFO.BBS` records. The current library does not
use it for scheduling and treats it only as a value other than `ES_ENABLED`.
