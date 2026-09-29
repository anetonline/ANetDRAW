# `od_get_user_8bit()`

Reports whether the caller's connection supports eight-bit character data.

## Synopsis

```c
BOOL od_get_user_8bit(void);
```

## Return value

Returns [`TRUE`](../constants/general.md#true) when the current caller can
receive all eight bits of each character, or
[`FALSE`](../constants/general.md#false) otherwise.

## Description

The value begins false. During [`od_init()`](od_init.md), OpenDoors derives it
from framing fields in DORINFO, CHAIN.TXT, and GAP-style DOOR.SYS files when
one of those formats is selected. The setting is independent of
[`od_control.user_ansi`](../control/caller.md#user_ansi): an eight-bit path
does not necessarily interpret ANSI control sequences, and an ANSI terminal
may be reached through a seven-bit path.

Applications may override the detected value with
[`od_set_user_8bit()`](od_set_user_8bit.md). The setting is exposed through
functions rather than `tODControl` so this addition does not change the public
structure's ABI.

This function does not initialize OpenDoors. Calling it after the session has
completed follows the ordinary public-entry lifecycle rules and returns
[`FALSE`](../constants/general.md#false).

## See also

[`od_set_user_8bit()`](od_set_user_8bit.md),
[`od_init()`](od_init.md),
[`od_control.user_ansi`](../control/caller.md#user_ansi)
