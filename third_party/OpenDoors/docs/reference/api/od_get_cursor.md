# `od_get_cursor()`

Returns OpenDoors' current cursor position.

## Synopsis

```c
void od_get_cursor(INT *pnRow, INT *pnColumn);
```

## Parameters

`pnRow`
: Pointer to an [`INT`](../types.md#int) which receives the one-based screen
  row, or `NULL` if the row is not required.

`pnColumn`
: Pointer to an [`INT`](../types.md#int) which receives the one-based screen
  column, or `NULL` if the column is not required.

At least one output pointer must be supplied.

## Return value

This function does not return a value. It stores the requested coordinates in
the supplied output variables.

## Description

OpenDoors tracks the cursor as output passes through its display functions.
[`od_get_cursor()`](od_get_cursor.md) returns that tracked position. Row 1 is the top row of the
current screen or window and column 1 is its leftmost column, using the same
coordinate convention as [`od_set_cursor()`](od_set_cursor.md).

In a remote session, the coordinates come from the virtual session screen.
This is the screen on which OpenDoors performs cursor movement, block
operations, scrolling, windows, and save/restore operations for the caller's
terminal dimensions. The returned position therefore describes the remote
display even when the local console has different dimensions.

When no virtual session screen is active, the position is obtained from the
local text screen. This is normally the case for a local-mode session. The
coordinates are relative to the active local window in the same manner as the
other OpenDoors cursor interfaces.

The function does not transmit a terminal-status query and does not wait for a
cursor report from the caller's terminal. It is OpenDoors' best estimate based
on output performed through the library. Output written directly to the local
console, serial port, socket, or terminal without going through OpenDoors
cannot update this estimate. Applications should likewise avoid assuming that
unrecognized terminal control sequences have changed it in a way OpenDoors can
observe.

Either coordinate may be requested independently:

```c
INT row;
INT column;

od_get_cursor(&row, &column); /* Obtain both. */
od_get_cursor(&row, NULL);    /* Obtain only the row. */
od_get_cursor(NULL, &column); /* Obtain only the column. */
```

Calling [`od_get_cursor()`](od_get_cursor.md) initializes OpenDoors if necessary. Programs which
need to set initialization fields in [`od_control`](../control/index.md) must
do so before this call.

## Errors

If both `pnRow` and `pnColumn` are `NULL`,
[`od_control.od_error`](../control/runtime.md#od_error) is set to
[`ERR_PARAMETER`](../constants/errors.md#err_parameter), and no coordinate is
stored.

## Example

The following code temporarily moves the cursor, writes a message, and returns
to the previous position:

```c
INT saved_row;
INT saved_column;

od_get_cursor(&saved_row, &saved_column);
od_set_cursor(1, 1);
od_disp_str("Status updated");
od_set_cursor(saved_row, saved_column);
```

This restores the cursor position only. It does not restore characters or
attributes overwritten by the message; use the screen save/restore functions
when the underlying display must also be preserved.

## See also

[`od_set_cursor()`](od_set_cursor.md), [`od_save_screen_ex()`](od_save_screen_ex.md),
[Terminal and screen model](../../guides/terminal-screen.md)
