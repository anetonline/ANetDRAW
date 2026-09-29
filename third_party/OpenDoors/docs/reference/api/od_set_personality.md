# `od_set_personality()`

Selects the current status-line and sysop-function-key personality.

## Synopsis

```c
BOOL od_set_personality(const char *pszName);
```

## Description

[`od_set_personality()`](od_set_personality.md) selects the text-mode local-interface
personality identified by `pszName`. Names are compared without regard to case
and only the first 32 characters are significant.

The built-in names are `STANDARD`, `OD_ONEROW`, `REMOTEACCESS`, `WILDCAT`, and
`PCBOARD`.
Additional names may be installed before initialization with
[`od_add_personality()`](od_add_personality.md).

When the selected personality differs from the current one, OpenDoors removes
the existing status line, sends the old personality
[`PEROP_DEINITIALIZE`](../constants/components.md#perop_deinitialize), sends the new
personality
[`PEROP_INITIALIZE`](../constants/components.md#perop_initialize), installs
the new output-screen boundaries, and displays its normal status line. The
function does nothing beyond reporting success when the requested personality
is already active.

To permit selection through the `Personality` configuration keyword or the
`-PERSONALITY` command-line option during initialization, set
[`od_control.od_mps`](../control/customization.md#od_mps) to
[`INCLUDE_MPS`](../constants/components.md#include_mps) before calling
[`od_init()`](od_init.md). A program can call this function directly without
using those configuration interfaces.

Personalities affect only the local operator display and keys. They do not
change the remote user's terminal mode or the contents sent to the remote
connection.

This function is supported by DOS, DOS32, and Windows console builds. Other builds
return [`FALSE`](../constants/general.md#false) and set
[`od_control.od_error`](../control/runtime.md#od_error) to
[`ERR_UNSUPPORTED`](../constants/errors.md#err_unsupported).

`pszName` must point to a nonempty, null-terminated string. An empty string
returns [`FALSE`](../constants/general.md#false) with
[`ERR_PARAMETER`](../constants/errors.md#err_parameter). The legacy interface
does not check for a null pointer.

## Return value

The function returns [`TRUE`](../constants/general.md#true) when the named
personality is selected. If no installed personality has that name, it returns
[`FALSE`](../constants/general.md#false) and sets
[`od_control.od_error`](../control/runtime.md#od_error) to
[`ERR_LIMIT`](../constants/errors.md#err_limit).

## See also

[`od_add_personality()`](od_add_personality.md),
[`od_set_statusline()`](od_set_statusline.md),
[Personality modules](../../guides/personalities.md)
