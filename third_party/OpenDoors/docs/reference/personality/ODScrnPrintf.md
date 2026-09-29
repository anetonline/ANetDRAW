# `ODScrnPrintf()`

Formats text on the DOS local screen.

## Synopsis

```c
INT ODScrnPrintf(char *pszFormat, ...);
```

Formatting follows `printf()` conventions. The result is displayed with the
current local attribute and is not sent to the remote caller. The return value
is the formatter's result.
