# Runtime state and policy fields

These members report changing session state or select policies which OpenDoors
consults after initialization. Their entries distinguish values which the
application may change from reports which should be treated as read-only.

## Diagnostics and input state

### `od_error`

```c
INT16 od_control.od_error;
```

`od_error` records the reason for the most recent OpenDoors API failure which
assigned an error code. Its static-storage initial value is
[`ERR_NONE`](../constants/errors.md#err_none). Initialization does not
unconditionally reset it, because an initialization helper may already have
reported a failure.

A successful API call normally leaves the previous value unchanged. The
application must therefore test a function's documented return value first and
inspect `od_error` only when that return indicates failure. Reading
`od_error == ERR_NONE` after a successful call does not prove that no earlier
call failed, and a nonzero stale value does not make a later successful call a
failure.

The complete values are listed under [Error codes](../constants/errors.md).
The application may inspect this field and may deliberately clear it to
[`ERR_NONE`](../constants/errors.md#err_none) for its own bookkeeping, but OpenDoors remains free to replace it
on the next diagnosed failure.

During [`od_exit()`](../api/od_exit.md), failure to open a supported text
door-information file for rewriting or a binary `EXITINFO.BBS` record for
updating stores
[`ERR_FILEOPEN`](../constants/errors.md#err_fileopen) here, and an output or
close failure stores
[`ERR_GENERALFAILURE`](../constants/errors.md#err_generalfailure). An activity-log
final-write or close failure also stores the same error; an oversized final-entry
template stores [`ERR_LIMIT`](../constants/errors.md#err_limit). Shutdown
continues and retains the process error level requested by the application.

### `od_last_input`

```c
BYTE od_control.od_last_input;
```

After [`od_get_key()`](../api/od_get_key.md) returns a character, this field is
0 if that event came from the remote caller and 1 if it came from the local
operator. OpenDoors chat and example code use it to select the appropriate
speaker or local-only command behavior.

The static-storage initial value is zero, but before the first returned key
that value does not prove that remote input has occurred. The library writes
the field; application code should normally read it without assigning it.

### `od_user_keyboard_on`

```c
BOOL od_control.od_user_keyboard_on;
```

This field controls whether remote caller keystrokes are admitted to the
common OpenDoors input queue. Initialization sets it to [`TRUE`](../constants/general.md#true). The local
keyboard-off command—Alt-K in the standard personality—toggles it and updates
the status indication. The Win32 local interface provides the equivalent
command.

When the value is [`FALSE`](../constants/general.md#false), the kernel discards newly received remote input;
local operator keys remain available. The application may inspect the state or
change it at runtime when implementing an equivalent policy. It is not a
pre-initialization setting because initialization always enables the caller's
keyboard.

## Display and status state

### `od_cur_attrib`

```c
INT16 od_control.od_cur_attrib;
```

This field is OpenDoors' current logical IBM-PC text attribute. Initialization
sets it to `-1`, meaning that the remote attribute is not yet known.
[`od_set_attrib()`](../api/od_set_attrib.md), terminal emulation, screen
clearing, menus, and chat update it as colors change. Values 0 through 255 have
the representation described under [Colors](../constants/colors.md).

Application code may read the field when it must save and later restore the
current color. It should change the display through
[`od_set_attrib()`](../api/od_set_attrib.md), rather than assigning this member,
because direct assignment sends no terminal command and does not update the
virtual or local screen attribute.

### `od_current_statusline`

```c
BOOL od_control.od_current_statusline;
```

On text-mode targets, this report contains the setting most recently applied
by [`od_set_statusline()`](../api/od_set_statusline.md): 0 through 7 for a
personality status line or [`STATUS_NONE`](../constants/display.md#status_none)
to remove it. The declared type is [`BOOL`](../types.md#bool) for historical ABI compatibility,
but the stored value is not limited to true and false.

Static initialization supplies zero. Normal initialization activates the
personality's initial status line and updates the member when the text-mode
status subsystem is available. It is not an instruction field: assign a new
setting by calling [`od_set_statusline()`](../api/od_set_statusline.md).

### `od_update_status_now`

```c
BOOL od_control.od_update_status_now;
```

Setting this field to [`TRUE`](../constants/general.md#true) requests a forced redraw of the active local
status line. During the next kernel update, OpenDoors redraws the current line
even if its selection has not changed, then resets the field to [`FALSE`](../constants/general.md#false).

Its static and normal default is [`FALSE`](../constants/general.md#false). The application and personality SDK
normally use [`ODStatForceStatusUpdate()`](../personality/ODStatForceStatusUpdate.md)
rather than depending on this structure member directly, but direct runtime
assignment retains its established effect.

### `od_chat_active`

```c
BOOL od_control.od_chat_active;
```

`od_chat_active` is true while OpenDoors' line-oriented sysop chat is active.
It begins as [`FALSE`](../constants/general.md#false); [`od_chat()`](../api/od_chat.md) and the local chat command
set it to true, and chat cleanup restores false.

The chat loop tests this member on every iteration. Application code may set it
to [`FALSE`](../constants/general.md#false) from an appropriate callback or cooperating execution context to
request that chat end. Setting it to [`TRUE`](../constants/general.md#true) does not by itself create the chat
loop, install callbacks, or enter chat mode; use [`od_chat()`](../api/od_chat.md) to begin a
complete chat session.

### `od_silent_mode`

```c
BOOL od_control.od_silent_mode;
```

When assigned [`TRUE`](../constants/general.md#true) before initialization, this field suppresses the local
OpenDoors user interface, including the local presentation and sysop command
handling. It defaults to [`FALSE`](../constants/general.md#false) and can be enabled by the `-SILENT`
command-line option.

On DOS and Windows, conventional local mode has `baud == 0`. Windows preserves
an explicitly enabled silent mode, so a silent local session neither creates
nor manages a console. Traditional non-Windows local initialization forces
this field back to false. Unix-like standard-I/O sessions use different
transport conventions and do not necessarily pass through that zero-baud
case. OpenDoors reads the value throughout local-screen and kernel operation.
Changing it after initialization does not reconstruct resources omitted at
startup and is unsupported.

## Time and paging state

### `od_inactive_warning`

```c
INT16 od_control.od_inactive_warning;
```

This field selects how many seconds before the inactivity deadline OpenDoors
displays [`od_control.od_inactivity_warning`](customization.md#od_inactivity_warning),
or passes that message to
[`od_control.od_time_msg_func`](customization.md#od_time_msg_func). Normal
initialization unconditionally assigns 10 seconds. An application override
must therefore be made after initialization.

The warning is used only while the inactivity limit is nonzero and
[`od_control.od_disable_inactivity`](customization.md#od_disable_inactivity) is
false. OpenDoors emits it at most once between input events; new input resets
the warning state. The field is read dynamically by
[`od_kernel()`](../api/od_kernel.md), so a runtime change affects the next time
check.

### `od_maxtime_deduction`

```c
INT16 od_control.od_maxtime_deduction;
```

This is the number of minutes temporarily removed from
[`od_control.user_timelimit`](caller.md#user_timelimit) because the session's
available BBS time exceeded
[`od_control.od_maxtime`](customization.md#od_maxtime). It begins at zero. If
[`od_maxtime`](customization.md#od_maxtime) is between 1 and 1,440 and is less than [`user_timelimit`](caller.md#user_timelimit),
initialization stores the difference here and reduces [`user_timelimit`](caller.md#user_timelimit) to the
cap.

During [`od_exit()`](../api/od_exit.md), OpenDoors adds the deduction back
before updating the door-information file. The field is maintained by the
library and should be treated as read-only. Altering it changes the time value
returned to the BBS.

### `od_okaytopage`

```c
BOOL od_control.od_okaytopage;
```

This tri-state field controls whether [`od_page()`](../api/od_page.md) may page
the sysop:

| Value | Policy |
| --- | --- |
| [`PAGE_DISABLE`](../constants/session.md#page_disable) | Never page. |
| [`PAGE_ENABLE`](../constants/session.md#page_enable) | Page regardless of configured hours. |
| [`PAGE_USE_HOURS`](../constants/session.md#page_use_hours) | Use `od_pagestartmin` and `od_pageendmin`. |

Although the member is declared [`BOOL`](../types.md#bool), it is not Boolean. Initialization
unconditionally sets [`PAGE_USE_HOURS`](../constants/session.md#page_use_hours), so an application override must be made
after initialization. The local personality may also change it at runtime.

### `od_pagestartmin`

```c
INT16 od_control.od_pagestartmin;
```

This field is the first minute of the daily paging interval, expressed as
minutes after midnight. Initialization first assigns 480 (08:00). If the
configuration file contains paging hours, the configuration component applies
its parsed value after normal OpenDoors initialization. Application code may
change the value afterward.

When `od_okaytopage` is [`PAGE_USE_HOURS`](../constants/session.md#page_use_hours), the beginning is inclusive. Values
should be in the range 0 through 1,439.

### `od_pageendmin`

```c
INT16 od_control.od_pageendmin;
```

This field is the exclusive ending minute of the daily paging interval.
Initialization first assigns 1,320 (22:00), after which configured paging
hours can replace it. Application code may change it at runtime.

If the start is less than the end, the interval lies within one calendar day.
If the start is greater, the interval crosses midnight. Equal start and end
values permit paging for the entire day when [`PAGE_USE_HOURS`](../constants/session.md#page_use_hours) is active.

### `od_page_pausing`

```c
BOOL od_control.od_page_pausing;
```

This field determines whether the display-file and file-listing interfaces
pause after a screenful of output. For ordinary text drop files,
initialization sets it to [`TRUE`](../constants/general.md#true). Extended `EXITINFO.BBS` records replace it
with bit `0x04` of [`user_attribute`](caller.md#user_attribute), the caller's stored pausing preference.
A custom door-information definition may supply it explicitly.

[`od_send_file()`](../api/od_send_file.md),
[`od_send_file_section()`](../api/od_send_file_section.md), and
[`od_list_files()`](../api/od_list_files.md) copy the setting when an operation
begins. The application may change it after initialization; changing it during
an active display does not necessarily change that operation's saved policy.

### `od_page_statusline`

```c
INT16 od_control.od_page_statusline;
```

This field selects the status line displayed while the caller is paging the
sysop. Personality initialization supplies the effective default: the
RemoteAccess personality uses line 5, the PCBoard personality uses line 0,
and the standard, Wildcat, and MPS fallback paths use `-1` to retain the
current status line.

[`od_page()`](../api/od_page.md) switches to the selected line only when the
value is not `-1` and a status line is currently active. It restores the
previous line afterward. The application or a personality may change this
field at runtime.

## Local operator and BBS state

### `sysop_next`

```c
BYTE od_control.sysop_next;
```

This Boolean indicates that the BBS should reserve the system for the sysop
after the current caller leaves. It begins as [`FALSE`](../constants/general.md#false). `SFDOORS.DAT` and
supported extended `EXITINFO.BBS` records can populate it. The standard local
command—Alt-N in the built-in personalities—and the Win32 interface toggle it.

OpenDoors displays the current state and writes it back to formats which
support the setting. The application may inspect or change it at runtime. A
false value for a format which does not provide the field means only that the
feature is not requested; it does not prove that the BBS supports it.

## Compatibility-only state

### `od_last_hot`

```c
INT16 od_control.od_last_hot;
```

This field records the most recent application- or personality-defined local
hot key recognized through [`od_hot_key`](customization.md#od_hot_key). It is
zero before any such key is pressed. On a match, OpenDoors stores the key code,
notifies the active personality with [`PEROP_CUSTOMKEY`](../constants/components.md#perop_customkey), and then invokes the
corresponding [`od_hot_function`](customization.md#od_hot_function), if any.

OpenDoors does not clear the field after dispatch. A personality may clear it
while handling [`PEROP_CUSTOMKEY`](../constants/components.md#perop_customkey); otherwise application code which polls the
field should restore it to zero after processing the key. The value is written
only for custom local hot keys and does not report built-in sysop commands,
ordinary local input, or remote caller input.

### `od_max_key_latency`

```c
tODMilliSec od_control.od_max_key_latency;
```

This member is retained in [`tODControl`](../types.md#todcontrol) for source
and binary compatibility, but the current implementation never reads it. Its
static value is zero and assigning it has no effect. Remote control-sequence
decoding currently uses the internal 250-millisecond
`MAX_CHARACTER_LATENCY` value between bytes.

The timeout passed to [`od_get_input()`](../api/od_get_input.md) limits the
wait for the first event and is independent of this compatibility member.

## See also

[`od_kernel()`](../api/od_kernel.md), [`od_page()`](../api/od_page.md),
[`od_set_statusline()`](../api/od_set_statusline.md),
[Customization fields](customization.md)
