# `ODStatGetUserAge()`

Formats the current caller's age for a personality display.

## Synopsis

```c
void ODStatGetUserAge(char *pszAge);
```

The age is calculated from [`od_control.user_birthday`](../control/caller.md)
when the active drop-file format supplies a recognized date. Otherwise, the
procedure writes `"?"`. The destination must have room for the result and its
terminating null byte.
