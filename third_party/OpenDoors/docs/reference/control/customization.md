# Customization and callbacks

OpenDoors exposes its built-in configuration, text, colors, local controls, and
lifecycle hooks through [`od_control`](index.md).

Unless an entry says otherwise, pointers begin as `NULL`, numeric and Boolean
fields begin at zero, and arrays begin filled with zero bytes. OpenDoors
assigns its documented defaults during initialization. Settings which affect
initialization must therefore be assigned before [`od_init()`](../api/od_init.md)
or the first function which performs implicit initialization; settings read
later may be changed at the times stated in their entries.

The field descriptions distinguish values supplied by the application from
values which OpenDoors changes itself. Callback pointers and replacement text
remain owned by the application unless an entry explicitly states otherwise.

## Command-line extensions

The command-line extension fields are read only by
[`od_parse_cmd_line()`](../api/od_parse_cmd_line.md). Because that function is
normally called before OpenDoors initialization, these fields must be assigned
before the call.

### `od_cmd_line_flag_handler`

```c
BOOL (*od_control.od_cmd_line_flag_handler)(const char *keyword);
```

This callback receives each command-line argument which is not one of the
built-in OpenDoors options. The argument is passed exactly as it appears in the
argument vector, including any leading `-` or `/`. Return [`TRUE`](../constants/general.md#true) to accept the
argument as a complete flag. Return [`FALSE`](../constants/general.md#false) to let
[`od_cmd_line_handler`](#od_cmd_line_handler) process it instead. No following
arguments are consumed when this callback returns [`TRUE`](../constants/general.md#true).

The pointer is initially `NULL`. OpenDoors calls it synchronously and never
assigns it.

### `od_cmd_line_handler`

```c
void (*od_control.od_cmd_line_handler)(char *keyword, char *options);
```

This compatibility callback handles an unknown command-line keyword not
accepted by [`od_cmd_line_flag_handler`](#od_cmd_line_flag_handler). `keyword`
is the original argument. `options` contains the following non-option
arguments, joined with spaces, up to the next argument beginning with `-` or
`/`, a recognized OpenDoors option, or the end of the command line. Those
arguments are consumed by the parser. The temporary buffer holds 80 bytes;
OpenDoors retains at most the first 79 bytes of the joined text and always adds
a terminating nul. An unknown keyword without following option text produces
an empty string when another command-line option follows. The platform-specific
sources of the argument array are described with
[`od_parse_cmd_line()`](../api/od_parse_cmd_line.md#parameters).

Both pointers refer to temporary parser storage and must not be retained after
the callback returns. The callback is synchronous, its pointer is initially
`NULL`, and OpenDoors never assigns it.

### `od_cmd_line_help_func`

```c
void (*od_control.od_cmd_line_help_func)(void);
```

When the parser encounters `-?`, `-H`, or `-HELP`, it invokes this callback in
place of all built-in command-line help and then terminates the process with
exit status zero. The callback cannot return control to command-line parsing.
The pointer is initially `NULL` and is read only by OpenDoors.

### `od_cmd_line_help`

```c
const char *od_control.od_cmd_line_help;
```

If no [`od_cmd_line_help_func`](#od_cmd_line_help_func) is installed, this
field may point to replacement help text. OpenDoors displays it on standard
output on non-Windows platforms or in a message box on Win32, then terminates
the process. A `NULL` pointer selects the built-in help text. OpenDoors reads
the pointer but does not copy, free, or assign the string.

## General callbacks

### `od_config_function`

```c
void (*od_control.od_config_function)(char *keyword, char *options);
```

This callback extends the optional OpenDoors configuration-file component. It
is invoked for every nonblank configuration line, including lines whose
keywords match entries in [`od_cfg_text`](#od_cfg_text). OpenDoors processes a
recognized built-in setting before invoking the callback, allowing the door to
observe or separately use that setting. The callback should ignore any keyword
it does not handle.

`keyword` contains the uppercased keyword, truncated to 32 characters;
`options` contains the remainder of the line after leading and trailing spaces
and tabs have been removed. Comments begin with a semicolon and are removed
before parsing. When the keyword has no option text, `options` points to a
valid empty string rather than being `NULL`.

The component saves the callback pointer when configuration processing begins,
so it must be assigned before initialization. Both arguments point into
temporary mutable parser buffers and must not be retained. The pointer is
initially `NULL`; OpenDoors reads it but never assigns it.

### `od_config_callback`

```c
void (*od_control.od_config_callback)(void);
```

On Win32, a non-`NULL` pointer retains the application's Config command in the
local window's Door menu. OpenDoors invokes the callback synchronously when the
operator chooses that command. If the pointer is `NULL` when the frame window
is created, OpenDoors removes the menu item; assigning it later does not add
the item again. Other platforms do not read this field. The pointer is
initially `NULL` and is never assigned by OpenDoors.

The callback runs on the Windows frame thread, not on the thread which called
[`od_init()`](../api/od_init.md). It must not call an OpenDoors function or
access [`od_control`](index.md), an OpenDoors global, or a pointer returned by OpenDoors.
It may signal application-owned synchronization or queue work for a serialized
application API caller and must return promptly.

### `od_help_callback`

```c
void (*od_control.od_help_callback)(void);
```

On Win32, a non-`NULL` pointer retains the Contents command in the local
window's Help menu. OpenDoors invokes the callback synchronously when the
operator chooses that command. If the pointer is `NULL` when the frame window
is created, OpenDoors removes the menu item; assigning it later does not add
the item again. Other platforms do not read this field. The pointer is
initially `NULL` and is never assigned by OpenDoors.

The callback runs on the Windows frame thread and is subject to the same
restriction as [`od_config_callback`](#od_config_callback): it must not access
the OpenDoors API or ABI. Queue any OpenDoors work for a serialized application
API caller.

### `od_ker_exec`

```c
void (*od_control.od_ker_exec)(void);
```

OpenDoors invokes this callback near the beginning of every non-recursive
[`od_kernel()`](../api/od_kernel.md) execution. It provides a regular service
point for application work in both single-threaded and multithreaded builds.
The callback executes synchronously while the kernel is marked active; a
recursive [`od_kernel()`](../api/od_kernel.md) call returns without invoking it again. The pointer is
initially `NULL` and is read only by OpenDoors.

### `od_local_input`

```c
void (*od_control.od_local_input)(INT16 key);
```

This callback receives a local operator keystroke which was not consumed by a
built-in command or an entry in [`od_hot_key`](#od_hot_key). The key uses the
IBM scan-code/ASCII representation described under [function
keys](#function-keys). OpenDoors calls the function before placing that same
key in the common local/remote input queue. Up and Down Arrow are also passed
to it while an API operation has reserved those keys for ordinary input.

Remote input never invokes this callback. The pointer is initially `NULL` and
is read only by OpenDoors.

### `od_no_file_func`

```c
void (*od_control.od_no_file_func)(void);
```

OpenDoors invokes this callback when it cannot read any supported
door-information file. Before the call,
[`od_info_type`](connection.md#od_info_type) is [`NO_DOOR_FILE`](../constants/session.md#no_door_file). The callback
may provide a local-login path by setting [`od_force_local`](#od_force_local)
to [`TRUE`](../constants/general.md#true), or may read another
format, populate the required control fields, and set [`od_info_type`](connection.md#od_info_type) to
[`CUSTOM`](../constants/session.md#custom). If neither action supplies sufficient startup information,
initialization reports that no door-information file could be read and exits.

The callback executes synchronously on the thread making the active API call during
[`od_kernel()`](../api/od_kernel.md) and may call other OpenDoors functions.
Its pointer is initially `NULL` and is read only by OpenDoors.

### `od_time_msg_func`

```c
void (*od_control.od_time_msg_func)(char *message);
```

This callback replaces OpenDoors' normal display of four time-related
messages: the inactivity warning, inactivity timeout, warning at three, two,
or one minute remaining, and expiration of the caller's session time. The
argument is the exact string OpenDoors would otherwise pass to
[`od_disp_str()`](../api/od_disp_str.md); the remaining-time warning has already
been formatted with the number of minutes. The storage may be a prompt string
or an internal work buffer and must not be modified or retained.

The callback changes only how the message is delivered. OpenDoors still
performs the warning bookkeeping and timeout shutdown. The pointer is initially
`NULL` and is read only by OpenDoors. Timer processing invokes this callback on
the thread making the active API call. The callback may call other OpenDoors
functions.

## Program, component, and lifecycle settings

### `od_app_icon`

```c
HICON od_control.od_app_icon;
```

On Win32, a non-`NULL` value supplies the icon for the OpenDoors frame window
class and therefore for the local window and taskbar representation. Assign a
valid application-owned `HICON`, such as one returned by the Windows
`LoadIcon()` function, before the frame window is created. OpenDoors reads the
handle but does not destroy or replace it. The field begins as `NULL`, which
selects the library's default icon. Non-Windows platforms do not contain this
member in their public structure layout.

### `od_box_chars`

```c
char od_control.od_box_chars[8];
```

These eight CP437 bytes draw the border used by
[`od_draw_box()`](../api/od_draw_box.md), popup windows, and menus:

| Index | Border position |
| --- | --- |
| [`BOX_UPPERLEFT`](../constants/display.md#box_upperleft) | Upper-left corner |
| [`BOX_TOP`](../constants/display.md#box_top) | Top edge |
| [`BOX_UPPERRIGHT`](../constants/display.md#box_upperright) | Upper-right corner |
| [`BOX_LEFT`](../constants/display.md#box_left) | Left edge |
| [`BOX_LOWERLEFT`](../constants/display.md#box_lowerleft) | Lower-left corner |
| [`BOX_LOWERRIGHT`](../constants/display.md#box_lowerright) | Lower-right corner |
| [`BOX_BOTTOM`](../constants/display.md#box_bottom) | Bottom edge |
| [`BOX_RIGHT`](../constants/display.md#box_right) | Right edge |

During initialization OpenDoors assigns CP437 single-line characters 218,
196, 191, 179, 192, and 217 to the first six elements. The bottom and right
elements remain zero until a drawing function first needs them; that function
then copies the top and left elements respectively. An application may replace
the array after initialization. Drawing functions read it and may perform that
zero-value completion, so the array is not strictly read-only to the library.

### `od_before_exit`

```c
void (*od_control.od_before_exit)(void);
```

This callback is invoked during [`od_exit()`](../api/od_exit.md), after the
maximum-door-time deduction has been restored and elapsed time has been
accounted for, but before OpenDoors displays its shutdown message, closes the
connection, or rewrites the door-information file. It can therefore perform
application cleanup and make final changes to fields which must be written
back to the BBS. The pointer is initially `NULL`; OpenDoors reads it but never
assigns it.

### `od_cafter_chat`

```c
void (*od_control.od_cafter_chat)(void);
```

This callback is invoked while chat cleanup is in progress, after
[`od_after_chat`](#od_after_chat) has been displayed, but before the original
display attribute is restored and before
[`od_chat_active`](runtime.md#od_chat_active) is reset to [`FALSE`](../constants/general.md#false). It can
restore screen contents saved by [`od_cbefore_chat`](#od_cbefore_chat). The
pointer is initially `NULL` and is read only by OpenDoors.

### `od_cafter_shell`

```c
void (*od_control.od_cafter_shell)(void);
```

This callback is invoked after the command interpreter returns and after
OpenDoors displays [`od_after_shell`](#od_after_shell). It can restore screen
state saved by [`od_cbefore_shell`](#od_cbefore_shell). The pointer is initially
`NULL` and is read only by OpenDoors.

### `od_cbefore_chat`

```c
void (*od_control.od_cbefore_chat)(void);
```

This callback is invoked after OpenDoors has set
[`od_chat_active`](runtime.md#od_chat_active) to [`TRUE`](../constants/general.md#true), but before it displays
[`od_before_chat`](#od_before_chat) or enters its chat input loop. It is
commonly used to save or rearrange the application's screen. The callback may
set [`od_chat_active`](runtime.md#od_chat_active) to [`FALSE`](../constants/general.md#false) to suppress the built-in chat loop; OpenDoors
then proceeds through its normal chat cleanup. The pointer is initially `NULL`
and is read only by OpenDoors.

### `od_cbefore_shell`

```c
void (*od_control.od_cbefore_shell)(void);
```

This callback is invoked immediately before OpenDoors displays
[`od_before_shell`](#od_before_shell) and runs the local command interpreter.
It can save screen state or prepare application resources for the shell. The
pointer is initially `NULL` and is read only by OpenDoors.

The chat and shell callbacks execute synchronously inside OpenDoors. A chat or
shell operation requested recursively from one of these callbacks is deferred
or suppressed until the current operation has completed.

### `od_cfg_lines`

```c
char od_control.od_cfg_lines[25][33];
```

This array contains the 25 keywords used in a custom door-information file
definition. Each element has room for a keyword of up to 32 characters plus
its terminating null byte. Matching is case-insensitive. The effective
defaults, in array order, are:

```text
Ignore           ComPort          FossilPort       ModemBPS
LocalMode        UserName         UserFirstName    UserLastName
Alias            HoursLeft        MinutesLeft      SecondsLeft
ANSI             AVATAR           PagePausing      ScreenLength
ScreenClearing   Security         City             Node
SysopName        SysopFirstName   SysopLastName    SystemName
RIP
```

OpenDoors supplies a default only when an element's first byte is zero, then
uppercases all elements when the configuration component begins reading a
file. An application which changes these keywords must do so before
initialization. OpenDoors reads them while interpreting `CustomFileLine`
configuration entries; it does not modify them after the initial defaulting
and uppercasing step.

### `od_cfg_text`

```c
char od_control.od_cfg_text[49][33];
```

This array contains the 49 built-in keywords recognized by the optional
configuration-file component. Each element has room for a keyword of up to 32
characters plus its terminating null byte. Matching is case-insensitive. The
effective defaults, in array order, are:

```text
Node                       BBSDir                     DoorDir
LogFileName                DisableLogging             SundayPagingHours
MondayPagingHours          TuesdayPagingHours         WednesdayPagingHours
ThursdayPagingHours        FridayPagingHours          SaturdayPagingHours
MaximumDoorTime            SysopName                  SystemName
SwappingDisable            SwappingDir                SwappingNoEMS
LockedBPS                  SerialPort                 CustomFileName
CustomFileLine             InactivityTimeout          PageDuration
ChatUserColour             ChatSysopColour            FileListTitleColour
FileListNameColour         FileListSizeColour         FileListDescriptionColour
FileListOfflineColour      Personality                NoFossil
PortAddress                PortIRQ                    ReceiveBuffer
TransmitBuffer             PagePromptColour           LocalMode
PopupMenuTitleColour       PopupMenuBorderColour      PopupMenuTextColour
PopupMenuKeyColour         PopupMenuHighlightColour   PopupMenuHighKeyColour
NoFIFO                     FIFOTriggerSize             DisableDTR
NoDTRDisable
```

The spelling shown above is the spelling installed by the library; matching is
case-insensitive but does not otherwise normalize alternate spellings.
OpenDoors supplies a default only when an element's first byte is zero, then
uppercases the complete array when the configuration component begins reading
a file. An application which replaces a keyword must do so before
initialization. OpenDoors subsequently reads these strings to identify built-in
options and does not restore their original case.

### `od_clear_on_exit`

```c
BOOL od_control.od_clear_on_exit;
```

Initialization sets this Boolean to [`TRUE`](../constants/general.md#true).
When it remains true, OpenDoors clears its output screen during ordinary
shutdown and before running a local shell or spawned program. Set it to
[`FALSE`](../constants/general.md#false) after initialization to leave the
existing display in place. OpenDoors reads the field at each of those
transitions and does not otherwise change it.

Leaving the display intact can produce a smoother handoff when the BBS
immediately redraws the same terminal, while clearing is more suitable before
a batch file or unrelated console program becomes visible. The setting does
not determine whether an individual call to
[`od_clr_scr()`](../api/od_clr_scr.md) is honored.

### `od_color_delimiter`

```c
char od_control.od_color_delimiter;
```

Initialization sets this field to the backquote character. It delimits named
color descriptions embedded in [`od_printf()`](../api/od_printf.md) format
strings. An application may select another byte when backquotes must be
displayed literally, or set the field to zero to disable named embedded color
descriptions. OpenDoors reads the current value while formatting each string;
it does not change the value after initialization.

### `od_color_names`

```c
char od_control.od_color_names[12][33];
```

These are the twelve names recognized in embedded
[`od_printf()`](../api/od_printf.md) color descriptions and by the optional
configuration parser. The effective defaults are:

| Index | Name | Index | Name |
| ---: | --- | ---: | --- |
| 0 | `BLACK` | 6 | `YELLOW` |
| 1 | `BLUE` | 7 | `WHITE` |
| 2 | `GREEN` | 8 | `BROWN` |
| 3 | `CYAN` | 9 | `GREY` |
| 4 | `RED` | 10 | `BRIGHT` |
| 5 | `MAGENTA` | 11 | `FLASHING` |

Each element can hold 32 bytes plus its terminating null. An application may
replace names before initialization; the configuration component uppercases
the strings when it starts, and matching is case-insensitive in formatted
output. OpenDoors reads the array thereafter. The array changes the vocabulary
used to describe colors, not the numeric IBM-PC attribute values themselves.

### `od_config_file`

```c
OD_COMPONENT *od_control.od_config_file;
```

The pointer begins `NULL`, which leaves the optional configuration component
disabled. Assign
[`INCLUDE_CONFIG_FILE`](../constants/components.md#include_config_file) before
initialization to select the built-in component; assigning
[`NO_CONFIG_FILE`](../constants/components.md#no_config_file) has the same
disabled effect as `NULL`. OpenDoors reads and invokes the selected component
during initialization and does not assign this field. See
[Configuration files](../../guides/configuration.md).

### `od_config_filename`

```c
const char *od_control.od_config_filename;
```

This pointer begins `NULL`, selecting the lowercase filename `door.cfg`. When
the configuration component begins processing a `NULL` pointer, OpenDoors
assigns it to an internal static string containing that default name. The
application must not modify or free that string.

A non-`NULL` value supplies the path to read instead. Assign it before
initialization and keep the string valid until configuration processing has
completed. OpenDoors first attempts the supplied path exactly. If that fails
and the value includes a directory separator or drive, it retries the basename
from the current directory, provided the entire basename fits in its 257-byte
parsing buffer. It does not truncate an oversized basename for this fallback.
An explicit empty string is not equivalent to `NULL`; it names a required file
which cannot be opened and therefore produces the normal
missing-configuration-file error. OpenDoors does not copy, modify, free, or
replace a non-`NULL` value supplied by the application.

### `od_default_personality`

```c
void (*od_control.od_default_personality)(BYTE operation);
```

On DOS, DOS32, and Windows console builds, this field selects the status-line personality used when the
multiple-personality component is absent or does not select a requested
personality. It may point to an application-supplied personality procedure or
to one of the built-in procedures named by [`PER_OPENDOORS`](../constants/components.md#per_opendoors), [`PER_OD_ONEROW`](../constants/components.md#per_od_onerow), [`PER_PCBOARD`](../constants/components.md#per_pcboard),
[`PER_RA`](../constants/components.md#per_ra), and [`PER_WILDCAT`](../constants/components.md#per_wildcat).

The pointer is initially `NULL`, which selects the standard OpenDoors
personality. OpenDoors reads it during initialization and then calls the
selected procedure with [`PEROP_INITIALIZE`](../constants/components.md#perop_initialize); the personality supplies function
key mappings, status-line behavior, and its paging status-line default. Assign
the pointer before the first OpenDoors API call. The personality system is not
implemented on the other platforms, where this field is retained for
structure compatibility but is not read.

### `od_default_rip_win`

```c
BOOL od_control.od_default_rip_win;
```

This Boolean begins [`FALSE`](../constants/general.md#false). When false,
clearing an active RIP display sends the normal RIP reset followed by
`!|w0000270M12`, selecting the 23-line text window used by doors which also
support non-RIP terminals. When true, OpenDoors omits that second command and
leaves the RIP terminal's default 43-line text window active. The field is read
when the screen is cleared for a caller whose RIP mode is active; OpenDoors
never assigns it.

The setting controls commands sent to a RIP terminal. It does not enlarge the
local console, change a plain ANSI caller's screen length, or alter the declared
capacity of a screen buffer.

### `od_disable`

```c
WORD od_control.od_disable;
```

This field begins at zero. Each set bit disables one normally enabled feature;
clear bits retain normal behavior:

| Mask | Effect |
| --- | --- |
| [`DIS_INFOFILE`](../constants/session.md#dis_infofile) | Do not search for, read, or rewrite a door-information file. Set this before initialization and provide the caller, connection, and session values which the application requires. This is suitable for an application which supplies an unsupported format itself or is not operating as a conventional BBS door. |
| [`DIS_CARRIERDETECT`](../constants/session.md#dis_carrierdetect) | Do not initiate shutdown when carrier is absent. The application may inspect the state with [`od_carrier()`](../api/od_carrier.md). This bit is read while the kernel and input paths monitor the connection, so it may be changed during a session. |
| [`DIS_TIMEOUT`](../constants/session.md#dis_timeout) | Do not initiate shutdown when the caller's session time expires. It does not disable inactivity handling; set [`od_inactivity`](#od_inactivity) to zero for that separate policy. The kernel reads this bit during the session. |
| [`DIS_LOCAL_OVERRIDE`](../constants/session.md#dis_local_override) | Permit a configured locked BPS rate to replace a zero rate obtained from the drop file. Ordinarily zero selects local mode and takes precedence over the configured rate. OpenDoors reads this bit while selecting the initial communications rate. |
| [`DIS_BPS_SETTING`](../constants/session.md#dis_bps_setting) | Do not ask a FOSSIL driver to change its port rate to the BBS-supplied rate. This bit is read during communications initialization. |
| [`DIS_LOCAL_INPUT`](../constants/session.md#dis_local_input) | Do not place ordinary local-keyboard input into the caller input queue. Local sysop command keys remain active. The kernel reads this bit while processing local input. |
| [`DIS_SYSOP_KEYS`](../constants/session.md#dis_sysop_keys) | Disable both local sysop command keys and ordinary local-keyboard input. The kernel reads it during the session. |
| [`DIS_DTR_DISABLE`](../constants/session.md#dis_dtr_disable) | Do not send the configured DTR-response-disabling modem command when closing a Win32 serial port which OpenDoors opened itself. It has no effect on a borrowed handle or a non-Windows connection. |
| [`DIS_NAME_PROMPT`](../constants/session.md#dis_name_prompt) | Suppress the local name prompt when forced local mode has no caller name. OpenDoors reads it during initialization. |

OpenDoors reads this bit mask but does not assign it. The field does not
control the local status subsystem; use [`od_status_on`](#od_status_on) for
that purpose. See [bit-mapped flags](../glossary.md#bit-mapped-flags) for the
operations used to set and clear individual masks.

### `od_disable_dtr`

```c
char od_control.od_disable_dtr[40];
```

If the array is empty at initialization, OpenDoors assigns
`"~+++~  AT&D0|  ATO|"`. On Win32, when OpenDoors itself opened the serial
port, shutdown sends this script before closing the port unless
[`DIS_DTR_DISABLE`](../constants/session.md#dis_dtr_disable) is set.

Within the script, `~` requests a one-second delay and `|` sends a carriage
return. A single space separates a command from the response it requires; two
spaces separate commands when no response is required. The default therefore
waits around the modem escape sequence, requests `AT&D0`, and returns the modem
online with `ATO`. An application or configuration file may replace the
script. OpenDoors reads it only on the applicable owned Win32 serial-port
shutdown path; socket, local, borrowed-handle, and non-Windows sessions do not
use it.

### `od_emu_simulate_modem`

```c
BOOL od_control.od_emu_simulate_modem;
```

This Boolean begins [`FALSE`](../constants/general.md#false), allowing terminal
emulation and display-file output to run without artificial delay. When true,
[`od_disp_emu()`](../api/od_disp_emu.md),
[`od_send_file()`](../api/od_send_file.md), and
[`od_hotkey_menu()`](../api/od_hotkey_menu.md) pace emulated output according
to the current connection speed. A local session uses 9,600 BPS for this
calculation.

The option is intended for locally previewing timing-dependent display files
and animations. It does not limit raw connection throughput, and OpenDoors
never assigns it.

### `od_errorlevel`

```c
BYTE od_control.od_errorlevel[8];
```

The array begins filled with zero bytes. A nonzero
[`ERRORLEVEL_ENABLE`](../constants/errors.md#errorlevel_enable) element tells
OpenDoors to use the remaining elements when it initiates shutdown:

| Index | Shutdown reason |
| --- | --- |
| [`ERRORLEVEL_CRITICAL`](../constants/errors.md#errorlevel_critical) | Fatal initialization or internal error |
| [`ERRORLEVEL_NOCARRIER`](../constants/errors.md#errorlevel_nocarrier) | Carrier or connection lost |
| [`ERRORLEVEL_HANGUP`](../constants/errors.md#errorlevel_hangup) | Sysop manually terminated the call |
| [`ERRORLEVEL_TIMEOUT`](../constants/errors.md#errorlevel_timeout) | Caller's session time expired |
| [`ERRORLEVEL_INACTIVITY`](../constants/errors.md#errorlevel_inactivity) | Caller exceeded the inactivity limit |
| [`ERRORLEVEL_DROPTOBBS`](../constants/errors.md#errorlevel_droptobbs) | Sysop returned the caller to the BBS |
| [`ERRORLEVEL_NORMAL`](../constants/errors.md#errorlevel_normal) | Normal library-initiated shutdown |

When the enable element is zero, the established built-in exit values are
used. The array does not override the explicit error level passed by the
application to [`od_exit()`](../api/od_exit.md), nor a status returned directly
from `main`. OpenDoors reads the array but does not assign it.

### `od_force_local`

```c
BOOL od_control.od_force_local;
```

This Boolean begins [`FALSE`](../constants/general.md#false). Set it to
[`TRUE`](../constants/general.md#true) before initialization to request local
operation without reading a door-information file. The standard
[`od_parse_cmd_line()`](../api/od_parse_cmd_line.md) parser sets it for the
`-LOCAL` option.

Forced local initialization enables ANSI, supplies a 60-minute limit when no
limit has been provided, uses the BBS name as the caller's location or
`"Unknown Location"` when the BBS name is empty, and selects the sysop name as
the default caller name. If the caller name is still absent and
[`DIS_NAME_PROMPT`](../constants/session.md#dis_name_prompt) is clear,
supported local interfaces obtain a local identity. The Unix interface uses
the current account's login and GECOS names when available and otherwise
retains the defaults already selected; other local interfaces prompt for a
name. The final selected values are stored in the ordinary caller and
connection fields.

OpenDoors reads and may set this field while processing launch options and
initialization. Changing it after initialization does not convert an active
remote connection into a local session.

### `od_in_buf_size`

```c
WORD od_control.od_in_buf_size;
```

This selects the number of ring entries allocated for OpenDoors' combined
local and remote inbound queue. The ring reserves one entry to distinguish
the full and empty states, so the usable capacity is one less than this value.
A zero value selects 256 ring entries and a usable capacity of 255 events.
The internal event size varies by target and is not part of this setting's
contract. Assign the field before initialization; OpenDoors reads it while
allocating the queue and does not update it to report the effective capacity.

This is distinct from the communications receive buffer selected by
[`od_com_rx_buf`](connection.md#od_com_rx_buf). That lower-level buffer holds
only bytes received from the communications driver, whereas this queue merges
accepted remote and local input for API functions. Increasing it can tolerate
longer bursts between reads but does not repair an undersized UART, FOSSIL, or
socket receive buffer.

### `od_inactivity`

```c
INT16 od_control.od_inactivity;
```

Initialization sets this limit to 200 seconds. When no caller input has been
received for the configured interval, OpenDoors displays the inactivity
timeout message, disconnects the caller, and begins shutdown. The separate
[`od_inactive_warning`](runtime.md#od_inactive_warning) field controls how many
seconds before the limit the warning is issued.

Set this field to zero after initialization to disable inactivity shutdown, or
use [`od_disable_inactivity`](#od_disable_inactivity) when a temporary
operation must suspend inactivity processing without discarding the configured
limit. OpenDoors reads the value in its kernel and does not change it after
installing the default.

### `od_list_pause`

```c
char od_control.od_list_pause;
```

This variable contains a Boolean value, which allows you to control whether or not the user may pause displaying within the [`od_list_files()`](../api/od_list_files.md) and [`od_send_file()`](../api/od_send_file.md) function. When this variable is set to its default value of TRUE, the user will be able to pause the display by pressing the [P] key, and resume display by pressing any other key. However, the pause feature may be disabled by setting this variable to FALSE.

### `od_list_stop`

```c
char od_control.od_list_stop;
```

This variable contains a Boolean value, which allows you to control whether or not the user may abort displaying within the [`od_list_files()`](../api/od_list_files.md) and [`od_send_file()`](../api/od_send_file.md) function. When this variable is set to its default value of TRUE, the user will be able to pause the display by pressing the [S], [CTRL]-[K] or [CTRL]-[C] keys. However, the stop feature may be disabled by setting this variable to FALSE.

### `od_logfile`

```c
OD_COMPONENT *od_control.od_logfile;
```

To make the OpenDoors log file system available in your program, set this variable to INCLUDE_LOGFILE, prior to calling any OpenDoors functions. If not set, or if set to NO_LOGFILE, the OpenDoors log file system will not automatically be enabled.

### `od_logfile_disable`

```c
BOOL od_control.od_logfile_disable;
```

This variable defaults to the value of FALSE, unless the "LogfileDisable" option is specified in the configuration file, in which case the variable will be set to TRUE. If this variable is set to TRUE, OpenDoors will not write to a logfile, even if the logfile system is enabled using [`od_control.od_logfile`](#od_logfile). Setting it after a logging session has opened stops subsequent entries; orderly OpenDoors shutdown still closes the active stream and releases the logging hooks.

### `od_logfile_messages`

```c
char *od_control.od_logfile_messages[14];
```

This array of pointers to strings contains the messages that OpenDoors will automatically write to the log file, if the log file system is enabled. If you wish to change the settings of this array, you should do so before calling any OpenDoors functions. The default strings for this array are as follows:

[0] "Carrier lost, exiting door" [1] "System operator terminating call, exiting door" [2] "User's time limit expired, exiting door" [3] "User keyboard inactivity time limit exceeded, exiting door" [4] "System operator returning user to BBS, exiting door" [5] "Exiting door with errorlevel %d, [6] "Invoking operating system shell" [7] "Returning from operating system shell" [8] "User paging system operator" [9] "Entering sysop chat mode" [10] "Terminating sysop chat mode" [11] "%s entering door" [12] "Reason for chat: %s" [13] "Exiting door"

Entries 5, 11, and 12 are `printf`-style templates receiving, respectively,
an integer error level, the caller name, and the caller's chat reason. Their
expanded text must fit in the internal 1,025-byte work buffer, including its
terminating null byte. An oversized startup entry makes
[`od_log_open()`](../api/od_log_open.md) fail with
[`ERR_LIMIT`](../constants/errors.md#err_limit); an oversized chat-reason entry
makes the standard-message operation fail with the same error. An oversized
exit entry is omitted while orderly log closure continues. After successful
expansion, the chat-reason entry retains its traditional 67-character limit.

### `od_logfile_name`

```c
char od_control.od_logfile_name[80];
```

This variable specifies the filename, and optionally the full path of the logfile where OpenDoors should perform logging. The array has room for 79 characters plus the terminating null byte. A longer `LogFileName` configuration setting is truncated to fit. This variable only has an effect when set prior to calling any OpenDoors functions. If the log file name is specified in the configuration file, that name will be stored in this variable. If you do not set this variable, and the log file name is not specified in the configuration file, the default name "DOOR.LOG" will be used. If you wish to set this variable, you should do so prior to calling [`od_init()`](../api/od_init.md) or any OpenDoors function.

### `od_maxtime`

```c
WORD od_control.od_maxtime;
```

This variable specifies the maximum length of time that any user is permitted to use the door, and is normally set from a configuration file option. If upon entering the door, the user's time remaining online is greater than the od_maxtime setting, their time remaining is temporarily decreased to the maximum value. Then upon exit of the door, the number of subtracted minutes is added back onto the user's remaining time. If the user's remaining time is less than this value, then the setting has no effect. A value of 0 disables the maximum time setting altogether.

### `od_mps`

```c
OD_COMPONENT *od_control.od_mps;
```

To make the OpenDoors Multiple Personality system available in your program, set this variable to INCLUDE_MPS before calling any OpenDoors functions. If this variable is not set, or is set to NO_MPS, the Multiple Personality System will be disabled. For more information on the OpenDoors Multiple Personality System, see page 233.

### `od_no_ra_codes`

```c
BOOL od_control.od_no_ra_codes;
```

Initialization sets this Boolean to [`TRUE`](../constants/general.md#true), so
RemoteAccess/QuickBBS `^F` and `^K` substitutions are disabled by default. Set
it to [`FALSE`](../constants/general.md#false) to enable the implemented
substitutions in [`od_disp_emu()`](../api/od_disp_emu.md),
[`od_send_file()`](../api/od_send_file.md), and
[`od_hotkey_menu()`](../api/od_hotkey_menu.md). OpenDoors reads the current
value while emulating output and does not change it after initialization.

These substitutions are a BBS display-file convention, not general ANSI or
OpenDoors color codes. Enabling them can consume matching control sequences in
otherwise literal files; the exact implemented and unimplemented codes are
listed in the [`od_send_file()` reference](../api/od_send_file.md).

### `od_nocopyright`

```c
BOOL od_control.od_nocopyright;
```

This member is retained in `tODControl` for source and binary compatibility.
The current OpenDoors implementation does not read it, so setting it to either
[`TRUE`](../constants/general.md#true) or [`FALSE`](../constants/general.md#false) does not change startup output, colors, or any other behavior.
Applications must not use the field to determine whether startup identification
was displayed.

### `od_noexit`

```c
BOOL od_control.od_noexit;
```

This Boolean prevents OpenDoors from terminating the host process when it
shuts down. It defaults to [`FALSE`](../constants/general.md#false). When it is
[`TRUE`](../constants/general.md#true), [`od_exit()`](../api/od_exit.md) still
performs the normal serial-port, door-information, screen, and kernel cleanup,
then returns so the host can continue with non-OpenDoors work. The completed
session cannot be restarted, and every later OpenDoors function call is an
error. The `od_before_exit` callback runs before this value is latched, so the
callback may decide whether the host should continue. Applications which need
to detect automatic shutdown should install that callback.

### `od_page_len`

```c
BYTE od_control.od_page_len;
```

This variable allows you to control the length, in seconds, of the sysop page beep produced when the user pages the sysop via the [`od_page()`](../api/od_page.md) function.

### `od_prog_copyright`

```c
char od_control.od_prog_copyright[40];
```

This variable should contain your program's copyright notice, such as "(C) Copyright 1996 by Your Name". This information is used in the Help|about dialog box under the Win32 version of OpenDoors, and may be used in other places in future versions of OpenDoors.

### `od_prog_name`

```c
char od_control.od_prog_name[40];
```

This variable should contain the full name of your program, up to 39 characters. If not set, OpenDoors will use the string "OpenDoors" in place of this variable. If used, this variable should be set prior to calling any OpenDoors functions, and should not include your program's version number. This information is used to write your program's name in the log file and to indicate your program's name on various windows, among other places.

### `od_prog_version`

```c
char od_control.od_prog_version[40];
```

This variable should contain the version information of your program. If used, this variable should be set prior to calling any OpenDoors functions. This information is used in the Help|About dialog box under the Win32 version of OpenDoors, among other places.

### `od_reg_key`

```c
DWORD od_control.od_reg_key;
```

This member is retained for source and binary compatibility. The current
implementation neither reads nor validates it. No current OpenDoors feature,
API result, startup path, or license check depends on its value. Leave it zero
unless an application itself uses the storage while maintaining compatibility
with an established `tODControl` layout.

### `od_reg_name`

```c
char od_control.od_reg_name[36];
```

This member is retained for source and binary compatibility. The current
implementation does not read or display it and does not associate it with a
license or registration mechanism. Leave the first byte zero unless application
code itself has an established reason to use the storage; any such use is an
application convention rather than OpenDoors behavior.

### `od_spawn_freeze_time`

```c
BOOL od_control.od_spawn_freeze_time;
```

This variable is a Boolean value which indicates whether or not the user's time remaining is frozen during the execution of one of the od_spawn...() functions. If this variable is set to TRUE, the user's time remaining will not decrease during the time that the od_spawn...() function is executing. However, if this variable is set to FALSE, the user's time remaining will continue to be subtracted during the execution of the od_spawn...() function. The default value of this variable is FALSE.

### `od_swapping_disable`

```c
BOOL od_control.od_swapping_disable;
```

This variable is a Boolean value which specifies whether or not OpenDoors will attempt to swap itself and your entire door upon DOS shell or a call to one of the od_spawn...() functions. This variable defaults to FALSE. If set to TRUE, OpenDoors will not attempt to perform swapping activities.

### `od_swapping_noems`

```c
BOOL od_control.od_swapping_noems;
```

This variable is a Boolean value which can be used to prevent OpenDoors from swapping to EMS memory. This variable defaults to the value FALSE. If set to TRUE, OpenDoors will not attempt to use EMS memory for swapping, and will only swap to disk.

### `od_swapping_path`

```c
char od_control.od_swapping_path[80];
```

This variable specifies the drive and directory where OpenDoors should create its disk swapping file, if applicable. More than one path can be specified, by separating the paths with a semi- colon (;) character.

### `od_status_on`

```c
BOOL od_control.od_status_on;
```

Kernel initialization sets this Boolean to [`TRUE`](../constants/general.md#true),
so an application which wants to disable the local status subsystem must do so
after [`od_init()`](../api/od_init.md). While false, periodic status updates
are skipped and [`od_set_statusline()`](../api/od_set_statusline.md) returns
without changing the selected line. Local personality keys therefore cannot
bring the status display back.

Changing the field from false to true causes the next kernel pass to select
and draw the personality's normal status line. Changing it from true to false
stops further updates but does not erase cells which were already drawn; an
application which requires a clean local display must arrange the local
presentation it wants. OpenDoors reads the field during kernel and status-line
processing. Apart from setting the initial true value, it does not change an
application's later selection.

This differs from selecting status-line setting 8 with
[`od_set_statusline()`](../api/od_set_statusline.md). That selection temporarily
removes the displayed line while leaving the subsystem and its local selection
keys enabled. Set this field false only when the application does not want the
status subsystem to operate at all. On platforms without the DOS text-mode
personality interface, the field remains in the control structure but has no
visible local status line to control.

## Output and diagnostic controls

### `od_always_clear`

```c
BOOL od_control.od_always_clear;
```

This field determines how [`od_clr_scr()`](../api/od_clr_scr.md) treats the
caller's screen-clearing preference. Initialization unconditionally sets it to
[`TRUE`](../constants/general.md#true), which makes every call clear the screen. Set it to [`FALSE`](../constants/general.md#false) after
initialization to honor bit `0x02` of
[`user_attribute`](caller.md#user_attribute) when an extended
`EXITINFO.BBS` record or a [`CUSTOM`](../constants/session.md#custom) door-information reader supplies that
field: OpenDoors clears when the bit is set and leaves the screen unchanged
when it is clear.

For other door-information formats, OpenDoors has no screen-clearing
preference to honor and clears the screen even when this field is [`FALSE`](../constants/general.md#false).
The field affects both local and remote clearing as one operation; it does not
cause either screen to be cleared independently.

### `od_color_char`

```c
char od_control.od_color_char;
```

This field enables the compact binary color form accepted by
[`od_printf()`](../api/od_printf.md). When the configured marker is encountered
in formatted output, the immediately following byte is passed as an IBM-PC
attribute to [`od_set_attrib()`](../api/od_set_attrib.md); neither byte is
displayed. For example, if the marker is `1`, the byte sequence `1, 0x0e`
selects yellow on black.

Initialization unconditionally sets this field to zero, disabling the binary
form. Assign a nonzero marker afterward. This mechanism is independent of the
delimited color-name syntax selected by
[`od_color_delimiter`](#od_color_delimiter); setting both fields to zero
disables all inline color processing in [`od_printf()`](../api/od_printf.md).

### `od_disable_inactivity`

```c
BOOL od_control.od_disable_inactivity;
```

When [`TRUE`](../constants/general.md#true), this field temporarily suppresses both the inactivity warning and
inactivity shutdown without changing [`od_inactivity`](#od_inactivity) or its
last-activity timestamp. It is initially [`FALSE`](../constants/general.md#false). The Win32 local User menu can
toggle it, and application code may change it at runtime. Re-enabling the timer
after its deadline has passed can cause timeout processing on the next kernel
update.

### `od_full_color`

```c
BOOL od_control.od_full_color;
```

OpenDoors normally compares a requested attribute with
[`od_cur_attrib`](runtime.md#od_cur_attrib) and transmits only the ANSI or
AVATAR changes which are required. Setting this initially false field to
[`TRUE`](../constants/general.md#true) makes each [`od_set_attrib()`](../api/od_set_attrib.md) call transmit a
complete color selection even when OpenDoors believes the attribute is already
active. This can resynchronize a terminal whose state was changed outside the
OpenDoors output interfaces, at the cost of additional output.

### `od_full_put`

```c
BOOL od_control.od_full_put;
```

When [`od_puttext()`](../api/od_puttext.md) updates a remote screen, OpenDoors
normally compares the new block with the saved virtual screen and skips runs
whose characters and effective backgrounds are already identical. Setting
this initially false field to [`TRUE`](../constants/general.md#true) disables that optimization and transmits
the complete requested rectangle. Local virtual-screen storage is updated in
either mode.

### `od_internal_debug`

```c
BOOL od_control.od_internal_debug;
```

This field is read only by builds compiled with the private `OD_DIAGNOSTICS`
instrumentation. In the diagnostic Win32 build, setting it to [`TRUE`](../constants/general.md#true) enables
modal progress and modem-command message boxes during shutdown. It is
initially [`FALSE`](../constants/general.md#false); ordinary distributed builds compile out these checks, so
assigning it has no effect there. The field remains public for structure and
diagnostic-build compatibility.

### `od_cmd_show`

```c
int od_control.od_cmd_show;
```

This field exists only in the Win32 structure layout. A GUI application should
copy the `nCmdShow` value received by `WinMain()` into it before initialization.
If the value is `SW_MINIMIZE`, `SW_SHOWMINIMIZED`, or `SW_SHOWMINNOACTIVE`,
OpenDoors initially shows its local frame minimized without activating it; all
other values cause the frame to be restored. The static value is zero, which
therefore follows the restore path. OpenDoors reads but never writes the field.

## Function keys

The local system operator can enter chat mode, invoke a command shell, change
the caller's remaining time, and perform the other operations described below
without those keystrokes being passed to the caller. The fields in this section
use the IBM BIOS key representation: the high byte is the keyboard scan code,
and the low byte is the ASCII character when the key produces one. Thus
`0x2e00` identifies Alt-C and `0x4800` identifies Up Arrow.

The DOS and DOS32 personalities assign the built-in mappings during
initialization. Consequently, an application which replaces one of these
mappings must do so after [`od_init()`](../api/od_init.md), or after the first
API call which causes initialization. A value of zero leaves an operation
without an ordinary keyboard mapping. On platforms which do not provide the
DOS personality system, these fields retain their zero-initialized values
unless the application assigns them.

OpenDoors tests built-in operations before application-defined hot keys. A
custom hot key which duplicates a built-in mapping therefore does not receive
the keystroke.

### `key_chat`

```c
WORD od_control.key_chat;
```

This field selects the key which enters or leaves sysop chat. The standard and
RemoteAccess personalities use Alt-C (`0x2e00`), PCBoard uses F10 (`0x4400`),
and Wildcat uses Alt-A (`0x4100`). While chat is active, the Escape key also
ends chat independently of this setting.

### `key_dosshell`

```c
WORD od_control.key_dosshell;
```

This field selects the key which invokes the local command shell. The standard
and RemoteAccess personalities use Alt-J (`0x2400`), PCBoard uses F5
(`0x3f00`), and Wildcat uses Alt-D (`0x2000`). OpenDoors processes the before
and after shell strings and callbacks around the shell operation.

### `key_drop2bbs`

```c
WORD od_control.key_drop2bbs;
```

This field selects the key which terminates the door and returns the connected
caller to the BBS without dropping the connection. The standard and
RemoteAccess personalities use Alt-D (`0x2000`), PCBoard uses Alt-X
(`0x2d00`), and Wildcat uses F10 (`0x4400`). OpenDoors performs the shutdown;
the application may change the mapping but does not otherwise read this field.

### `key_hangup`

```c
WORD od_control.key_hangup;
```

This field selects the key which terminates the caller's connection and shuts
down the door with the configured hangup error level. The standard and
RemoteAccess personalities use Alt-H (`0x2300`), PCBoard uses F8 (`0x4200`),
and Wildcat leaves the operation unassigned.

### `key_keyboardoff`

```c
WORD od_control.key_keyboardoff;
```

This field selects the key which toggles
[`od_user_keyboard_on`](runtime.md#od_user_keyboard_on), thereby enabling or
disabling input from the remote caller. All five built-in personalities use
Alt-K (`0x2500`). Local sysop keys remain active; to disable local keyboard
processing, use the appropriate [`od_disable`](#od_disable) flag.

### `key_lesstime`

```c
WORD od_control.key_lesstime;
```

This field selects the key which subtracts one minute from
[`user_timelimit`](caller.md#user_timelimit). The value is never reduced below
zero. The standard and RemoteAccess personalities use Down Arrow (`0x5000`);
PCBoard and Wildcat leave this built-in operation unassigned. The Wildcat
personality separately installs Down Arrow as a personality hot key and uses it
to subtract five minutes.

### `key_lockout`

```c
WORD od_control.key_lockout;
```

This field selects the key which sets
[`user_security`](caller.md#user_security) to zero, terminates the connection,
and shuts down the door with the hangup error level. The standard and
RemoteAccess personalities use Alt-L (`0x2600`), PCBoard uses F2 (`0x3c00`),
and Wildcat uses Alt-0 (`0x8100`).

### `key_moretime`

```c
WORD od_control.key_moretime;
```

This field selects the key which adds one minute to
[`user_timelimit`](caller.md#user_timelimit), up to 1,440 minutes. The standard
and RemoteAccess personalities use Up Arrow (`0x4800`); PCBoard and Wildcat
leave this built-in operation unassigned. The Wildcat personality separately
installs Up Arrow as a personality hot key and uses it to add five minutes.

### `key_status`

```c
WORD od_control.key_status[9];
```

Each element selects the key which activates the status-line number having the
same array index. There are nine elements, for lines 0 through 8. The mapping
has no visible effect while [`od_status_on`](#od_status_on) is [`FALSE`](../constants/general.md#false), and
pressing the key for the already active line does not redraw it.

The RemoteAccess personality maps F1 through F7 to elements 0 through 6, F9 to
element 7, and F10 to element 8. The standard personality maps F1, F9, and F10
to elements 0, 7, and 8 and leaves the others unassigned. PCBoard maps Alt-H
to element 1 and may exchange the element 0 and element 1 mappings as its
display changes. Wildcat leaves all nine elements unassigned. The compatibility
macros [`key_nohelp`](../compatibility.md) and [`key_help`](../compatibility.md) name elements 0 and 6 respectively.

### `key_sysopnext`

```c
WORD od_control.key_sysopnext;
```

This field selects the key which toggles
[`sysop_next`](runtime.md#sysop_next). The standard, RemoteAccess, and PCBoard
personalities use Alt-N (`0x3100`); Wildcat uses F1 (`0x3b00`).

### `od_num_keys`

```c
BYTE od_control.od_num_keys;
```

This field is the number of active entries in [`od_hot_key`](#od_hot_key) and
[`od_hot_function`](#od_hot_function). It is initially zero, but a personality
may add its own entries during initialization. The maximum is 16; assigning a
larger value causes OpenDoors to read beyond both arrays.

An application which installs custom local keys should do so after
initialization, preserve any entries already installed by the personality, and
never increment this field beyond 16.

### `od_hot_key`

```c
INT16 od_control.od_hot_key[16];
```

The first [`od_num_keys`](#od_num_keys) elements contain application- or
personality-defined local key codes. OpenDoors compares each otherwise
unhandled local keystroke with these entries. A match is not placed in the
normal local/remote input queue and is not passed to
[`od_local_input`](#od_local_input).

The array is zero-initialized. OpenDoors does not assign application entries;
the DOS personalities may append private entries through the personality SDK.
[`ODStatRemoveKey()`](../personality/ODStatRemoveKey.md) removes a key by moving
the final active key and its callback into the removed position, then clearing
the vacated pair. Consequently, removal does not preserve array order.

### `od_hot_function`

```c
void (*od_control.od_hot_function[16])(void);
```

Each element is the optional callback for the corresponding
[`od_hot_key`](#od_hot_key) entry. After a matching key is stored in
[`od_last_hot`](runtime.md#od_last_hot), OpenDoors first notifies the active
personality with [`PEROP_CUSTOMKEY`](../constants/components.md#perop_customkey), then calls this function if the pointer is
not `NULL`. The callback is synchronous and receives no arguments. It may use
the array index known when it was installed or examine
[`od_last_hot`](runtime.md#od_last_hot); a personality is permitted to clear
that field before the callback runs.

All pointers are initially `NULL`. OpenDoors does not install application
callback pointers. [`ODStatRemoveKey()`](../personality/ODStatRemoveKey.md) may
move a pointer together with its corresponding key and clears the pointer in
the vacated final slot.

For example, the following adds Page Up as an application-defined local key,
provided fewer than 16 keys are already installed:

```c
static void add_points(void)
{
    current_user->points += 10;
}

if (od_control.od_num_keys < 16) {
    unsigned int key = od_control.od_num_keys++;
    od_control.od_hot_key[key] = 0x4900;
    od_control.od_hot_function[key] = add_points;
}
```

## Color customization

These fields contain IBM-PC text attributes in the format accepted by
[`od_set_attrib()`](../api/od_set_attrib.md): the low four bits select the
foreground, bits 4 through 6 select the background, and bit 7 selects blinking
when that mode is supported. The optional configuration component can set most
of them through the corresponding `Colour` keyword.

During initialization, OpenDoors replaces every zero-valued color field with
the default given below. Consequently, a nonzero application override may be
assigned before initialization, but attribute zero (black on black) must be
assigned afterward.

### `od_chat_color1`

```c
BYTE od_control.od_chat_color1;
```

This is the attribute used for text entered by the local sysop in built-in chat
mode. It is also used for the before- and after-chat messages and as the common
chat attribute outside typed text. Its default is `0x0c` (light red on black).
The `ChatSysopColour` configuration setting can replace it. OpenDoors reads
the field whenever it selects the sysop chat color.

### `od_chat_color2`

```c
BYTE od_control.od_chat_color2;
```

This is the attribute used for text entered by the remote caller in built-in
chat mode. Its default is `0x0f` (bright white on black). The
`ChatUserColour` configuration setting can replace it.

### `od_list_title_col`

```c
BYTE od_control.od_list_title_col;
```

[`od_list_files()`](../api/od_list_files.md) uses this attribute for blank
lines and lines beginning with a space, which serve as headings or comments in
a `FILES.BBS` listing. Its default is `0x0f`. The `FileListTitleColour`
configuration setting can replace it.

### `od_list_name_col`

```c
BYTE od_control.od_list_name_col;
```

This is the filename attribute in a `FILES.BBS` listing, including entries
whose referenced file is unavailable. Its default is `0x0e`. The
`FileListNameColour` configuration setting can replace it.

### `od_list_size_col`

```c
BYTE od_control.od_list_size_col;
```

This is the file-size attribute for available files in a `FILES.BBS` listing.
Its default is `0x0d`. The `FileListSizeColour` configuration setting can
replace it.

### `od_list_comment_col`

```c
BYTE od_control.od_list_comment_col;
```

This is the file-description attribute in a `FILES.BBS` listing. Its default
is `0x03`. The `FileListDescriptionColour` configuration setting can replace
it.

### `od_list_offline_col`

```c
BYTE od_control.od_list_offline_col;
```

This is the attribute used for [`od_offline`](#od_offline) when a
`FILES.BBS` entry names a file which cannot be found. Its default is `0x0c`.
The `FileListOfflineColour` configuration setting can replace it.

### `od_continue_col`

```c
BYTE od_control.od_continue_col;
```

This is the attribute used for the [`od_continue`](#od_continue) page prompt.
OpenDoors restores the previous attribute before reading the response or
erasing the prompt. Its default is `0x0f`. The `PagePromptColour`
configuration setting can replace it.

### `od_local_win_col`

```c
BYTE od_control.od_local_win_col;
```

This attribute supplies both the text and border colors for local message
windows, including the startup information-file message and the shutdown
message. Its default is `0x19`. No built-in configuration-file keyword changes
it; application code may assign it.

### `od_menu_title_col`

```c
BYTE od_control.od_menu_title_col;
```

This is the title attribute used by
[`od_popup_menu()`](../api/od_popup_menu.md). Its default is `0x74`. The
`PopupMenuTitleColour` configuration setting can replace it.

### `od_menu_border_col`

```c
BYTE od_control.od_menu_border_col;
```

This is the popup-menu border and separator attribute. Its default is `0x70`.
The `PopupMenuBorderColour` configuration setting can replace it.

### `od_menu_text_col`

```c
BYTE od_control.od_menu_text_col;
```

This is the ordinary text attribute for an unselected popup-menu item. Its
default is `0x70`. The `PopupMenuTextColour` configuration setting can replace
it.

### `od_menu_key_col`

```c
BYTE od_control.od_menu_key_col;
```

This is the shortcut-key attribute within an unselected popup-menu item. Its
default is `0x7f`. The `PopupMenuKeyColour` configuration setting can replace
it.

### `od_menu_highlight_col`

```c
BYTE od_control.od_menu_highlight_col;
```

This is the text attribute for the selected popup-menu item. Its default is
`0x07`. The `PopupMenuHighlightColour` configuration setting can replace it.

### `od_menu_highkey_col`

```c
BYTE od_control.od_menu_highkey_col;
```

This is the shortcut-key attribute within the selected popup-menu item. Its
default is `0x0f`. The `PopupMenuHighKeyColour` configuration setting can
replace it.

## Text customization

The prompts in this section allow the application's local interface and caller
messages to be translated or otherwise customized. Initialization assigns all
of their defaults unconditionally, so replacements must be installed after
[`od_init()`](../api/od_init.md), or after another API call has caused
initialization. For example:

```c
od_control.od_before_shell=(char *)"\n\rJust a moment...\n\r";
```

The detailed entries below give the current default from the implementation,
the field's exact consumer, any required formatting conversions, and the cases
in which a fixed display width or `NULL` value has special meaning.


### `od_after_chat`

```c
char *od_control.od_after_chat;
```

This string is displayed when built-in chat cleanup begins, before
[`od_cafter_chat`](#od_cafter_chat) is called. Its default is
`"\n\rChat mode ended.\n\r\n\r"`. Assign `NULL` after initialization to suppress
the message. OpenDoors reads the string but does not modify or free it.

### `od_after_shell`

```c
char *od_control.od_after_shell;
```

This string is displayed after the local command interpreter returns and
before [`od_cafter_shell`](#od_cafter_shell) is called. Its default is
`"\n\r...Thanks for waiting\n\r\n\r"`. Assign `NULL` after initialization to
suppress the message.

### `od_before_chat`

```c
char *od_control.od_before_chat;
```

This string is displayed after [`od_cbefore_chat`](#od_cbefore_chat) returns
and immediately before the built-in chat input loop. Its default is
`"\n\rThe system operator has placed you in chat mode to talk with you:\n\r\n\r"`.
Assign `NULL` after initialization to suppress the message.

### `od_before_shell`

```c
char *od_control.od_before_shell;
```

This string is displayed after [`od_cbefore_shell`](#od_cbefore_shell) returns
and immediately before OpenDoors invokes the local command interpreter. Its
default is `"\n\rPlease wait a moment...\n\r"`. Assign `NULL` after
initialization to suppress the message.

### `od_chat_reason`

```c
char *od_control.od_chat_reason;
```

[`od_page()`](../api/od_page.md) displays this prompt before reading the
caller's reason for requesting chat. The default is
`"               Why would you like to chat? (Blank line to cancel)\n\r"`.
The fifteen leading spaces center the prompt over the 79-column input frame.
This pointer must not be `NULL` when paging is used.

### `od_continue`

```c
char *od_control.od_continue;
```

This is the page-boundary prompt used by display-file and file-listing
operations. The default is `"Continue? [Y/n/=]"`. OpenDoors uses its byte
length to erase the prompt after a valid response, so embedded terminal
control sequences or characters whose displayed width differs from one column
will leave the erasure width incorrect. The pointer must not be `NULL` while
page pausing is enabled.

### `od_continue_yes`

```c
char od_control.od_continue_yes;
```

This is the affirmative response to [`od_continue`](#od_continue). Its default
is lowercase `y`; OpenDoors accepts both cases. Enter and Space are always
accepted as affirmative responses independently of this field.

### `od_continue_no`

```c
char od_control.od_continue_no;
```

This is the response which stops the current paged display. Its default is
lowercase `n`; OpenDoors accepts both cases. Escape, Ctrl-C, Ctrl-K, and `s` in
either case also stop the display independently of this field.

### `od_continue_nonstop`

```c
char od_control.od_continue_nonstop;
```

This response continues the current display and disables further page prompts
for that operation. Its default is `=`; OpenDoors applies the usual
case-insensitive comparison even though the default has no case.

### `od_day`

```c
char *od_control.od_day[7];
```

These strings contain abbreviated weekday names in Sunday-through-Saturday
order. The defaults are `"Sun"`, `"Mon"`, `"Tue"`, `"Wed"`, `"Thu"`, `"Fri"`,
and `"Sat"`. The logfile component uses them in each session-opening date.
OpenDoors does not require a three-byte value, but longer strings change the
log layout.

### `od_month`

```c
char *od_control.od_month[12];
```

These strings contain abbreviated month names in January-through-December
order. The defaults are `"Jan"` through `"Dec"`. The logfile component uses
them in session-opening dates, and the RemoteAccess personality uses them in a
local status line. Longer strings therefore alter the log layout and can
overwrite adjacent status-line fields.

### `od_hanging_up`

```c
char *od_control.od_hanging_up;
```

This is the local message-window text shown while OpenDoors is terminating the
call. Its default is `"Ending call..."`. A `NULL` pointer suppresses this
window on the terminating-call path. The string is not sent to the remote
caller.

### `od_exiting`

```c
char *od_control.od_exiting;
```

This is the local message-window text shown during a normal return to the BBS.
Its default is `"Program is exiting..."`. Unlike
[`od_hanging_up`](#od_hanging_up), this pointer is not checked for `NULL` before
it is passed to the local window implementation.

### `od_help_text`

```c
char *od_control.od_help_text;
```

The standard DOS and DOS32 personality displays this string on row 24 of its
help status screen. OD_ONEROW displays it on row 25 when F8 is pressed. The
80-column default is
`"  Alt: [C]hat [H]angup [L]ockout [J]Dos [K]eyboard-Off [D]rop to BBS            "`.
Replacement text should occupy exactly 80 screen columns; shorter text can
leave prior contents visible and longer text can wrap into the following row.
PCBoard, RemoteAccess, and Wildcat do not use this field.

### `od_help_text2`

```c
char *od_control.od_help_text2;
```

The standard DOS and DOS32 personality displays this string on row 25 of its
help status screen. OD_ONEROW displays it there when F9 is pressed. It defaults
to the library's `OD_VER_STATUSLINE` text:
`"  OpenDoors 6.30 - (C) Copyright 1991-2001 by Brian Pirie                      "`.
Replacement text must fit the 80-column row; output beyond its final cell can
wrap and scroll the local screen. PCBoard, RemoteAccess, and Wildcat do not use
this field.

### `od_inactivity_timeout`

```c
char *od_control.od_inactivity_timeout;
```

This message is delivered when the caller's inactivity limit expires, before
OpenDoors shuts down the connection. Its default is
`"\n\rMaximum user inactivity time has elapsed, please call again.\n\r\n\r"`.
If [`od_time_msg_func`](#od_time_msg_func) is non-`NULL`, the string is passed
to that callback instead of being displayed. It contains no formatting
conversion and must not be `NULL` while inactivity checking is enabled.

### `od_inactivity_warning`

```c
char *od_control.od_inactivity_warning;
```

This message is delivered once when the inactivity warning interval begins.
Its default is
`"\n\rWARNING: Inactivity timeout in 10 seconds, press a key now to remain online.\n\r\n\r"`.
The number in this literal is not generated from
[`od_inactive_warning`](runtime.md#od_inactive_warning); an application which
changes that interval should also replace this text. The message is redirected
through [`od_time_msg_func`](#od_time_msg_func) when that callback is present.

### `od_no_keyboard`

```c
char *od_control.od_no_keyboard;
```

The standard DOS and DOS32 personality displays this indicator on row 24 when
remote caller input is disabled. Its default is `"[Keyboard]"`, exactly ten
columns. Replacement text should remain ten columns so that status updates
which erase ten spaces neither leave old characters nor erase adjacent fields.

### `od_no_sysop`

```c
char *od_control.od_no_sysop;
```

[`od_page()`](../api/od_page.md) displays this message when paging is disabled
or the current time is outside the allowed interval. Its default is
`"\n\rSorry, the system operator is not available at this time.\n\r"`. It is
followed immediately by [`od_press_key`](#od_press_key).

### `od_no_response`

```c
char *od_control.od_no_response;
```

[`od_page()`](../api/od_page.md) displays this message if the configured page
duration expires without the sysop entering chat. Its default is
`" No response.\n\r\n\r"`. It is followed immediately by
[`od_press_key`](#od_press_key).

### `od_no_time`

```c
char *od_control.od_no_time;
```

This message is delivered when the caller's remaining session time reaches
zero, immediately before OpenDoors initiates timeout shutdown. Its default is
`"\n\rSorry, you have used up all of your time for this session.\n\r\n\r"`.
[`od_time_msg_func`](#od_time_msg_func) receives it instead when installed.

### `od_offline`

```c
char *od_control.od_offline;
```

[`od_list_files()`](../api/od_list_files.md) displays this marker when a
`FILES.BBS` entry names a file which cannot be found. Its default is
`"[OFFLINE] "`, exactly ten columns. A different length changes the alignment
and available width of the following description field.

### `od_paging`

```c
char *od_control.od_paging;
```

[`od_page()`](../api/od_page.md) displays this text when it begins sounding the
sysop page. Its default is `"\n\rPaging system operator for chat"`. One period
and one bell are then emitted for each second of
[`od_page_len`](#od_page_len), until the sysop answers or the interval expires.

### `od_press_key`

```c
char *od_control.od_press_key;
```

This prompt follows [`od_no_sysop`](#od_no_sysop) and
[`od_no_response`](#od_no_response). Its default is
`"Press [Enter] to continue"`. Despite the wording, the paging paths accept
Enter or Line Feed through [`od_get_answer()`](../api/od_get_answer.md).

### `od_sending_rip`

```c
char *od_control.od_sending_rip;
```

When a `.RIP` file is sent without local echo,
[`od_send_file()`](../api/od_send_file.md) and
[`od_send_file_section()`](../api/od_send_file_section.md) prepend this text to
the filename in a local progress window. Its default is
`"Sending RIP file: "`. It is not transmitted to the caller.

### `od_status_line`

```c
char *od_control.od_status_line[3];
```

The standard DOS and DOS32 personality uses these strings to construct its
primary two-row local status display:

| Element | Default and use |
| --- | --- |
| `0` | `"                                                                     [Node:     "`; the 80-column row-24 background. The personality overwrites the beginning with element 1 and writes the node at column 77. |
| `1` | `"%s of %s at %lu BPS"`; formatted with [`user_name`](caller.md#user_name), [`user_location`](caller.md#user_location), and [`od_connect_speed`](connection.md#od_connect_speed), beginning at column 1 of row 24. |
| `2` | `"Security:        Time:                                               [F9]=Help "`; the 79-column row-25 background. The personality separately fills security, time, and state indicators, then writes the final cell directly. |

Element 1 must retain, in order, two string conversions and one `unsigned
long` conversion compatible with those arguments. Elements 0 and 2 should
remain exactly 80 and 79 screen columns respectively. Longer rendered output
can wrap or overwrite the next status row; shorter background strings can
leave stale characters visible.

### `od_sysop_next`

```c
char *od_control.od_sysop_next;
```

The standard DOS and DOS32 personality displays this indicator on row 25 while
[`sysop_next`](runtime.md#sysop_next) is true. Its default is `"[SN] "`, exactly
five columns. Replacement text should remain five columns because the update
path clears exactly five spaces when the state becomes false.

### `od_time_left`

```c
char *od_control.od_time_left;
```

The standard DOS and DOS32 personality formats this string with
[`user_timelimit`](caller.md#user_timelimit) at column 24 of row 25. Its default
is `"%4d mins  "`. The replacement must contain one integer conversion
compatible with an `int`. For values from zero through 1,440, the default
occupies exactly eleven columns: the number is right-aligned in four columns,
followed by a space, `mins`, and two spaces. It therefore fills columns 24
through 34; the next status field begins at column 35.

### `od_time_warning`

```c
char *od_control.od_time_warning;
```

This format is used when three, two, or one minute remains in the caller's
session. Its default is
`"\n\rWARNING: You only have %d minute(s) remaining for this session.\n\r\n\r"`.
It must contain one integer conversion compatible with an `int`. OpenDoors
formats the result before passing it to [`od_time_msg_func`](#od_time_msg_func)
or displaying it.

### `od_want_chat`

```c
char *od_control.od_want_chat;
```

The standard DOS and DOS32 personality displays this indicator on row 25 while
[`user_wantchat`](caller.md#user_wantchat) is true. Its default is
`"[Want-Chat]"`, exactly eleven columns. Replacement text should remain eleven
columns because the update path clears exactly eleven spaces when the request
is removed.

Initialization assigns every prompt pointer and response character in this
section unconditionally. Applications must therefore replace them after
[`od_init()`](../api/od_init.md), or after another API call has caused
initialization. Except for the explicitly nullable chat, shell, and hangup
messages, prompt pointers must refer to valid strings whenever the associated
facility can use them. OpenDoors reads these application replacements but does
not copy, modify, or free them.
