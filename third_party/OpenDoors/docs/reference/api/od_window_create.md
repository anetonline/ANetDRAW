# `od_window_create()`

Creates a popup window and saves the screen cells which it covers.

## Synopsis

```c
void *od_window_create(INT nLeft, INT nTop, INT nRight, INT nBottom,
    char *pszTitle, BYTE btBorderCol, BYTE btTitleCol,
    BYTE btInsideCol, INT nReserved);
```

## Description

The first four arguments are one-based, inclusive screen coordinates.
`nLeft` and `nTop` identify the upper-left corner; `nRight` and `nBottom`
identify the lower-right corner. The rectangle must fit within columns 1
through 80 and rows 1 through 25, and must contain a border plus at least one
interior row and one interior column. In other words, each pair of opposing
edges must be separated by at least two cells. These limits remain part of
this interface when the active virtual screen is larger.

Before drawing, [`od_window_create()`](od_window_create.md) obtains and saves
the cells in the rectangle. It then draws a border using
[`od_control.od_box_chars`](../control/customization.md#od_box_chars), fills
the interior, and optionally centers a title in the top border. As with
[`od_draw_box()`](od_draw_box.md), a zero bottom-edge character is replaced in
the control structure by the top-edge character and a zero right-edge
character is replaced by the left-edge character.

`pszTitle` points to the title, or may be null or point to an empty string when
no title is wanted. OpenDoors places one space on each side of a displayed
title and retains at least one top-border character between each title space
and corner. A window must therefore be at least seven columns wide to display
a title. Narrower windows omit it. A title which does not fit is truncated to
the available bytes and remains centered. `btBorderCol`, `btTitleCol`, and
`btInsideCol` are complete IBM text attributes for the border, title, and
interior respectively. `nReserved` is ignored by this version; pass zero so
that the call remains compatible if a later version assigns it a meaning.

In AVATAR mode, OpenDoors uses the AVATAR clear-area command to fill the
interior. Otherwise it uses the ANSI-compatible cursor and display path. RIP
by itself is not tested: either
[`od_control.user_ansi`](../control/caller.md#user_ansi) or
[`od_control.user_avatar`](../control/caller.md#user_avatar) must be enabled.

On success, the returned value is an opaque, allocated window handle. Pass it
exactly once to [`od_window_remove()`](od_window_remove.md), which restores the
saved cells and releases the allocation. Windows may overlap, but they must be
removed in reverse creation order if each one is to reveal the screen contents
which existed before it was drawn.

## Return value

The function returns an opaque non-null handle on success. It returns `NULL`
and sets [`od_control.od_error`](../control/runtime.md#od_error) as follows:

| Error | Condition |
| --- | --- |
| [`ERR_PARAMETER`](../constants/errors.md#err_parameter) | A coordinate is outside the supported range, the edges are reversed, or the rectangle has no usable interior. |
| [`ERR_NOGRAPHICS`](../constants/errors.md#err_nographics) | Neither ANSI nor AVATAR mode is enabled. |
| [`ERR_MEMORY`](../constants/errors.md#err_memory) | Storage for the saved cells and window description cannot be allocated. |

An error returned by [`od_gettext()`](od_gettext.md) while saving the rectangle
is preserved.

## Example

```c
void *window;

window = od_window_create(17, 9, 63, 15, "DOS Shell",
    0x1f, 0x1e, 0x17, 0);
if(window == NULL)
    od_exit(10);

/* Display and input operations within the window go here. */

od_window_remove(window);
```

## See also

[`od_window_remove()`](od_window_remove.md),
[`od_draw_box()`](od_draw_box.md),
[`od_gettext()`](od_gettext.md),
[`od_puttext()`](od_puttext.md)
