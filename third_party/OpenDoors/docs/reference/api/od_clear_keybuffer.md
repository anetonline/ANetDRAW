# `od_clear_keybuffer()`

Discards pending local and remote input

## Synopsis

```c
void od_clear_keybuffer(void);
```

## Return value

N/A

## Description

OpenDoors maintains a common type-ahead queue for caller input and ordinary
keys entered at the local console. This permits a caller to begin entering a
command before a menu or other output has finished. The queue allocates the
number of ring entries configured by
[`od_control.od_in_buf_size`](../control/customization.md#od_in_buf_size), with
one entry reserved to distinguish full and empty states. When that field is
zero, OpenDoors allocates the default 256 entries, providing room for 255
events.

[`od_clear_keybuffer()`](od_clear_keybuffer.md) first gives the OpenDoors kernel an opportunity to move
newly received input into the common queue. It then empties that queue. In a
remote session, OpenDoors also discards pending contents from any distinct
inbound buffer maintained by the active communications method.

Only input which arrived before the buffers were cleared is discarded. Keys
which arrive afterward remain available to [`od_get_key()`](od_get_key.md),
[`od_get_input()`](od_get_input.md), and the higher-level input functions.

Clearing type-ahead is useful after an operation during which input could not
be meaningful. It should not be done automatically before every prompt,
because intentional type-ahead is an important convenience for experienced
callers.

## Example

```c
void wait_for_return(void)
{
   od_disp_str("Please press [Enter] to continue...\n\r");
   od_clear_keybuffer();
   while(od_get_key(TRUE) != '\r')
      ;
}
```

## See also

[`od_key_pending()`](od_key_pending.md), [`od_get_key()`](od_get_key.md),
[`od_get_input()`](od_get_input.md)
