# `od_send_file()`

Displays an ASCII, ANSI, AVATAR, or RIP file.

## Synopsis

```c
BOOL od_send_file(const char *pszFileName);
```

## Description

[`od_send_file()`](od_send_file.md) opens a display file, sends it to the
remote user, and maintains the OpenDoors local or virtual screen. It can be
used for menus, welcome and logoff screens, instructions, or any other
presentation kept outside the program.

`pszFileName` may name one particular file or a base name from which OpenDoors
selects a terminal-specific variant.

### Automatic file selection

When the final component of `pszFileName` contains no period, OpenDoors tries
the following extensions in order, skipping formats whose capability flag is
not enabled. Periods in preceding directory components do not affect this
selection:

| Priority | Extension | Required caller capability |
| ---: | --- | --- |
| 1 | `.rip` | [`od_control.user_rip`](../control/caller.md#user_rip) |
| 2 | `.avt` | [`od_control.user_avatar`](../control/caller.md#user_avatar) |
| 3 | `.ans` | [`od_control.user_ansi`](../control/caller.md#user_ansi) |
| 4 | `.asc` | None |

For example:

```c
od_send_file("MAINMENU");
```

may open `MAINMENU.rip`, `MAINMENU.avt`, `MAINMENU.ans`, or `MAINMENU.asc`.
The search stops at the first compatible file which can be opened. AVATAR
capability does not by itself enable the ANSI candidate, and RIP capability
does not by itself enable AVATAR or ANSI; each fallback is considered only
when its own control-structure flag is true. The ASCII candidate is always
considered.

OpenDoors constructs automatic candidates in an internal 1,025-byte buffer.
The base path, four-character extension, and terminating null byte must fit in
that buffer. A longer automatic base path is rejected before any candidate is
opened. This internal limit does not apply to an explicit filename, which is
passed directly to the C runtime.

If the selected remote file is RIP, OpenDoors performs a second search,
beginning with `.avt`, for a version which can be shown on the local screen.
It uses that companion file for local presentation while sending the RIP file
unchanged to the remote user. If no companion can be opened, it displays the
message beginning with
[`od_control.od_sending_rip`](../control/customization.md#od_sending_rip) on
the local interface until remote transmission has drained.

### Explicit filenames

If the final path component contains a period, OpenDoors treats it as an
explicit filename and opens exactly that path; it does not perform extension
fallback. Thus:

```c
od_send_file("MAINMENU.ANS");
```

opens only `MAINMENU.ANS`.

OpenDoors first opens an explicit path exactly as supplied. After that open
succeeds, a final extension which is exactly `.rip`, without regard to the case
of those three letters, classifies the opened file as RIP data. Page pausing
and local emulation are then disabled, and the local RIP-transmission message
is displayed. Names ending in `.ripple` or `.rip.txt` are not classified as
RIP. Unlike automatic mode, this path does not search for a companion local
file.

For every non-RIP file, content rather than the filename extension determines
which ANSI and AVATAR commands are interpreted for the local or virtual
screen. An explicitly named file with another extension can therefore contain
the same supported terminal commands.

## Paging and interruption

Page pausing begins in the state reported by
[`od_control.od_page_pausing`](../control/runtime.md#od_page_pausing) and uses
[`od_control.user_screen_length`](../control/caller.md#user_screen_length) as
the page length. OpenDoors counts records returned by the C stream reader which
end in carriage return or line feed. At the page boundary it uses the standard
continue prompt, allowing the caller to continue, disable further pauses, or
stop. Pausing is always disabled for RIP transmission.

When [`od_control.od_list_pause`](../control/customization.md#od_list_pause) is
enabled, `P` pauses display until another key is pressed. When
[`od_control.od_list_stop`](../control/customization.md#od_list_stop) is
enabled, `S`, Ctrl-C, Ctrl-K, or Ctrl-X stops display. These checks accept both
upper- and lower-case letters. Stopping an open file is a successful,
user-requested termination and does not make the function return
[`FALSE`](../constants/general.md#false).

When [`od_control.od_emu_simulate_modem`](../control/customization.md#od_emu_simulate_modem)
is true, local emulation is paced according to the reported connection speed
rather than being completed immediately.

## RemoteAccess and QuickBBS substitutions

Unless [`od_control.od_no_ra_codes`](../control/customization.md#od_no_ra_codes)
is true, the emulator recognizes the established RemoteAccess/QuickBBS prefix
bytes Ctrl-A (`0x01`), Ctrl-F (`0x06`), and Ctrl-K (`0x0b`). Ctrl-A waits for
Enter. Ctrl-F and Ctrl-K consume the following byte as a case-sensitive
substitution code. The code letters shown below are uppercase; an unlisted or
lower-case code is consumed without output. Numeric values are rendered in
unpadded base-10 notation unless an entry specifies literal text.

The implemented Ctrl-F substitutions are:

| Code | Output |
| --- | --- |
| `^FA` | [`od_control.user_name`](../control/caller.md#user_name) |
| `^FB` | [`od_control.user_location`](../control/caller.md#user_location) |
| `^FC` | [`od_control.user_password`](../control/caller.md#user_password) |
| `^FD` | [`od_control.user_dataphone`](../control/caller.md#user_dataphone) |
| `^FE` | [`od_control.user_homephone`](../control/caller.md#user_homephone) |
| `^FF` | [`od_control.user_lastdate`](../control/caller.md#user_lastdate) |
| `^FG` | [`od_control.user_lasttime`](../control/caller.md#user_lasttime) |
| `^FH` through `^FK` | Eight `X` or `-` characters representing [`od_control.user_flags[0]`](../control/caller.md#user_flags) through `user_flags[3]`, least-significant bit first |
| `^FL` | Decimal [`od_control.user_net_credit`](../control/caller.md#user_net_credit) |
| `^FM` | Decimal [`od_control.user_messages`](../control/caller.md#user_messages) |
| `^FN` | Decimal [`od_control.user_lastread`](../control/caller.md#user_lastread) |
| `^FO` | Decimal [`od_control.user_security`](../control/caller.md#user_security) |
| `^FP` | Decimal [`od_control.user_numcalls`](../control/caller.md#user_numcalls) |
| `^FQ` | Decimal [`od_control.user_uploads`](../control/caller.md#user_uploads) |
| `^FR` | Decimal [`od_control.user_upk`](../control/caller.md#user_upk) |
| `^FS` | Decimal [`od_control.user_downloads`](../control/caller.md#user_downloads) |
| `^FT` | Decimal [`od_control.user_downk`](../control/caller.md#user_downk) |
| `^FU` | Decimal [`od_control.user_time_used`](../control/caller.md#user_time_used) |
| `^FV` | Decimal [`od_control.user_screen_length`](../control/caller.md#user_screen_length) |
| `^FW` | First space-delimited word of [`od_control.user_name`](../control/caller.md#user_name) |
| `^FX` | `ON` when [`od_control.user_ansi`](../control/caller.md#user_ansi) is true; otherwise `OFF` |
| `^FY` | `ON` when bit `0x04` of [`od_control.user_attribute`](../control/caller.md#user_attribute) is set; otherwise `OFF` |
| `^FZ` | `ON` when bit `0x02` of [`od_control.user_attribute`](../control/caller.md#user_attribute) is set; otherwise `OFF` |
| `^F0` | `ON` when bit `0x40` of [`od_control.user_attribute`](../control/caller.md#user_attribute) is set; otherwise `OFF` |
| `^F1` | `ON` when bit `0x80` of [`od_control.user_attribute`](../control/caller.md#user_attribute) is set; otherwise `OFF` |
| `^F2` | `ON` when bit `0x01` of [`od_control.user_attrib2`](../control/caller.md#user_attrib2) is set; otherwise `OFF` |
| `^F3` | [`od_control.user_handle`](../control/caller.md#user_handle) |
| `^F4` | [`od_control.user_firstcall`](../control/caller.md#user_firstcall) |
| `^F5` | [`od_control.user_birthday`](../control/caller.md#user_birthday) |
| `^F6` | [`od_control.user_subdate`](../control/caller.md#user_subdate) |
| `^F8` | `ON` when bit `0x02` of [`od_control.user_attrib2`](../control/caller.md#user_attrib2) is set; otherwise `OFF` |
| `^F9` | Decimal [`od_control.user_uploads`](../control/caller.md#user_uploads), a colon, and decimal [`od_control.user_downloads`](../control/caller.md#user_downloads) |
| `^F:` | Decimal [`od_control.user_upk`](../control/caller.md#user_upk), a colon, and decimal [`od_control.user_downk`](../control/caller.md#user_downk) |
| `^F;` | `ON` when bit `0x04` of [`od_control.user_attrib2`](../control/caller.md#user_attrib2) is set; otherwise `OFF` |

All other Ctrl-F substitution codes are consumed without output.

The implemented Ctrl-K substitutions are:

| Code | Output or action |
| --- | --- |
| `^KA` | Decimal [`od_control.system_calls`](../control/caller.md#system_calls) |
| `^KB` | [`od_control.system_last_caller`](../control/caller.md#system_last_caller) |
| `^KM` | Decimal [`od_control.user_time_used`](../control/caller.md#user_time_used) |
| `^KN` | The literal text `00` |
| `^KO` | Decimal [`od_control.user_timelimit`](../control/caller.md#user_timelimit) |
| `^KQ` | The literal character `0` |
| `^KR` | The literal character `0` |
| `^KV` | [`od_control.event_starttime`](../control/caller.md#event_starttime), written only to the local screen by the current implementation |
| `^KX` | Calls [`od_exit()`](od_exit.md) with error level 2 and hangup enabled |

All other Ctrl-K substitution codes are consumed without output.

When [`od_control.od_no_ra_codes`](../control/customization.md#od_no_ra_codes)
is true, these prefixes are not interpreted as substitutions and proceed
through ordinary terminal emulation instead.

## Return value

The function returns [`TRUE`](../constants/general.md#true) after an opened
file reaches end of file or the user stops it. It returns
[`FALSE`](../constants/general.md#false) and sets
[`od_control.od_error`](../control/runtime.md#od_error) to
[`ERR_PARAMETER`](../constants/errors.md#err_parameter) for a null filename,
or to [`ERR_FILEOPEN`](../constants/errors.md#err_fileopen) when the explicit
file or every compatible automatic candidate cannot be opened. An automatic
base path too long for internal extension construction returns
[`FALSE`](../constants/general.md#false) and
sets [`od_control.od_error`](../control/runtime.md#od_error) to
[`ERR_LIMIT`](../constants/errors.md#err_limit).

The implementation reads through `fgets()` and treats any failed read as end
of file. It does not distinguish a stream read error from normal end of file
and does not set [`ERR_FILEREAD`](../constants/errors.md#err_fileread).

## See also

[`od_send_file_section()`](od_send_file_section.md),
[`od_hotkey_menu()`](od_hotkey_menu.md),
[`od_disp_emu()`](od_disp_emu.md),
[`od_list_files()`](od_list_files.md)
