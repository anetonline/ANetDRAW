# `od_scroll()`

Scrolls a rectangular area of the screen

## Synopsis

```c
BOOL od_scroll(INT nLeft, INT nTop, INT nRight, INT nBottom,
    INT nDistance, WORD nFlags);
```

## Return value

Returns [`TRUE`](../constants/general.md#true) when the operation succeeds, or
[`FALSE`](../constants/general.md#false) on failure.

## Description

[`od_scroll()`](od_scroll.md) moves the contents of a rectangular portion of the current
OpenDoors screen upward or downward. The coordinates are one-based and
inclusive. `nLeft` and `nRight` identify the first and last columns; `nTop`
and `nBottom` identify the first and last rows.

A positive `nDistance` scrolls the text upward and creates new rows at the
bottom of the rectangle. A negative value scrolls downward and creates new
rows at the top. A value of zero leaves the display unchanged and succeeds.
If the absolute distance is greater than the height of the rectangle, it is
limited to the rectangle height; the result is therefore equivalent to
scrolling every old row out of the area.

`nFlags` may be one of the following values:

| Value | Meaning |
| --- | --- |
| [`SCROLL_NORMAL`](../constants/display.md#scroll_normal) | Clear newly exposed rows in the current display attribute. |
| [`SCROLL_NO_CLEAR`](../constants/display.md#scroll_no_clear) | Permit the ANSI implementation to omit the clearing pass. This flag is intended for an application which will immediately overwrite every newly exposed cell. |

[`SCROLL_NO_CLEAR`](../constants/display.md#scroll_no_clear) does not promise that the exposed cells retain any
particular previous value. The AVATAR implementation always blanks them while
updating the screen model; the ANSI implementation leaves them uncleared.

The function requires ANSI, AVATAR, or RIP operation. In AVATAR mode,
OpenDoors transmits the terminal's rectangular-scroll command, updates the
virtual or local screen, and blanks the newly exposed rows. In ANSI mode, it
moves the retained cells using [`od_gettext()`](od_gettext.md) and
[`od_puttext()`](od_puttext.md), then clears exposed rows unless
[`SCROLL_NO_CLEAR`](../constants/display.md#scroll_no_clear) was specified. Clearing is most efficient when the right
edge of the rectangle is the right edge of the current window, because
[`od_clr_line()`](od_clr_line.md) can then be used instead of transmitting a
row of spaces.

Newly cleared rows use the current display attribute. The original cursor
position is restored when the operation completes. The current display
attribute is not deliberately changed by [`od_scroll()`](od_scroll.md); the ANSI block-output
path restores it before returning.

For a remote session, the operation applies to the virtual session screen and
is not limited by the local console dimensions. The rectangle must be entirely
within the current virtual window. In local mode it must be within the active
local text window. When AVATAR is active, the right and bottom coordinates
must not exceed 255 because the terminal command represents them in single
bytes.

## Errors

On failure, [`od_control.od_error`](../control/runtime.md#od_error) is set to:

- [`ERR_PARAMETER`](../constants/errors.md#err_parameter) if the rectangle is
  reversed or outside the current window;
- [`ERR_NOGRAPHICS`](../constants/errors.md#err_nographics) if neither ANSI
  nor AVATAR operation is available;
- [`ERR_MEMORY`](../constants/errors.md#err_memory) if a temporary block or
  blank row cannot be allocated;
- [`ERR_LIMIT`](../constants/errors.md#err_limit) if a temporary-buffer size
  cannot be represented or an AVATAR coordinate exceeds 255.

## Example

The following call moves rows 5 through 15 upward by one line and clears the
new row 15:

```c
if(!od_scroll(1, 5, od_control.user_screenwidth, 15,
    1, SCROLL_NORMAL))
{
   od_exit(1, FALSE);
}
```

## See also

[`od_gettext()`](od_gettext.md), [`od_puttext()`](od_puttext.md),
[`od_clr_line()`](od_clr_line.md),
[`od_window_create()`](od_window_create.md),
[`od_window_remove()`](od_window_remove.md)
