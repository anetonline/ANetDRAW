# DOS personality SDK

`ODStat.h` is the companion to [`OpenDoor.h`](../api/index.md) for writing DOS
status-line and sysop-key personalities. It provides local-only screen output,
status helpers, and the traditional shared work buffer. These interfaces are
not available on Windows or Unix builds.

See [DOS personalities](../../guides/personalities.md) for the callback
lifecycle and a complete description of personality selection.
