# `ODScrnSetCursorPos()`

Sets the DOS local-screen cursor position.

## Synopsis

```c
void ODScrnSetCursorPos(BYTE btColumn, BYTE btRow);
```

The column and row are one-based. During a personality display or update
callback, they address the complete 80 by 25 local screen.
