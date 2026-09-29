# `od_reserve_configure()`

Selects a shared filesystem registry for named reservations.

## Synopsis

```c
BOOL od_reserve_configure(const char *pszPath);
```

## Description

Call [`od_reserve_configure()`](od_reserve_configure.md) before
[`od_init()`](od_init.md), using the same path for every cooperating node.
Initialization registers the current `NodeX` identity, or `Local` for a
nodeless session, and rejects a simultaneously active duplicate identity.

The registry uses fixed-size little-endian records and operating-system byte
range locks. The first session for an identity appends a record; later sessions
with that identity reacquire and reuse it. A session writes only that record's
payload and exclusively owns its lifetime lock. Failure to acquire an existing
record for the same identity makes initialization fail rather than creating a
duplicate. The filesystem must make byte-range locks and file writes coherent
between all participating hosts.
On DOS, the redirector or `SHARE.EXE` must support those operations for
multi-node use. When DOS has no record-locking service, reservations remain
usable but uncontended for a non-shared, single-node installation.

## Return value

The function returns [`TRUE`](../constants/general.md#true) after copying a
nonempty path. It returns [`FALSE`](../constants/general.md#false) after
initialization, for an empty or null path, or when memory cannot be allocated.

## See also

[`od_reserve_request()`](od_reserve_request.md),
[`od_init()`](od_init.md), [Multi-node operation](../../guides/multinode.md)
