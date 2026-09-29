# `od_save_screen_ex()`

Saves the complete current screen and its display state in a size-aware
snapshot.

## Synopsis

```c
BOOL od_save_screen_ex(void *pBuffer, DWORD dwBufferSize);
```

## Parameters

`pBuffer`
: Pointer to writable storage for the snapshot. Allocate at least the number of
  bytes returned by [`od_save_screen_size()`](od_save_screen_size.md).

`dwBufferSize`
: Total number of writable bytes available at `pBuffer`.

## Return value

Returns [`TRUE`](../constants/general.md#true) when the complete snapshot has
been written. Returns [`FALSE`](../constants/general.md#false) when the screen is
unavailable, a parameter is invalid, or the supplied buffer is too small.

## Description

[`od_save_screen_ex()`](od_save_screen_ex.md) saves the whole screen used by the current OpenDoors
session. Each text cell's character and IBM-PC attribute is retained. The
snapshot also records the active window boundaries, cursor position, current
display attribute, and a scrolling flag, so that
[`od_restore_screen_ex()`](od_restore_screen_ex.md) can restore the display
environment as well as its visible text.

For a virtual session screen, the scrolling flag is the actual current virtual
screen setting. The legacy local-screen interface has no corresponding query,
so a snapshot taken without a virtual session screen records scrolling as
enabled rather than discovering the previous local setting.

In a remote session, the source is the virtual session screen. Its dimensions
match the caller's configured screen rather than the local console, so a
132-column or long-screen caller can be saved without truncation. Direct local
console rendering is not copied back into this virtual screen; only display
operations performed through OpenDoors are part of the remote snapshot. In
local mode, the function reads the active local text screen.

The normal sequence is:

1. Call [`od_save_screen_size()`](od_save_screen_size.md).
2. Allocate at least that many bytes.
3. Call [`od_save_screen_ex()`](od_save_screen_ex.md) with the allocation and its size.
4. Keep the buffer unchanged until it is passed to
   [`od_restore_screen_ex()`](od_restore_screen_ex.md).
5. Free the buffer when it is no longer needed.

A buffer larger than required is accepted; OpenDoors writes only the current
snapshot. A buffer even one byte smaller than required is rejected, and no
partial snapshot should be used. Because screen dimensions can change, an old
size must not be assumed to remain sufficient after a terminal resize or a new
session initialization.

The saved data is an opaque, versioned binary format. It may contain zero bytes
and is not a C string. Do not modify its header or cell data, translate its
character encoding, write a terminator into it, or reconstruct it from a
private structure definition. The snapshot can be stored temporarily by the
application, but portability between different OpenDoors versions is not a
promised file-format interface.

Saving has no visible effect. It does not clear or redraw either screen, move
the cursor, or send data to the remote user. The function initializes
OpenDoors if necessary.

This interface is distinct from [`od_save_screen()`](od_save_screen.md). The
older function retains its fixed 80-column, at-most-25-row format and its buffer
cannot be passed to [`od_restore_screen_ex()`](od_restore_screen_ex.md). Likewise, a size-aware snapshot
cannot be passed to [`od_restore_screen()`](od_restore_screen.md).

## Errors

On failure, [`od_control.od_error`](../control/runtime.md#od_error) is set as
follows:

- [`ERR_PARAMETER`](../constants/errors.md#err_parameter) when `pBuffer` is
  `NULL` or `dwBufferSize` is smaller than the current required size.
- [`ERR_MEMORY`](../constants/errors.md#err_memory) when the virtual screen
  needed for a remote snapshot could not be created.
- [`ERR_LIMIT`](../constants/errors.md#err_limit) when the required snapshot
  size cannot be represented or no more specific screen error is available.

Always test the Boolean return. The contents of the buffer are not a valid
snapshot after a failed call.

## Example

This example displays a temporary screen and then restores the original one:

```c
DWORD size = od_save_screen_size();
void *saved = size != 0 ? malloc((size_t)size) : NULL;

if(saved != NULL && od_save_screen_ex(saved, size))
{
    od_clr_scr();
    od_set_color(L_WHITE, D_BLUE);
    od_set_cursor(1, 1);
    od_disp_str("Temporary display\n\r");
    od_get_key(TRUE);

    if(!od_restore_screen_ex(saved, size))
        od_clr_scr();
}

free(saved);
```

Production code should handle allocation, save, and restore failures in the
manner appropriate to the door rather than silently continuing.

## See also

[`od_save_screen_size()`](od_save_screen_size.md),
[`od_restore_screen_ex()`](od_restore_screen_ex.md),
[`od_save_screen()`](od_save_screen.md), [`od_gettext()`](od_gettext.md)
