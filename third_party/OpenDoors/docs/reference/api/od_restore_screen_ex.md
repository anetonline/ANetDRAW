# `od_restore_screen_ex()`

Restores a complete screen snapshot created by
[`od_save_screen_ex()`](od_save_screen_ex.md).

## Synopsis

```c
BOOL od_restore_screen_ex(const void *pBuffer, DWORD dwBufferSize);
```

## Parameters

`pBuffer`
: Pointer to an unmodified snapshot produced by
  [`od_save_screen_ex()`](od_save_screen_ex.md).

`dwBufferSize`
: Number of readable bytes available at `pBuffer`. This is normally the size
  originally returned by [`od_save_screen_size()`](od_save_screen_size.md).

## Return value

Returns [`TRUE`](../constants/general.md#true) when the snapshot and its display
state have been restored. Returns [`FALSE`](../constants/general.md#false) if the
buffer is invalid, incomplete, incompatible with the current screen, or cannot
be displayed.

## Description

[`od_restore_screen_ex()`](od_restore_screen_ex.md) restores every character and attribute saved by
[`od_save_screen_ex()`](od_save_screen_ex.md), then restores the saved window
boundaries, cursor position, current attribute, and recorded scrolling flag. It is the
counterpart to the size-aware save interface and is suitable for caller screens
larger than the legacy 80-column, at-most-25-row save area.

The current screen dimensions must exactly match the dimensions recorded in
the snapshot. OpenDoors does not crop, pad, scale, or reflow saved contents.
This rule prevents a snapshot taken before a resize from applying saved window
and cursor coordinates to a differently shaped screen. If the caller's
terminal dimensions change, discard snapshots taken with the previous size.

Before changing the display, OpenDoors validates the snapshot's identifier,
format version, total length, width and height, window boundaries, cursor
coordinates, scrolling value, and reserved fields. It also checks that the
declared snapshot length fits within `dwBufferSize`. A larger
`dwBufferSize` is accepted, but the embedded snapshot length must itself be
correct.

In a remote session with ANSI or AVATAR available, OpenDoors transmits the
saved cells using the block-display path, restores the virtual session-screen
state, and refreshes the visible portion of the local presentation. It then
restores the remote cursor and current attribute. For a remote caller without
cursor-addressable graphics, OpenDoors clears the display and emits a plain
text representation as far as that terminal mode permits; the virtual screen
still receives the complete saved cells and state.

In local mode, the cells and state are restored directly to the active local
text screen. Because a local snapshot records scrolling as enabled, restoration
enables local scrolling even if application code had disabled it before the
save. The function may therefore visibly replace the entire display;
applications should not call it merely to recover a cursor position.

The buffer is read but not changed. A successfully restored snapshot may be
used again while the session dimensions remain compatible. The application
continues to own the buffer and must eventually free it.

Snapshots from [`od_save_screen()`](od_save_screen.md) are not accepted. The
fixed-format save/restore pair and the size-aware pair are separate interfaces
and their buffers are not interchangeable.

Calling this function initializes OpenDoors if necessary. In normal use it is
called in the same initialized session in which the snapshot was taken.

## Errors

On failure, [`od_control.od_error`](../control/runtime.md#od_error) is normally
set to:

- [`ERR_PARAMETER`](../constants/errors.md#err_parameter) for a `NULL` buffer,
  a buffer shorter than the minimum snapshot, an invalid identifier or version,
  inconsistent lengths or dimensions, invalid saved coordinates or state, a
  dimension mismatch, or failure to display otherwise valid saved cells.
- [`ERR_MEMORY`](../constants/errors.md#err_memory) when the virtual session
  screen required for restoration could not be created.
- [`ERR_LIMIT`](../constants/errors.md#err_limit) when the current screen size
  cannot be represented or no more specific screen error is available.

If the function returns false, the application must not assume that a damaged
or externally modified buffer is reusable. Validate the return before
continuing with display operations that depend on the restore.

## Example

The following helper restores a snapshot and releases it regardless of the
result:

```c
static BOOL restore_and_free(void *snapshot, DWORD snapshot_size)
{
    BOOL restored;

    if(snapshot == NULL)
        return FALSE;

    restored = od_restore_screen_ex(snapshot, snapshot_size);
    free(snapshot);
    return restored;
}
```

For nested windows or displays, take and restore snapshots in last-in,
first-out order. Restoring an older full-screen snapshot discards every visible
change made after that snapshot was taken.

## See also

[`od_save_screen_size()`](od_save_screen_size.md),
[`od_save_screen_ex()`](od_save_screen_ex.md),
[`od_restore_screen()`](od_restore_screen.md), [`od_puttext()`](od_puttext.md)
