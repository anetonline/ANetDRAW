# `ODScrnDisplayChar()`

Writes one character to the DOS local screen at the current cursor position.

## Synopsis

```c
void ODScrnDisplayChar(unsigned char chToOutput);
```

The character uses the current local attribute and advances the local cursor.
It is not sent to the remote caller.
