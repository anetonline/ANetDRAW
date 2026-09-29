# `od_set_cursor()`

Positions the text cursor

## Synopsis

```c
void od_set_cursor(INT nRow, INT nColumn);
```

## Return value

N/A

## Description

[`od_set_cursor()`](od_set_cursor.md) moves the cursor to one-based row `nRow` and column
`nColumn` within the active OpenDoors window. ANSI, AVATAR, or RIP operation
is required.

Both coordinates must be at least 1 and must not exceed the current window
dimensions. In a remote session those dimensions are the virtual session
window and need not match the local console. The local cursor follows the
virtual cursor when the selected position is within the portion presented
locally.

In AVATAR mode the coordinates are carried in single-byte fields and neither
may exceed 255, even when the active window is taller. ANSI output writes the
coordinates in decimal and can address rows above 255 when the active window
contains them. The public
[`od_control.user_screenwidth`](../control/caller.md#user_screenwidth) field
is a [`BYTE`](../types.md#byte), while
[`od_control.user_screen_length`](../control/caller.md#user_screen_length)
is a [`WORD`](../types.md#word).

If neither ANSI nor AVATAR operation is available,
[`ERR_NOGRAPHICS`](../constants/errors.md#err_nographics) is placed in
[`od_control.od_error`](../control/runtime.md#od_error). A coordinate below 1
or beyond the active window sets
[`ERR_PARAMETER`](../constants/errors.md#err_parameter). The same error is
reported for an AVATAR coordinate above 255. Invalid coordinates do not move
the local or virtual cursor and are not transmitted to the remote terminal.
The function returns no value, so applications which need to preserve the
previous position should first call [`od_get_cursor()`](od_get_cursor.md).

## Example

```c
if(od_control.user_ansi || od_control.user_avatar)
{
   od_clr_scr();
   od_set_cursor(1, 1);
   od_disp_str("Top-left corner");
   od_set_cursor(15, 1);
   od_disp_str("Fifteenth row\n\r");
}
```

## See also

[`od_get_cursor()`](od_get_cursor.md), [`od_clr_line()`](od_clr_line.md),
[`od_set_attrib()`](od_set_attrib.md)
