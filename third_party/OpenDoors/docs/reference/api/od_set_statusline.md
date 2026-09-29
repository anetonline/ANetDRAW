# `od_set_statusline()`

Selects the status display provided by the current personality.

## Synopsis

```c
void od_set_statusline(INT nSetting);
```

## Description

The DOS personality system can provide one normal status display and seven
alternate displays. [`od_set_statusline()`](od_set_statusline.md) selects one
of those displays or removes the status area from the local screen:

| Value | Selection |
| --- | --- |
| [`STATUS_NORMAL`](../constants/display.md#status_normal) | Normal display, operation 0 |
| [`STATUS_ALTERNATE_1`](../constants/display.md#status_alternate_1) through [`STATUS_ALTERNATE_7`](../constants/display.md#status_alternate_7) | Personality-defined alternate displays, operations 1 through 7 |
| [`STATUS_NONE`](../constants/display.md#status_none) | No status display, operation 8 |

The compatibility names [`STATUS_USER1`](../compatibility.md#status-and-color-aliases)
through [`STATUS_USER4`](../compatibility.md#status-and-color-aliases),
[`STATUS_SYSTEM`](../compatibility.md#status-and-color-aliases), and
[`STATUS_HELP`](../compatibility.md#status-and-color-aliases) designate particular
alternate values. Their descriptive names reflect the historical built-in
personalities; a custom personality determines what each alternate display
actually contains.

If `nSetting` is outside the range 0 through 8, OpenDoors substitutes
[`STATUS_NORMAL`](../constants/display.md#status_normal). The selected value is
stored in
[`od_control.od_current_statusline`](../control/runtime.md#od_current_statusline).

When [`STATUS_NONE`](../constants/display.md#status_none) is selected,
OpenDoors clears the rows which had been occupied by the status display and
expands the local door-output area. Selecting another value invokes the current
personality to draw that display and restores its configured output-screen
boundaries. Cursor position, display attribute, and local screen boundaries are
preserved across the redraw as required for ordinary door output.

The change is temporary. Enabled local status-selection keys can select another
display later. To disable status-line processing completely, set
[`od_control.od_status_on`](../control/customization.md#od_status_on) to
[`FALSE`](../constants/general.md#false); while that field is false this
function makes no change.

If the requested display is already active, the function normally returns
without redrawing it. Setting
[`od_control.od_update_status_now`](../control/runtime.md#od_update_status_now)
to [`TRUE`](../constants/general.md#true) forces the same selection to be drawn
again. Personality modules should normally request an incremental update with
[`ODStatForceStatusUpdate()`](../personality/ODStatForceStatusUpdate.md)
instead.

This function is supported by the DOS and DOS32 text-mode builds. On other
builds it sets [`od_control.od_error`](../control/runtime.md#od_error) to
[`ERR_UNSUPPORTED`](../constants/errors.md#err_unsupported).

## Return value

This function returns no value.

## Example

The following removes the local status display while leaving the personality
and its status-selection keys enabled:

```c
od_set_statusline(STATUS_NONE);
```

## See also

[`od_set_personality()`](od_set_personality.md),
[`ODStatForceStatusUpdate()`](../personality/ODStatForceStatusUpdate.md),
[Display constants](../constants/display.md)
