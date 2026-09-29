# `ODScrnDisplayBuffer()`

Writes a counted character buffer to the DOS local screen.

## Synopsis

```c
void ODScrnDisplayBuffer(const char *pBuffer, INT nCharsToDisplay);
```

Exactly `nCharsToDisplay` bytes are written using the current local attribute;
the buffer need not be null-terminated. Output is local only.
