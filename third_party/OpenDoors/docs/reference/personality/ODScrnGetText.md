# `ODScrnGetText()`

Copies a rectangle from the DOS local screen into a cell buffer.

## Synopsis

```c
BOOL ODScrnGetText(BYTE btLeft, BYTE btTop, BYTE btRight, BYTE btBottom,
    void *pbtBuffer);
```

Coordinates are one-based and inclusive. Cells are copied row by row as a
character byte followed by its attribute byte, so the caller must provide
`2 * (btRight - btLeft + 1) * (btBottom - btTop + 1)` bytes. The procedure
returns true when the complete rectangle is copied.

Restore the buffer with [`ODScrnPutText()`](ODScrnPutText.md).
