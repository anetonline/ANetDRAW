# `od_reserve_wait_until()`

Waits for a named reservation until an absolute session-time deadline.

## Synopsis

```c
tODReserveResult od_reserve_wait_until(DWORD dwSeconds,
    WORD wMilliseconds);
```

## Description

The deadline uses the same zero-based session clock returned by
[`od_get_time()`](od_get_time.md); `wMilliseconds` must be from 0 through 999.
If the deadline has already been reached, the function immediately returns
`OD_RESERVE_PENDING` without removing the request from the queue.

The acquired, pending, and error states have the same meanings as for
[`od_reserve_wait()`](od_reserve_wait.md).

## See also

[`od_reserve_request()`](od_reserve_request.md),
[`od_reserve_end()`](od_reserve_end.md),
[`od_get_time()`](od_get_time.md)
