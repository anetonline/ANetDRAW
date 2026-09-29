# `od_reserve_request()`

Enters the queue for a named filesystem reservation.

## Synopsis

```c
BOOL od_reserve_request(const char *pszName);
```

## Description

The name must contain 1 through 31 printable ASCII characters. Names are
case-sensitive: requests for the same name serialize, while different names
are independent. A session may have only one outstanding request.

The function publishes a bakery ticket. Equal ticket numbers are valid; the
permanent registry-record index breaks the tie, so a participant never needs
to rewrite its ticket or move to the back of the queue.

## Return value

The function returns [`TRUE`](../constants/general.md#true) when the request is
queued. It returns [`FALSE`](../constants/general.md#false) for an invalid name,
an existing request, an unconfigured registry, or a synchronization failure.
A successful request must eventually be completed or cancelled with
[`od_reserve_end()`](od_reserve_end.md).

## See also

[`od_reserve_configure()`](od_reserve_configure.md),
[`od_reserve_wait()`](od_reserve_wait.md),
[`od_reserve_wait_until()`](od_reserve_wait_until.md)
