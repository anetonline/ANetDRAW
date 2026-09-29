# `od_reserve_wait()`

Waits for a named reservation using a relative timeout.

## Synopsis

```c
tODReserveResult od_reserve_wait(tODMilliSec Milliseconds);
```

## Description

Pass zero to poll without waiting, or
[`OD_NO_TIMEOUT`](../constants/general.md#od_no_timeout) to wait indefinitely.
OpenDoors continues normal cooperative servicing while it waits.

A timeout returns `OD_RESERVE_PENDING` and leaves the request queued. The
caller may wait again or cancel it with
[`od_reserve_end()`](od_reserve_end.md). `OD_RESERVE_ACQUIRED` means the caller
may access the protected resource until it ends the reservation.

Each scan treats ordinary registry or peer-lock contention as pending, so a
zero-time poll does not incur a lock wait and finite waits are not extended by
a separate lock-retry interval. An actual registry I/O or locking failure
returns `OD_RESERVE_ERROR` and detaches reservation synchronization from the
session; normal door operation may continue.

## See also

[`od_reserve_request()`](od_reserve_request.md),
[`od_reserve_wait_until()`](od_reserve_wait_until.md),
[`tODReserveResult`](../types.md#todreserveresult)
