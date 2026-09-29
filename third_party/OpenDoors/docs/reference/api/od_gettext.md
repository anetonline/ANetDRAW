# `od_gettext()`

Stores a rectangular region of text and color information

## Synopsis

```c
BOOL od_gettext(INT nLeft, INT nTop, INT nRight, INT nBottom,
    void *pBlock);
```

## Return value

Returns [`TRUE`](../constants/general.md#true) when the complete rectangle has
been stored, or [`FALSE`](../constants/general.md#false) on failure.

## Description

[`od_gettext()`](od_gettext.md) copies the characters and color attributes in a rectangular
portion of the current OpenDoors screen into the buffer addressed by
`pBlock`. The saved rectangle may subsequently be displayed by
[`od_puttext()`](od_puttext.md).

The coordinates are one-based and inclusive. `nLeft` and `nRight` identify
the first and last columns; `nTop` and `nBottom` identify the first and last
rows. The required buffer size, in bytes, is:

```text
(nRight - nLeft + 1) * (nBottom - nTop + 1) * 2
```

The application is responsible for providing a buffer of at least that size.
OpenDoors cannot determine the size of the object addressed by `pBlock` and
does not add a terminator.

Each screen cell occupies two consecutive bytes. The first byte is the
character code and the second is the IBM-PC color attribute described for
[`od_set_attrib()`](od_set_attrib.md). Cells are stored from left to right,
beginning with the top row and continuing row by row through the bottom row.
The buffer is therefore in precisely the format accepted by
[`od_puttext()`](od_puttext.md), and may also be examined or constructed by
the application.

ANSI, AVATAR, or RIP operation is required. The function does not change the
cursor position, current display attribute, or visible contents of the
screen.

For a remote session, the rectangle is taken from OpenDoors' virtual session
screen. Its bounds are the current virtual window and are independent of the
size of the local console. In local mode, the rectangle is taken from the
active local text window.

The coordinates must describe an ordered rectangle entirely within the
current window, and `pBlock` must not be `NULL`. OpenDoors rejects the call if
the left edge follows the right edge or the top edge follows the bottom edge.

If the entire screen is to be retained, [`od_save_screen_ex()`](od_save_screen_ex.md)
and [`od_restore_screen_ex()`](od_restore_screen_ex.md) also preserve the
window, cursor, and display state and work in plain-ASCII mode. The older
[`od_save_screen()`](od_save_screen.md) and
[`od_restore_screen()`](od_restore_screen.md) functions retain their original
fixed-width local-screen format.

## Errors

On failure, [`od_control.od_error`](../control/runtime.md#od_error) is set to:

- [`ERR_PARAMETER`](../constants/errors.md#err_parameter) if `pBlock` is
  `NULL`, a coordinate is outside the current screen, or the rectangle is
  reversed;
- [`ERR_NOGRAPHICS`](../constants/errors.md#err_nographics) if neither ANSI
  nor AVATAR operation is available.

## Example

The following example saves a ten-column, three-row rectangle and restores it
at the same location:

```c
unsigned char block[10 * 3 * 2];

if(od_gettext(5, 4, 14, 6, block))
{
   od_clr_scr();
   od_puttext(5, 4, 14, 6, block);
}
```

## See also

[`od_puttext()`](od_puttext.md), [`od_scroll()`](od_scroll.md),
[`od_save_screen_ex()`](od_save_screen_ex.md),
[`od_window_create()`](od_window_create.md)
