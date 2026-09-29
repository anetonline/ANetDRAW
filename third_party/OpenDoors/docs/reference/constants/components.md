# Components and personalities

OpenDoors packages three optional subsystems as component initializer pointers
and provides five built-in DOS and Windows-console local-interface personalities. Select components
and the default personality before [`od_init()`](../api/od_init.md) or any call
which triggers automatic initialization.

## Configuration component

### `INCLUDE_CONFIG_FILE`

Assign `INCLUDE_CONFIG_FILE` to
[`od_control.od_config_file`](../control/customization.md#od_config_file) to
enable the built-in configuration-file reader. During initialization it locates
the configured file, processes standard keywords, invokes custom keyword
callbacks, and applies settings before the active session begins.

The macro is a correctly typed reference to [`ODConfigInit`](#odconfiginit), not a Boolean.
Assign the macro as shown; do not compare the field with `TRUE` or call the
initializer directly.

```c
od_control.od_config_file = INCLUDE_CONFIG_FILE;
od_control.od_config_filename = "example.cfg";
od_init();
```

See [Configuration and command lines](../../guides/configuration.md) for
keyword order, comments, callbacks, and custom drop-file definitions.

### `NO_CONFIG_FILE`

Assign `NO_CONFIG_FILE` to disable the built-in configuration component. It is
a null [`OD_COMPONENT`](../types.md#od_component) pointer. This is useful when the application owns all
configuration or when deterministic initialization must not read a file.

Disabling the component does not disable standard command-line parsing. The
application controls that separately by whether and how it calls
[`od_parse_cmd_line()`](../api/od_parse_cmd_line.md).

## Log-file component

### `INCLUDE_LOGFILE`

Assign `INCLUDE_LOGFILE` to
[`od_control.od_logfile`](../control/customization.md#od_logfile) to enable the
built-in session logger. The component opens the selected log and writes the
standard lifecycle messages; application entries can be added with
[`od_log_write()`](../api/od_log_write.md).

[`od_logfile_disable`](../control/customization.md#od_logfile_disable) can
suppress opening at run time even when this component is linked and selected.
The name and message table are controlled by adjacent [`od_control`](../control/index.md) members.

### `NO_LOGFILE`

Assign `NO_LOGFILE` when automatic session logging is not wanted. Initialization
does not open the log and the internal `pfLogWrite` hook remains null, so normal
page, chat, shell, and exit events are not logged through the component.

This does not disable the public [`od_log_write()`](../api/od_log_write.md)
function. Calling that function explicitly still attempts to open the configured
log file and write the supplied message unless [`od_logfile_disable`](../control/customization.md#od_logfile_disable) is true.
The application also remains free to maintain an independent log.

## Multiple-personality component

### `INCLUDE_MPS`

Assign `INCLUDE_MPS` to
[`od_control.od_mps`](../control/customization.md#od_mps) to enable the DOS
initialization-time personality selector. During the first part of
[`od_init()`](../api/od_init.md), OpenDoors calls `ODMPSEnable()`, which assigns
[`od_set_personality()`](../api/od_set_personality.md) to its internal selection hook. Later initialization
uses that hook to select the name read into its desired-personality setting;
if selection fails, initialization falls back to the configured default
procedure or `pdef_opendoors()`.

The system is implemented for DOS local interfaces. Portable door logic may
set a default under an `ODPLAT_DOS` or `ODPLAT_DOS32` guard, but must not assume
that a personality changes remote terminal behavior.

### `NO_MPS`

`NO_MPS` is a null [`OD_COMPONENT`](../types.md#od_component) pointer. With this value, initialization does
not install or invoke the initialization-time named-personality selection hook.
It instead initializes
[`od_control.od_default_personality`](../control/customization.md#od_default_personality),
or `pdef_opendoors()` when that field is null.

This value does not remove the public [`od_add_personality()`](../api/od_add_personality.md)
or [`od_set_personality()`](../api/od_set_personality.md) functions. A DOS door
can still call them explicitly. On targets without `OD_TEXTMODE`, those public
functions report `ERR_UNSUPPORTED` regardless of this component field.

## Component entry points

### `ODConfigInit`

`ODConfigInit()` is the initializer named by `INCLUDE_CONFIG_FILE`. It remains
public for source and binary compatibility. Applications should select it
through the component macro so the declaration, pointer type, and initialization
phase remain correct.

### `ODLogEnable`

`ODLogEnable()` is the initializer named by `INCLUDE_LOGFILE`. Assign the macro
to [`od_control.od_logfile`](../control/customization.md#od_logfile) instead of calling this routine directly.

### `ODMPSEnable`

`ODMPSEnable()` is the initializer named by `INCLUDE_MPS`. Direct invocation
sets the same internal selection hook immediately. Assigning `INCLUDE_MPS` to
[`od_control.od_mps`](../control/customization.md#od_mps) is the documented way to have [`od_init()`](../api/od_init.md) invoke it at the
correct phase.

## Built-in personality identifiers

The `PER_*` definitions identify procedure functions. Pass them where a
personality procedure is required; they are not status-line numbers.

### `PER_OPENDOORS`

Selects the standard OpenDoors personality implemented by
`pdef_opendoors()`. It supplies the standard local status presentation and
operator key behavior. The standard personality uses the same personality SDK
contract as the alternatives.

### `PER_OD_ONEROW`

Selects the compact Standard-style personality implemented by
`pdef_od_onerow()`. It reserves only row 25 for the local status display, so
ordinary door output can use rows 1 through 24. The normal row shows caller
identity, time remaining, ANSI/Avatar and sysop-next indicators, the node, and
the F8/F9 help hint; it omits security and connection speed. F8 and F9 display
[`od_help_text`](../control/customization.md#od_help_text) and
[`od_help_text2`](../control/customization.md#od_help_text2), respectively.
F10 hides the status row and F1 restores the normal row.

### `PER_PCBOARD`

Selects `pdef_pcboard()`, whose local status layout and key presentation follow
the PCBoard style. It does not change the drop-file parser into PCBoard mode;
the active drop file is selected independently.

### `PER_RA`

Selects `pdef_ra()`, the RemoteAccess-style local personality. Personality
initialization affects its status and keys only. It does not enable or alter
`EXITINFO.BBS` handling.

### `PER_WILDCAT`

Selects `pdef_wildcat()`, the Wildcat!-style local personality. As with the
other identifiers, the name describes local presentation rather than the
remote protocol or detected door-information format.

Use [`od_add_personality()`](../api/od_add_personality.md) to install a named
choice and [`od_set_personality()`](../api/od_set_personality.md) to activate an
installed choice. [`od_control.od_default_personality`](../control/customization.md#od_default_personality)
sets the procedure used initially.

## Personality procedure operations

OpenDoors invokes an [`OD_PERSONALITY_PROC`](../types.md#od_personality_proc)
with one of the following byte values. A personality should use a `switch` and
ignore operation values it does not implement so a compatible module can run
with later libraries.

### Display operations

`PEROP_DISPLAY1` through `PEROP_DISPLAY8` request a complete draw of status
display slots 1 through 8. A complete draw writes labels, fixed decoration, and
initial dynamic values for that display. It is used when the display is first
selected and whenever something may have overwritten its local screen region.

| Name | Value | Status selection |
| --- | ---: | --- |
| `PEROP_DISPLAY1` | 0 | `STATUS_NORMAL` |
| `PEROP_DISPLAY2` | 1 | `STATUS_ALTERNATE_1` |
| `PEROP_DISPLAY3` | 2 | `STATUS_ALTERNATE_2` |
| `PEROP_DISPLAY4` | 3 | `STATUS_ALTERNATE_3` |
| `PEROP_DISPLAY5` | 4 | `STATUS_ALTERNATE_4` |
| `PEROP_DISPLAY6` | 5 | `STATUS_ALTERNATE_5` |
| `PEROP_DISPLAY7` | 6 | `STATUS_ALTERNATE_6` |
| `PEROP_DISPLAY8` | 7 | `STATUS_ALTERNATE_7` |

Every personality may implement every display slot. A personality which does
not offer meaningful alternate content should still leave the local screen in
a consistent state when it receives the operation.

### Update operations

`PEROP_UPDATE1` through `PEROP_UPDATE8` request an incremental update of the
dynamic content in the corresponding display. The personality can update time,
caller status, and other changing fields without repainting fixed labels.

| Name | Value | Corresponding full draw |
| --- | ---: | --- |
| `PEROP_UPDATE1` | 10 | `PEROP_DISPLAY1` |
| `PEROP_UPDATE2` | 11 | `PEROP_DISPLAY2` |
| `PEROP_UPDATE3` | 12 | `PEROP_DISPLAY3` |
| `PEROP_UPDATE4` | 13 | `PEROP_DISPLAY4` |
| `PEROP_UPDATE5` | 14 | `PEROP_DISPLAY5` |
| `PEROP_UPDATE6` | 15 | `PEROP_DISPLAY6` |
| `PEROP_UPDATE7` | 16 | `PEROP_DISPLAY7` |
| `PEROP_UPDATE8` | 17 | `PEROP_DISPLAY8` |

An update operation is intended to refresh the changing portions of a display
which has already been drawn. A personality can request that this incremental
operation run immediately with
[`ODStatForceStatusUpdate()`](../personality/ODStatForceStatusUpdate.md). That
helper does not request a complete `PEROP_DISPLAY*` redraw.

### `PEROP_INITIALIZE`

Initialize personality-owned status and install its custom local keys. This
operation occurs when the personality becomes active. It is not general
OpenDoors initialization and must not change drop-file parsing, connection
ownership, or unrelated application state.

Use [`ODStatAddKey()`](../personality/ODStatAddKey.md) for personality keys and
store only state which will be undone by `PEROP_DEINITIALIZE`.

### `PEROP_CUSTOMKEY`

Handle the personality's most recently activated custom local key. The key is
reported through
[`od_control.od_last_hot`](../control/runtime.md#od_last_hot). A handler
may update personality state and request a redraw. It should not treat this
operation as remote caller input.

### `PEROP_DEINITIALIZE`

Remove keys and release state owned by the outgoing personality. OpenDoors can
then activate another personality without leaving old callbacks installed.
Use [`ODStatRemoveKey()`](../personality/ODStatRemoveKey.md) for every key added
during initialization.

## Personality SDK boundary

Personality modules include [`ODStat.h`](../personality/index.md), use the [`OD_PERSONALITY_PROC`](../types.md#od_personality_proc) calling
convention, and link only against the documented SDK helpers. [`OpenDoor.h`](../api/index.md)
remains the application API; private screen, frame, string, communication, and
parser headers are not personality interfaces. See
[DOS personalities](../../guides/personalities.md) for module structure,
installation, and build testing.
