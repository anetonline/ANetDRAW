# `od_window_remove()`

Removes a popup window and restores the screen cells beneath it.

## Synopsis

```c
BOOL od_window_remove(void *pWinInfo);
```

## Description

`pWinInfo` must be the still-owned opaque handle returned by a successful call
to [`od_window_create()`](od_window_create.md). OpenDoors restores the exact
character and attribute cells saved when that window was created, then frees
the handle. The handle is consumed whether restoration succeeds or fails and
must not be passed to this function again.

Overlapping windows must be removed in reverse creation order. If window A is
created before overlapping window B, removing A first restores cells which B
has replaced and damages B's display. Removing B and then A restores each
saved layer in the expected order.

The legacy interface can recognize a null pointer, but cannot determine
whether a non-null pointer is stale or belongs to another allocation. Passing
anything other than a currently owned window handle has undefined behavior.

## Return value

The function returns [`TRUE`](../constants/general.md#true) after restoring the
saved rectangle and releasing the handle. A null handle returns
[`FALSE`](../constants/general.md#false) and sets
[`od_control.od_error`](../control/runtime.md#od_error) to
[`ERR_PARAMETER`](../constants/errors.md#err_parameter).

If [`od_puttext()`](od_puttext.md) cannot restore the rectangle, the function
still releases the handle, returns [`FALSE`](../constants/general.md#false),
and preserves the error recorded by [`od_puttext()`](od_puttext.md).

## See also

[`od_window_create()`](od_window_create.md),
[`od_popup_menu()`](od_popup_menu.md),
[`od_puttext()`](od_puttext.md)
