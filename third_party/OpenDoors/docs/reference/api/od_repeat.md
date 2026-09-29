# `od_repeat()`

Displays one character repeatedly

## Synopsis

```c
void od_repeat(char chValue, BYTE btTimes);
```

## Return value

N/A

## Description

[`od_repeat()`](od_repeat.md) displays `chValue` exactly `btTimes` times on the current
OpenDoors screen and, in a remote session, on the caller's terminal. A count
of zero has no effect. Since `btTimes` is a [`BYTE`](../types.md#byte), one
call can request from 0 through 255 copies.

The visible result is the same as repeated calls to
[`od_putch()`](od_putch.md). The complete expanded sequence is applied to the
virtual session screen or local display, including normal cursor movement,
wrapping, and scrolling. In AVATAR mode OpenDoors transmits the terminal's
three-byte repeat-character command instead of sending every copy separately.
Other terminal modes receive the expanded sequence.

The repeated character normally should be a printable display character.
Control characters are interpreted by the OpenDoors screen one copy at a
time, while a remote terminal's handling of an AVATAR repeat command
containing a control value is terminal-dependent.

The function does not report communications or display errors and returns no
value.

## Example

This simplified box-drawing function demonstrates repeated horizontal and
blank cells. It requires ANSI, AVATAR, or RIP cursor positioning:

```c
void draw_window(INT left, INT top, INT right, INT bottom)
{
   INT row;
   BYTE inside_width;

   inside_width = (BYTE)(right - left - 1);

   od_set_cursor(top, left);
   od_putch((char)218);
   od_repeat((char)196, inside_width);
   od_putch((char)191);

   for(row = top + 1; row < bottom; ++row)
   {
      od_set_cursor(row, left);
      od_putch((char)179);
      od_repeat(' ', inside_width);
      od_putch((char)179);
   }

   od_set_cursor(bottom, left);
   od_putch((char)192);
   od_repeat((char)196, inside_width);
   od_putch((char)217);
}
```

[`od_draw_box()`](od_draw_box.md) provides the complete, configurable version
of this operation and can use still more efficient AVATAR commands.

## See also

[`od_putch()`](od_putch.md), [`od_disp_str()`](od_disp_str.md),
[`od_draw_box()`](od_draw_box.md)
