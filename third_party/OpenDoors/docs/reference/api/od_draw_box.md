# `od_draw_box()`

Draws a bordered rectangle without saving the previous screen contents.

## Synopsis

```c
BOOL od_draw_box(BYTE btLeft, BYTE btTop, BYTE btRight, BYTE btBottom);
```

## Description

The four arguments are one-based, inclusive screen coordinates. `btLeft` and
`btTop` identify the upper-left corner; `btRight` and `btBottom` identify the
lower-right corner. As with [`od_set_cursor()`](od_set_cursor.md), row 1 and
column 1 are at the upper-left of the terminal screen.

[`od_draw_box()`](od_draw_box.md) uses the current display attribute for the
border and interior. Set it beforehand with
[`od_set_color()`](od_set_color.md) or
[`od_set_attrib()`](od_set_attrib.md). The eight characters in
[`od_control.od_box_chars`](../control/customization.md#od_box_chars) define
the corners and edges. If the bottom-edge element is zero, OpenDoors replaces
it with the top-edge character; if the right-edge element is zero, OpenDoors
replaces it with the left-edge character. These replacements modify the
control-structure array and remain in effect for subsequent boxes.

In AVATAR mode, OpenDoors uses the AVATAR clear-area command to fill the
interior and then draws the vertical and horizontal edges. Otherwise it draws
the rectangle using cursor movement, repeated characters, and spaces. RIP by
itself is not tested: the function requires
[`od_control.user_ansi`](../control/caller.md#user_ansi) or
[`od_control.user_avatar`](../control/caller.md#user_avatar) to be enabled. A
RIP session for which ANSI is also enabled uses the ANSI path.

The box is displayed on the normal OpenDoors output path and therefore appears
on the remote screen and on the library's local or virtual screen. Unlike
[`od_window_create()`](od_window_create.md), this function neither saves nor
restores the cells it replaces.

Coordinates must be within columns 1 through 80 and rows 1 through 25. The
right edge must follow the left edge, and the bottom edge must follow the top
edge. The smallest valid box is therefore two columns by two rows. Adjacent
edges are permitted; such a box has no interior cells between those edges.

## Return value

The function returns [`TRUE`](../constants/general.md#true) after the box has
been drawn. It returns [`FALSE`](../constants/general.md#false) and sets
[`od_control.od_error`](../control/runtime.md#od_error) to
[`ERR_PARAMETER`](../constants/errors.md#err_parameter) for a coordinate
outside the 80-column by 25-row range or for equal or reversed edges, or to
[`ERR_NOGRAPHICS`](../constants/errors.md#err_nographics) when neither ANSI nor
AVATAR is enabled.

## Example

```c
od_set_color(L_WHITE, D_BLUE);
if(!od_draw_box(10, 5, 70, 13))
    od_exit(10);
```

## See also

[`od_window_create()`](od_window_create.md),
[`od_set_color()`](od_set_color.md),
[`od_set_attrib()`](od_set_attrib.md),
[`od_set_cursor()`](od_set_cursor.md)
