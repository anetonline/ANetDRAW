# `od_emulate()`

Processes and displays one terminal-emulation character

## Synopsis

```c
void od_emulate(char chToEmulate);
```

## Return value

N/A

## Description

The [`od_emulate()`](od_emulate.md) function processes a single character through the same
terminal-emulation system used by [`od_disp_emu()`](od_disp_emu.md). The
result is displayed locally and, when the door is operating in remote mode,
sent to the remote user. Successive calls retain the state of an incomplete
ANSI, AVATAR or RemoteAccess/QBBS control sequence, allowing a complete stream
to be supplied one character at a time.

Ordinary display characters are written at the current cursor position. ANSI
and AVATAR control sequences alter the cursor, attributes and screen contents
recognized by the OpenDoors emulator. Unless
[`od_control.od_no_ra_codes`](../control/customization.md#od_no_ra_codes) is
set, supported RemoteAccess/QBBS control codes are interpreted as well. Those
codes may pause for input or substitute caller and system information from
[`od_control`](../control/index.md) before producing remote output.

The character is processed as part of a nul-terminated one-character buffer.
Consequently, passing `\0` produces no output and cannot be used to place a nul
byte in the emulated stream. The function does not report unsupported or
incomplete terminal sequences; an incomplete sequence remains pending for a
later call.

For more than one character, [`od_disp_emu()`](od_disp_emu.md) is normally more
convenient and avoids the overhead of one API call per byte. Use
[`od_disp()`](od_disp.md) instead when bytes are to be transmitted without
terminal interpretation.

## See also

[`od_disp_emu()`](od_disp_emu.md), [`od_disp()`](od_disp.md),
[`od_putch()`](od_putch.md)
