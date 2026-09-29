# `od_disp_str()`

Displays a null-terminated string on the remote and local screens

## Synopsis

```c
void od_disp_str(const char *pszToDisplay);
```

## Return value

N/A

## Description

[`od_disp_str()`](od_disp_str.md) displays every byte preceding the terminating null in
`pszToDisplay`. In a remote session the string is sent to the caller and
applied to OpenDoors' virtual session screen; the portion which fits is also
shown on the local presentation. In local mode it is displayed on the local
screen. `pszToDisplay` must not be `NULL`.

Unlike [`od_printf()`](od_printf.md), this function performs neither C format
substitution nor OpenDoors color-name processing. It is therefore the normal
choice for a complete string, particularly when the string may contain
percent characters supplied by a user. Use [`od_putch()`](od_putch.md) for a
single character, [`od_disp()`](od_disp.md) for a counted buffer or output
without local echo, and [`od_repeat()`](od_repeat.md) for a run of one
character.

Control characters are applied individually to the OpenDoors screen. A
carriage return moves to the beginning of the current row; a line feed moves
down one row and may scroll the active window. OpenDoors does not insert either
character automatically. Existing OpenDoors programs conventionally use
`"\n\r"` when beginning a new line:

```c
od_disp_str("First line\n\r");
od_disp_str("Second line\n\r");
```

ANSI, AVATAR, and RIP escape sequences in the string are transmitted but are
not interpreted by this function for the OpenDoors screen. Pass such a stream
to [`od_disp_emu()`](od_disp_emu.md), or place a complete display in a file and
use [`od_send_file()`](od_send_file.md).

The function does not report communications or display errors and returns no
value.

## Examples

Several calls may be used to construct one line:

```c
od_disp_str("Another ");
od_disp_str("od_disp_str() ");
od_disp_str("example\n\r");
```

A string variable is displayed in the same manner:

```c
char string[80];

strcpy(string, "This is a string!\n\r");
od_disp_str(string);
```

## See also

[`od_printf()`](od_printf.md), [`od_disp()`](od_disp.md),
[`od_disp_emu()`](od_disp_emu.md), [`od_putch()`](od_putch.md),
[`od_repeat()`](od_repeat.md)
