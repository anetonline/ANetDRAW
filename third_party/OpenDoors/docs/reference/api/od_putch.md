# `od_putch()`

Displays one character

## Synopsis

```c
void od_putch(char chToDisplay);
```

## Return value

N/A

## Description

[`od_putch()`](od_putch.md) applies `chToDisplay` to the current OpenDoors screen and, in a
remote session, sends the same byte to the caller. Printable characters are
displayed at the current cursor position using the current attribute. The
cursor then advances; output at the end of a row wraps, and output at the end
of the active window scrolls when scrolling is enabled.

The following control characters have screen-control meanings:

| Character | Effect |
| --- | --- |
| `\r` | Move the cursor to the first column of the current row. |
| `\n` | Move the cursor down one row, scrolling at the bottom of the active window. |
| `\b` | Move the cursor left one column unless it is already at the first column. |
| `\t` | Advance to the next eight-column tab stop, wrapping or scrolling if necessary. |
| `\a` | Ring the local bell and transmit the bell byte in remote mode. |

Other byte values are treated as display characters. In ANSI, AVATAR, or RIP
mode they use the current color attribute; in plain-ASCII mode no color can be
transmitted.

The function periodically gives the OpenDoors kernel an opportunity to run.
It does not report communications or display errors and returns no value.

## Example

The following wrapper reads one key and echoes printable ASCII characters:

```c
char get_key_with_echo(BOOL wait)
{
   char pressed;

   pressed = od_get_key(wait);
   if(pressed >= 32 && pressed <= 126)
      od_putch(pressed);

   return pressed;
}
```

## See also

[`od_disp_str()`](od_disp_str.md), [`od_disp()`](od_disp.md),
[`od_repeat()`](od_repeat.md), [`od_set_attrib()`](od_set_attrib.md),
[`od_get_key()`](od_get_key.md)
