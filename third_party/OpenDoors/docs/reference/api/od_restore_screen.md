# `od_restore_screen()`

Restores a legacy screen saved by [`od_save_screen()`](od_save_screen.md)

## Synopsis

```c
BOOL od_restore_screen(void *pBuffer);
```

## Return value

Returns [`TRUE`](../constants/general.md#true) when restoration succeeds, or
[`FALSE`](../constants/general.md#false) on failure.

## Description

[`od_restore_screen()`](od_restore_screen.md) clears the current display and restores a screen saved
by [`od_save_screen()`](od_save_screen.md). `pBuffer` must contain an unchanged
legacy snapshot, including its four-byte state header and the saved 80-column
cell rows. The function cannot determine the size or validity of an arbitrary
application buffer.

The active local output window must begin at column 1 and end at column 80. If
the current window is taller than the saved window, only the saved number of
rows is restored. If the current window is shorter, OpenDoors skips the rows
at the beginning of the snapshot and restores as many of its final rows as the
window can contain. The saved cursor row is moved upward by the number of
skipped rows. If the saved cursor was above the retained portion, its row is
clamped to the first restored row; its saved column is retained.

When ANSI, AVATAR, or RIP operation is available, OpenDoors clears the screen,
displays the saved character-and-attribute cells with
[`od_puttext()`](od_puttext.md), then restores the saved cursor position and
display attribute. In a remote session the block is also recorded in the
virtual session screen. Because the original snapshot was taken from the
local presentation, however, it may not contain portions of a remote screen
which were outside the local console.

In plain-ASCII mode, color attributes cannot be transmitted. OpenDoors clears
the display and reconstructs the retained text from its first row through the
mapped cursor position, omitting trailing blank cells and stopping before
output which would pass the saved cursor. The saved display attribute is not
applied in this path.

Each saved row remains 80 cells wide regardless of the current destination
width. After a shorter row, OpenDoors sends line feed followed by carriage
return. If a row reaches the destination's final column, OpenDoors sends the
first printable character of the following row before assuming that a wrap
occurred. This works with terminals which wrap immediately and terminals
which retain a DEC last-column flag until the next printable character. When
the mapped cursor belongs at column 1, a space and backspace settle a pending
wrap without restoring text at or beyond the saved cursor. On a destination
wider than the 80-column snapshot, OpenDoors sends the line-feed and
carriage-return pair because cell 80 did not reach the destination margin.

The legacy buffer is not compatible with [`od_puttext()`](od_puttext.md),
[`od_restore_screen_ex()`](od_restore_screen_ex.md), or any application-defined
format. Use only a buffer produced by [`od_save_screen()`](od_save_screen.md).

## Errors

[`ERR_PARAMETER`](../constants/errors.md#err_parameter) is placed in
[`od_control.od_error`](../control/runtime.md#od_error) if `pBuffer` is `NULL`
or the current local output window is not exactly 80 columns wide. In a
graphics mode, an error from [`od_puttext()`](od_puttext.md) is returned and
its error code remains available.

## Example

See the complete chat-callback example under
[`od_save_screen()`](od_save_screen.md#example).

## See also

[`od_save_screen()`](od_save_screen.md),
[`od_restore_screen_ex()`](od_restore_screen_ex.md),
[`od_puttext()`](od_puttext.md), [`od_clr_scr()`](od_clr_scr.md)
