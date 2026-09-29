# OpenDoors Reference

OpenDoors is a C and C++ toolkit for writing online software such as BBS doors.
It provides the connection handling, terminal output, keyboard input,
configuration, status display, and door-information support that would otherwise
need to be built into every program.

This site documents the current OpenDoors 6.30 (release 6.3.0) source tree.
The [versioning guide](guides/versioning.md) explains the two notations. The function
reference describes the interface declared by
[`OpenDoor.h`](reference/api/index.md); DOS personality modules additionally
use [`ODStat.h`](reference/personality/index.md). The accompanying guides explain how
those functions fit together in a working door.

If you are writing your first OpenDoors program, begin with [Getting
started](getting-started.md). If you already know which operation you need,
proceed directly to the [API reference](reference/api/index.md).

## What is covered

- All high-level [`od_*`](reference/api/index.md) API functions declared by
  [`OpenDoor.h`](reference/api/index.md).
- Every field of the [`od_control`](reference/control/index.md) structure.
- Every public data type, callback, constant, macro, error value, and
  compatibility definition.
- Current native, Windows, and 16-bit or 32-bit DOS build procedures.
- The relationship between the remote terminal and the local OpenDoors
  display.

The lower-level `ODScrn`, `ODCom`, and kernel module interfaces are library
implementation interfaces. They are not part of this high-level reference.
