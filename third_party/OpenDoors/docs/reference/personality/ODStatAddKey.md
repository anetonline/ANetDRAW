# `ODStatAddKey()`

Adds a custom local sysop key for the current personality.

## Synopsis

```c
void ODStatAddKey(WORD wKeyCode);
```

`wKeyCode` is an IBM scan-code/ASCII-code key value. OpenDoors can hold up to
16 custom keys; this legacy procedure does not report overflow. Handle the key
under [`PEROP_CUSTOMKEY`](../constants/components.md#personality-procedure-operations)
and remove it with
[`ODStatRemoveKey()`](ODStatRemoveKey.md) during deinitialization.
