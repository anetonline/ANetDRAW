# `ODStatRemoveKey()`

Removes a custom local sysop key.

## Synopsis

```c
void ODStatRemoveKey(WORD wKeyCode);
```

The first matching key previously installed with
[`ODStatAddKey()`](ODStatAddKey.md) is removed. The order of the remaining keys
is not preserved: OpenDoors moves the final active key into the removed slot.
The corresponding [`od_control.od_hot_function`](../control/customization.md#od_hot_function)
callback moves with that key, and both elements of the vacated final slot are
cleared. A missing key has no effect.
