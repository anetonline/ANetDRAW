# `od_reserve_end()`

Leaves the current named reservation queue.

## Synopsis

```c
BOOL od_reserve_end(void);
```

## Description

Call [`od_reserve_end()`](od_reserve_end.md) after using an acquired resource.
It may also cancel a request which is still pending. The function clears the
published name and ticket before another request can be made by this session.

## Return value

The function returns [`TRUE`](../constants/general.md#true) when the request is
removed. It returns [`FALSE`](../constants/general.md#false) when there is no
outstanding request or when the registry update fails.

## See also

[`od_reserve_request()`](od_reserve_request.md),
[`od_reserve_wait()`](od_reserve_wait.md),
[Multi-node operation](../../guides/multinode.md)
