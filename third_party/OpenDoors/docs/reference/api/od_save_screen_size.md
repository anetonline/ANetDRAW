# `od_save_screen_size()`

Returns the buffer size required to save the complete current screen.

## Synopsis

```c
DWORD od_save_screen_size(void);
```

## Return value

Returns the number of bytes required by [`od_save_screen_ex()`](od_save_screen_ex.md)
for the current session screen. A return value of 0 indicates that OpenDoors
could not determine or represent the required size; the reason is placed in
[`od_control.od_error`](../control/runtime.md#od_error).

## Description

The original [`od_save_screen()`](od_save_screen.md) and
[`od_restore_screen()`](od_restore_screen.md) interfaces use a fixed historical
buffer format with four state bytes followed by as many as 25 rows of 80
character/attribute pairs. They must retain that format for source and binary
compatibility. [`od_save_screen_size()`](od_save_screen_size.md) is the first step in the size-aware
interface used when the current caller's screen may be wider or taller.

The returned size accounts for every character and IBM-PC text attribute on
the current screen, together with the state needed to restore the active
window, cursor, current attribute, and a scrolling flag. In a remote session,
the calculation uses the dimensions of the virtual session screen. OpenDoors
starts with
[`od_control.user_screenwidth`](../control/caller.md#user_screenwidth) and
[`od_control.user_screen_length`](../control/caller.md#user_screen_length), then
raises either dimension to the platform's minimum OpenDoors output area when
necessary. It is not limited by the dimensions of the machine's local console.
In local mode, the active local text-screen dimensions are used.

Call this function after OpenDoors has been initialized and after the caller's
screen dimensions have reached their intended values. The result is valid for
the dimensions in effect at the time of the call. If the session screen is
later resized, call [`od_save_screen_size()`](od_save_screen_size.md) again before taking another
snapshot.

The snapshot format is opaque. Applications may allocate, retain, and later
return the buffer to OpenDoors, but must not depend on the size of its header,
the ordering of fields, or the representation of individual cells. A snapshot
should be treated as one indivisible binary object.

[`od_save_screen_size()`](od_save_screen_size.md) does not alter the screen, move the cursor, transmit
output, or allocate the caller's buffer. It initializes OpenDoors if necessary,
so initialization settings must be assigned before calling it.

## Errors

When the size cannot be produced, the function returns 0. The error is normally
one of the following:

- [`ERR_MEMORY`](../constants/errors.md#err_memory) if the virtual session
  screen required for a remote session could not be allocated.
- [`ERR_LIMIT`](../constants/errors.md#err_limit) if the screen dimensions
  cannot be represented by the snapshot interface or no more specific error
  is available.

The function does not reserve memory, so a successful nonzero return does not
guarantee that the application's following allocation will succeed.

## Example

The following code allocates a correctly sized buffer and saves the current
screen:

```c
DWORD snapshot_size;
void *snapshot;

snapshot_size = od_save_screen_size();
if(snapshot_size == 0)
{
    /* od_control.od_error describes the failure. */
    od_exit(1, FALSE);
}

snapshot = malloc((size_t)snapshot_size);
if(snapshot == NULL)
    od_exit(1, FALSE);

if(!od_save_screen_ex(snapshot, snapshot_size))
{
    free(snapshot);
    od_exit(1, FALSE);
}

/* The snapshot may now be restored with od_restore_screen_ex(). */
```

On platforms where [`DWORD`](../types.md#dword) can be wider than `size_t`, an
application should also verify that `snapshot_size` can be represented as a
`size_t` before passing it to `malloc()`.

## See also

[`od_save_screen_ex()`](od_save_screen_ex.md),
[`od_restore_screen_ex()`](od_restore_screen_ex.md),
[`od_save_screen()`](od_save_screen.md)
