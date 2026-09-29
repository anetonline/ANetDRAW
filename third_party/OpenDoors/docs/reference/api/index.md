# API function reference

The high-level OpenDoors interface consists of the `od_*` functions declared in
`OpenDoor.h`. The functions are grouped here by the task they perform.

Most functions initialize OpenDoors automatically if necessary. Functions which
must run before initialization say so explicitly. Unless a page states
otherwise, failures place an [`ERR_*`](../constants/errors.md) value in
[`od_control.od_error`](../control/runtime.md).

After [`od_exit()`](od_exit.md) completes, the library is permanently terminal
and rejects every later function call with
[`ERR_GENERALFAILURE`](../constants/errors.md#err_generalfailure). A host using
[`od_noexit`](../control/customization.md#od_noexit) may continue without
OpenDoors and may directly inspect the exported
[`od_control`](../control/index.md) object, but it cannot start a second session.

## Function groups

- **Text display** sends ordinary, formatted, repeated, raw, or emulated text.
- **Color and cursor control** changes display attributes and cursor position.
- **Screen manipulation** clears or saves the complete screen.
- **Block manipulation** reads, writes, clears, or scrolls rectangular areas.
- **Windows and menus** provides boxes, popup windows, and menu selection.
- **File display** selects and sends terminal-appropriate display files.
- **Input** reads keys, events, strings, and multi-line text.
- **Door activity and command lines** handles paging, child processes, logs,
  and arguments.
- **Special control** initializes, services, configures, and ends a session.

OpenDoors does not serialize API or ABI access and does not require calls to
remain on the thread which called [`od_init()`](od_init.md). A multithreaded
application must protect all such access with one application lock.
