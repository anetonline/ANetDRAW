# `od_set_user_8bit()`

Records whether the caller's connection supports eight-bit character data.

## Synopsis

```c
BOOL od_set_user_8bit(BOOL bEightBit);
```

## Return value

Returns [`TRUE`](../constants/general.md#true) when the setting was recorded.
Returns [`FALSE`](../constants/general.md#false) when the session has already
completed.

## Description

A zero `bEightBit` records a seven-bit path; any nonzero value records an
eight-bit path. The function may be called before or after
[`od_init()`](od_init.md). A framing field in a DORINFO, CHAIN.TXT, or
GAP-style DOOR.SYS file replaces a value set before initialization. Other
drop-file formats leave an explicitly selected value unchanged.

Use this function when application configuration or terminal negotiation has
more accurate information than the selected drop file. The setting is
independent of [`od_control.user_ansi`](../control/caller.md#user_ansi).

This function does not initialize OpenDoors.

## Example

```c
od_init();
if(terminal_supports_cp437)
   od_set_user_8bit(TRUE);
```

## See also

[`od_get_user_8bit()`](od_get_user_8bit.md),
[`od_init()`](od_init.md),
[`od_control.user_ansi`](../control/caller.md#user_ansi)
