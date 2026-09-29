# `od_puttext()`

Displays a rectangular region of text and color information

## Synopsis

```c
BOOL od_puttext(INT nLeft, INT nTop, INT nRight, INT nBottom,
    void *pBlock);
```

## Return value

Returns [`TRUE`](../constants/general.md#true) when the complete rectangle has
been displayed, or [`FALSE`](../constants/general.md#false) on failure.

## Description

[`od_puttext()`](od_puttext.md) displays a rectangular block of characters and color
attributes. The block is normally obtained from [`od_gettext()`](od_gettext.md),
but it may also be constructed by the application.

The coordinates are one-based and inclusive. `nLeft` and `nRight` identify
the first and last columns; `nTop` and `nBottom` identify the first and last
rows. `pBlock` must address exactly two bytes of information for every cell in
the rectangle. The application is responsible for providing a buffer large
enough for:

```text
(nRight - nLeft + 1) * (nBottom - nTop + 1) * 2
```

The first byte of each cell is the character code. The second is the IBM-PC
color attribute used by [`od_set_attrib()`](od_set_attrib.md). Cells are
arranged from left to right, beginning with the top row and continuing row by
row. Each row in the buffer must contain exactly the number of cells specified
by the rectangle; there are no line separators or terminators.

ANSI, AVATAR, or RIP operation is required. In remote mode, OpenDoors first
records the entire block in its virtual session screen. It compares the new
cells with the cells previously present and transmits only the portions which
must change. Long runs of unchanged cells are skipped by repositioning the
remote cursor. Setting
[`od_control.od_full_put`](../control/customization.md#od_full_put) to
[`TRUE`](../constants/general.md#true) disables the comparison and causes all
cells to be transmitted.

The virtual session screen is not limited by the size of the local console.
Only the portion which fits the local presentation is shown there, while the
complete rectangle remains available to later OpenDoors screen operations. In
local mode, the block is copied directly to the local text screen.

Except while it is being used internally by [`od_scroll()`](od_scroll.md),
[`od_puttext()`](od_puttext.md) restores the original cursor position after transmitting the
block. It also restores the original display attribute. Direct block output to
the local text screen does not move the local cursor.

The coordinates must describe an ordered rectangle entirely within the
current window, and `pBlock` must not be `NULL`. When AVATAR mode is active,
the right and bottom coordinates must not exceed 255 because the AVATAR cursor
and scrolling commands represent them in single bytes.

If the entire display and its state are to be retained, use
[`od_save_screen_ex()`](od_save_screen_ex.md) and
[`od_restore_screen_ex()`](od_restore_screen_ex.md). The block used by those
functions is opaque and is not interchangeable with an [`od_puttext()`](od_puttext.md) buffer.

## Errors

On failure, [`od_control.od_error`](../control/runtime.md#od_error) is set to:

- [`ERR_PARAMETER`](../constants/errors.md#err_parameter) if the pointer is
  `NULL`, the rectangle is reversed or outside the current window, or the
  underlying screen rejects the operation;
- [`ERR_NOGRAPHICS`](../constants/errors.md#err_nographics) if neither ANSI
  nor AVATAR operation is available;
- [`ERR_MEMORY`](../constants/errors.md#err_memory) if the temporary copy of
  the previous remote rectangle cannot be allocated;
- [`ERR_LIMIT`](../constants/errors.md#err_limit) if the buffer-size
  calculation overflows or an AVATAR coordinate exceeds 255.

## Example

This example constructs a two-cell block without first calling
[`od_gettext()`](od_gettext.md):

```c
unsigned char block[4];

block[0] = 'O';
block[1] = L_WHITE | B_BLUE;
block[2] = 'K';
block[3] = L_YELLOW | B_BLUE;

if(!od_puttext(10, 5, 11, 5, block))
   od_exit(1, FALSE);
```

## See also

[`od_gettext()`](od_gettext.md), [`od_scroll()`](od_scroll.md),
[`od_save_screen_ex()`](od_save_screen_ex.md),
[`od_window_create()`](od_window_create.md)
