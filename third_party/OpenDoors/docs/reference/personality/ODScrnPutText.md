# `ODScrnPutText()`

Copies a cell buffer into a rectangle on the DOS local screen.

## Synopsis

```c
BOOL ODScrnPutText(BYTE btLeft, BYTE btTop, BYTE btRight, BYTE btBottom,
    void *pbtBuffer);
```

Coordinates are one-based and inclusive. The buffer contains row-major pairs
of character and attribute bytes and must contain
`2 * (btRight - btLeft + 1) * (btBottom - btTop + 1)` bytes. The procedure
returns true when the complete rectangle is restored and does not send the
cells to the remote caller.

Create a matching buffer with [`ODScrnGetText()`](ODScrnGetText.md).
