# Console personalities

The DOS and Windows-console versions of OpenDoors can replace their local status display and sysop
function keys as a unit. Each such unit is called a *personality*. OpenDoors
includes Standard, OD_ONEROW, PCBoard, RemoteAccess, and Wildcat personalities;
a door may also supply its own.

Personality support is local to the DOS or Windows console. Output produced through the
personality screen functions is never sent to the caller's remote terminal.

## Writing a personality

Include [`OpenDoor.h`](../reference/api/index.md) and
[`ODStat.h`](../reference/personality/index.md), then define an
[`OD_PERSONALITY_PROC`](../reference/types.md). OpenDoors calls the procedure
with a [`PEROP_*`](../reference/constants/components.md#personality-procedure-operations)
operation:

- [`PEROP_INITIALIZE`](../reference/constants/components.md#personality-procedure-operations)
  establishes function keys and initial state.
- [`PEROP_DISPLAY1`](../reference/constants/components.md#personality-procedure-operations)
  through [`PEROP_DISPLAY8`](../reference/constants/components.md#personality-procedure-operations)
  draw complete displays.
- [`PEROP_UPDATE1`](../reference/constants/components.md#personality-procedure-operations)
  through [`PEROP_UPDATE8`](../reference/constants/components.md#personality-procedure-operations)
  update changing fields.
- [`PEROP_CUSTOMKEY`](../reference/constants/components.md#personality-procedure-operations)
  handles the key in
  [`od_control.od_last_hot`](../reference/control/runtime.md).
- [`PEROP_DEINITIALIZE`](../reference/constants/components.md#personality-procedure-operations)
  removes keys and releases personality state.

Install custom keys with
[`ODStatAddKey()`](../reference/personality/ODStatAddKey.md), and remove every
installed key with
[`ODStatRemoveKey()`](../reference/personality/ODStatRemoveKey.md) during
deinitialization. After changing a value displayed by the current status line,
[`ODStatForceStatusUpdate()`](../reference/personality/ODStatForceStatusUpdate.md)
requests its incremental update operation.

## Drawing the local display

The output procedures in [`ODStat.h`](../reference/personality/index.md) use
one-based local-screen coordinates. During a display or update callback,
OpenDoors permits access to the complete 80 by 25 DOS screen and restores the
door's output boundary afterward.

[`ODScrnSetAttribute()`](../reference/personality/ODScrnSetAttribute.md) and
[`ODScrnSetCursorPos()`](../reference/personality/ODScrnSetCursorPos.md) select
the output settings. Text can then be written with
[`ODScrnDisplayChar()`](../reference/personality/ODScrnDisplayChar.md),
[`ODScrnDisplayBuffer()`](../reference/personality/ODScrnDisplayBuffer.md),
[`ODScrnDisplayString()`](../reference/personality/ODScrnDisplayString.md), or
[`ODScrnPrintf()`](../reference/personality/ODScrnPrintf.md).

Rectangles can be cached with
[`ODScrnGetText()`](../reference/personality/ODScrnGetText.md) and restored with
[`ODScrnPutText()`](../reference/personality/ODScrnPutText.md). Their buffers
contain row-major character/attribute pairs and therefore require
`2 * width * height` bytes. Both corners are inclusive.

[`szStatusText`](../reference/personality/szStatusText.md) is the traditional
80-byte shared work area. A personality may instead use its own storage.

## Selecting a personality

To use one personality, assign its procedure to
[`od_control.od_default_personality`](../reference/control/customization.md)
before initialization. To permit runtime selection, set
[`od_control.od_mps`](../reference/control/customization.md) to
[`INCLUDE_MPS`](../reference/constants/components.md), register additional
procedures with
[`od_add_personality()`](../reference/api/od_add_personality.md), and select one
with [`od_set_personality()`](../reference/api/od_set_personality.md).

The output bounds passed to [`od_add_personality()`](../reference/api/od_add_personality.md)
describe the portion reserved for normal door output. For example, lines 1
through 23 leave lines 24 and 25 for the personality.

## Detailed reference

The personality SDK is supported by DOS and by Windows console applications
that define `OD_WINDOWS_CONSOLE` (directly or through a Console CMake target).
Current public declarations are in [`ODStat.h`](../reference/personality/index.md); the operational description below is the complete original reference.

The OpenDoors Multiple Personality System allows  the DOS version of OpenDoors to support multiple sysop function key / status line "personalities". Most commonly, you will use this feature in conjunction with the "Personality" setting in the OpenDoors configuration file, to allow the sysop to choose one of the built-in personalities that most closely mimics the BBS software they are using. OpenDoors includes the following personalities:

| Configuration keyword | Manifest constant |
| --- | --- |
| `STANDARD` | [`PER_OPENDOORS`](../reference/constants/components.md#per_opendoors) |
| `OD_ONEROW` | [`PER_OD_ONEROW`](../reference/constants/components.md#per_od_onerow) |
| `PCBOARD` | [`PER_PCBOARD`](../reference/constants/components.md#per_pcboard) |
| `REMOTEACCESS` | [`PER_RA`](../reference/constants/components.md#per_ra) |
| `WILDCAT` | [`PER_WILDCAT`](../reference/constants/components.md#per_wildcat) |

The PCBoard, RemoteAccess and Wildcat personalities mimic the status lines and function keys used by the BBS packages with those names. The Standard personality, which is the personality used by default, is a trimmed down version of the status lines provided by OpenDoors 4.10 and earlier. OD_ONEROW is a compact Standard-style personality which reserves only row 25, leaving rows 1 through 24 for door output. Its normal row omits security and connection speed. F8 and F9 show its first and second help text, respectively; F10 hides the status row, and F1 restores the normal row.

In addition to using the personalities supplied with OpenDoors, you can create your own personalities. This simply involves writing a function which OpenDoors will call to setup the sysop function keys and to display the status line.

Include the following line before your first call to any OpenDoors function:

```c
od_control.od_mps = INCLUDE_MPS;
```

to include the multiple personality system in your program. This also enables the Personality setting in the configuration file, if you are using the configuration file system.

You can set the default personality to be used by OpenDoors by setting od_control.od_default_personality to one of the manifest constants listed in the table above. If you have included the multiple personality system in your program, this setting will determine the personality to use if the "Personality" option is not set in the configuration file, and your program does not later change the personality using the [`od_set_personality()`](../reference/api/od_set_personality.md) function. If you do not include the multiple personality system in your program, this setting will determine the personality that will always be used.

Creating your own personality involves writing a single function.. Whenever OpenDoors needs to perform an operation that involves the personality, it will call this function, passing one of the following message values:

PEROP_INITIALIZE    Initialize the personality, installing any custom function keys. PEROP_DEINITIALIZE  Deinitialize the personality, returning any changed settings to their original values. PEROP_CUSTOMKEY     Indicates that a custom function key has been pressed. PEROP_DISPLAYx      Where x is a number from 1 to 10. Indicates that the specified status line should be drawn from scratch. PEROP_UPDATEx       Where x is a number from 1 to 10. Indicates that the specified status line should be updated to reflect any changes.

If you have enabled the multiple personality system by setting [`od_control.od_mps`](../reference/control/customization.md#od_mps) to INCLUDE_MPS, you can install your personality function into OpenDoors by calling [`od_add_personality()`](../reference/api/od_add_personality.md). When you call [`od_add_personality()`](../reference/api/od_add_personality.md), you supply a string containing the name of the personality, along with the top and bottom output line numbers to use. These line numbers specify the portion of the screen to use for door output, leaving the remainder of the screen available for displaying the personality's status line. Once the personality has been installed into OpenDoors, it can be selected by the sysop using the "Personality" configuration file option, or manually activated using the [`od_set_personality()`](../reference/api/od_set_personality.md) function. For more information on the [`od_add_personality()`](../reference/api/od_add_personality.md) function, see page 47.

You can make your personality function the default personality by setting od_control.od_default_personality to point to your personality function. As is the case with the built-in personalities, this setting will be used as the default personality if you have enabled the multiple personality system by setting [`od_control.od_mps`](../reference/control/customization.md#od_mps) to INCLUDE_MPS. If you have not enabled the multiple personality system in this manner, your personality function will become the one and only personality used within your program. When creating your own personality, you can use the [`od_control.od_page_statusline`](../reference/control/runtime.md#od_page_statusline) variable to set which status line (if any) will be activated when the user pages the system operator.
