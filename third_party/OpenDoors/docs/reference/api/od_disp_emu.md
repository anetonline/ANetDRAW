# `od_disp_emu()`

Interprets a null-terminated terminal-emulation stream

## Synopsis

```c
void od_disp_emu(const char *pszToDisplay, BOOL bRemoteEcho);
```

## Return value

N/A

## Description

[`od_disp_emu()`](od_disp_emu.md) passes the null-terminated stream at `pszToDisplay` through
the OpenDoors ANSI, AVATAR, and RemoteAccess/QuickBBS display-code emulator.
Printable text, cursor movement, color changes, screen clearing, scrolling,
and the other supported commands are applied as they are encountered. The
same emulator is used by [`od_send_file()`](od_send_file.md), whose reference
describes the supported display-file behavior in detail.

If `bRemoteEcho` is [`TRUE`](../constants/general.md#true), the resulting output
is sent to the remote caller and its interpreted effect is recorded in the
virtual session screen. The portion of that screen which fits on the local
presentation is refreshed. This is the usual setting for application output.

If `bRemoteEcho` is [`FALSE`](../constants/general.md#false), no bytes are sent to
the caller. The stream is interpreted only on the local emulated display and
does not alter the authoritative virtual screen for a remote session. This
retains the established use of the function for local-only displays.

Unless
[`od_control.od_no_ra_codes`](../control/customization.md#od_no_ra_codes) is
enabled, recognized RemoteAccess and QuickBBS control bytes are interpreted.
They can pause for input or substitute caller and system information before
the translated result is sent. When [`od_no_ra_codes`](../control/customization.md#od_no_ra_codes) is enabled and remote
echo is requested, the original stream is transmitted without that
translation while the terminal emulator still interprets its ANSI and AVATAR
effects for the screen model.

If
[`od_control.od_emu_simulate_modem`](../control/customization.md#od_emu_simulate_modem)
is enabled, interpretation is paced according to the configured connection
speed. Otherwise the complete string is processed as quickly as possible.

Emulator parsing state is retained between calls, so a control sequence may
be supplied in successive strings. `pszToDisplay` must not be `NULL`. Because
the input is null-terminated, an embedded null byte ends processing; use
[`od_disp()`](od_disp.md) for arbitrary binary output. The function does not
report malformed or incomplete terminal sequences and returns no value.

For a display stored in a disk file, [`od_send_file()`](od_send_file.md)
provides extension selection, section handling, pausing, and hotkey support in
addition to terminal emulation. For a stream received one character at a time,
the compatibility function [`od_emulate()`](od_emulate.md) retains the same
emulator state between characters.

## Example

```c
od_disp_emu("\x1b[2J\x1b[1;1H", TRUE);
od_disp_emu("\x1b[1;33mOpenDoors\x1b[0m\n\r", TRUE);
```

## See also

[`od_send_file()`](od_send_file.md), [`od_emulate()`](od_emulate.md),
[`od_disp()`](od_disp.md), [`od_disp_str()`](od_disp_str.md)
