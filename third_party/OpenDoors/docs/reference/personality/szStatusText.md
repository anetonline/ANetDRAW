# `szStatusText`

Provides the traditional shared personality work buffer.

## Declaration

```c
extern char szStatusText[80];
```

The buffer is shared by the built-in DOS personalities and is not preserved
across callbacks. It holds at most 79 characters followed by a terminating
null byte. Custom personalities may use their own storage instead.
