# `od_add_personality()`

Registers a status-line and sysop-function-key personality.

## Synopsis

```c
BOOL od_add_personality(const char *pszName, BYTE btOutputTop,
    BYTE btOutputBottom, OD_PERSONALITY_PROC *pfPerFunc);
```

## Description

[`od_add_personality()`](od_add_personality.md) adds a personality to the set
which can be selected by name. Personalities control the text-mode local status line,
the portion of the local display assigned to door output, and the handling of
personality operations. They do not change the remote terminal protocol.

The function must be called before [`od_init()`](od_init.md), or before any
other OpenDoors function which can initialize the library. It does not itself
initialize OpenDoors.

`pszName` points to the name by which the personality will be selected. Names
are case-insensitive. OpenDoors copies, converts to upper case, and retains the
first 32 characters; the caller may therefore release or reuse the source
string after this function returns. The name can subsequently be supplied to
[`od_set_personality()`](od_set_personality.md), the `Personality`
configuration-file keyword, or the `-PERSONALITY` command-line option.

`btOutputTop` and `btOutputBottom` specify the one-based first and last local
screen rows assigned to ordinary door output while the personality is active.
For example, values of 1 and 23 reserve rows 24 and 25 for the status display.
The personality callback receives the
[`PEROP_*`](../constants/components.md#personality-procedure-operations)
operations used to initialize, terminate, display, update, and report custom
hot keys. Its address is retained and the callback must remain available for
as long as the personality can be selected.

The current interface does not validate a null name, a null callback, or the
order and range of the output rows. Supplying any of them incorrectly results
in undefined behavior or an unusable local display.

This function is supported by DOS, DOS32, and Windows console builds. Other builds
return [`FALSE`](../constants/general.md#false) and set
[`od_control.od_error`](../control/runtime.md#od_error) to
[`ERR_UNSUPPORTED`](../constants/errors.md#err_unsupported).

## Return value

The function returns [`TRUE`](../constants/general.md#true) when the
personality has been registered. OpenDoors can hold twelve personalities,
including its five built-in personalities. Seven slots remain available for
custom personalities. If the table is full, the function
returns [`FALSE`](../constants/general.md#false) and sets
[`od_control.od_error`](../control/runtime.md#od_error) to
[`ERR_LIMIT`](../constants/errors.md#err_limit).

## See also

[`od_set_personality()`](od_set_personality.md),
[`od_set_statusline()`](od_set_statusline.md),
[Personality modules](../../guides/personalities.md),
[Types and callbacks](../types.md)
