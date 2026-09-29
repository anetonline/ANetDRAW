# `od_disp()`

Sends an exact number of bytes with optional local display

## Synopsis

```c
void od_disp(const char *pachBuffer, INT nSize, BOOL bLocalEcho);
```

## Return value

N/A

## Description

[`od_disp()`](od_disp.md) is the lowest-level OpenDoors buffer-output function. It sends
exactly `nSize` bytes beginning at `pachBuffer` to the caller when OpenDoors is
operating remotely. The buffer need not end with a null byte and may contain
embedded null bytes.

If `bLocalEcho` is [`TRUE`](../constants/general.md#true), the same bytes are
also applied to the OpenDoors screen and its local presentation. If it is
[`FALSE`](../constants/general.md#false), the bytes are sent only to the remote
communications connection. In local mode there is no remote connection, so a
false `bLocalEcho` produces no output.

The application must pass a nonnegative size and a pointer which addresses at
least that many readable bytes. The function does not validate the buffer,
report communications errors, or return a byte count.

No C formatting, OpenDoors color-name processing, or terminal emulation is
performed. When local echo is enabled, printable bytes and ordinary control
characters update the screen model, but ANSI, AVATAR, and RIP command streams
are not interpreted. Use [`od_disp_emu()`](od_disp_emu.md) when encoded
terminal output must have the same interpreted effect on the OpenDoors screen
as it has on the caller's terminal.

The explicit byte count makes [`od_disp()`](od_disp.md) useful for buffers containing null
characters. Its ability to suppress local echo is also required for modem
commands and for library code which has already updated the screen model and
must transmit only the corresponding terminal command.

## Examples

Display one character on both screens:

```c
od_disp(&character, 1, TRUE);
```

Send a null-terminated modem command without displaying it locally:

```c
od_disp(command, (INT)strlen(command), FALSE);
```

Send exactly five bytes and display those bytes locally:

```c
od_disp(buffer, 5, TRUE);
```

## See also

[`od_disp_str()`](od_disp_str.md), [`od_disp_emu()`](od_disp_emu.md),
[`od_printf()`](od_printf.md), [`od_putch()`](od_putch.md),
[`od_repeat()`](od_repeat.md)
