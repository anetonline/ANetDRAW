# `od_get_time()`

Returns elapsed wall-clock time for the current OpenDoors session.

## Synopsis

```c
void od_get_time(DWORD *pdwSeconds, WORD *pwMilliseconds);
```

## Description

[`od_get_time()`](od_get_time.md) returns a zero-based time measured from the
start of [`od_init()`](od_init.md). `pdwSeconds` receives whole seconds and
`pwMilliseconds` receives the fractional part, from 0 through 999. Either
pointer may be null when that component is not needed, but they may not both
be null.

This is elapsed civil wall-clock time, not process CPU time. It therefore
continues to advance while the machine is suspended when the host clock
accounts for that interval. OpenDoors uses the standard real-time clock on
Unix-like systems, the system file time on Windows, and the DOS date and time
services on DOS. The effective resolution is platform dependent; the DOS
clock normally advances at approximately 55-millisecond intervals.

Host clock corrections which move time forward are reflected immediately. If
a correction moves the host clock backward, OpenDoors clamps the result so a
session time already returned by this function never decreases. The clock
begins again at zero for a new process session; it is not an absolute date or
time-of-day value.

Calling this function initializes OpenDoors if necessary. Calling it after
the session has completed follows the ordinary public-entry lifecycle rules.

## Return value

The function returns no value. Passing two null pointers sets
[`od_control.od_error`](../control/runtime.md#od_error) to
[`ERR_PARAMETER`](../constants/errors.md#err_parameter).

## Example

```c
DWORD seconds;
WORD milliseconds;

od_get_time(&seconds, &milliseconds);
od_printf("Session time: %lu.%03u\r\n",
    (unsigned long)seconds, (unsigned)milliseconds);
```

## See also

[`od_get_input_until()`](od_get_input_until.md), [`od_sleep()`](od_sleep.md),
[`od_init()`](od_init.md)
