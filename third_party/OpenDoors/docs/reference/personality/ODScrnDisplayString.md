# `ODScrnDisplayString()`

Writes a null-terminated string to the DOS local screen.

## Synopsis

```c
void ODScrnDisplayString(const char *pszString);
```

Characters use the current local attribute and advance the local cursor. They
are not sent to the remote caller.
