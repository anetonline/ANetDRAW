# The `od_control` structure

`od_control` contains the information and settings associated with the current
OpenDoors session. It is available as an imported global and through
[`od_control_get()`](../api/od_control_get.md).

The fields do not all have the same ownership or initialization rules. Some
are application inputs which must be assigned before
[`od_init()`](../api/od_init.md). Some defaults are applied during
initialization and can be changed only afterward. Caller and system fields are
normally populated from a particular door-information format. Runtime fields
are maintained by OpenDoors but remain visible to the application. Finally,
several compatibility fields are retained in the structure even though the
current library does not read them.

Every field entry in this reference identifies:

- its exact declaration and storage capacity;
- its value before initialization and any default applied by
  [`od_init()`](../api/od_init.md);
- the command-line option, configuration setting, door-information format, or
  runtime operation which can populate it;
- whether OpenDoors reads the field, writes it, or exposes it solely for the
  application to inspect;
- any restrictions on when the application may assign it.

Unless an entry says that initialization assigns a different default, the
global structure's C static-storage initialization supplies zero bytes. For a
numeric or Boolean field that is zero; for a pointer it is `NULL`; and for a
character array it is an empty string followed by zero-filled storage. A zero
value does not necessarily mean that the corresponding information was
present in the door-information file. Fields unavailable from the selected
format normally retain their initialization value.

- [Connection and door information](connection.md)
- [Caller and system information](caller.md)
- [Runtime state and policy](runtime.md)
- [Customization and callbacks](customization.md)

The header requests byte alignment for supported Turbo C versions, MSVC, and
the Open Watcom DOS32 target. Other compilers use the layout produced by their
normal ABI because [`OpenDoor.h`](../api/index.md) does not apply a universal packed attribute.
Do not calculate offsets from this documentation or copy offsets from a
different compiler, platform, or architecture; bind the current header or use
a deliberately maintained foreign-function definition for the exact build.

## Using the structure

[`tODControl`](../types.md#todcontrol) is declared in
[`OpenDoor.h`](../api/index.md). The library exports one instance named
`od_control`. For example, the BBS name is accessed as:

```c
od_control.system_name
```

The same address can be obtained with
[`od_control_get()`](../api/od_control_get.md). That function exists for
bindings and environments in which importing a data symbol is inconvenient;
it does not return a separate copy.

OpenDoors does not internally serialize this object or impose affinity to the
thread which calls [`od_init()`](../api/od_init.md). A multithreaded application
must protect every access to this object, every OpenDoors API call, and every
other public global or returned pointer with the same application lock. The
internal Windows UI consumes a cache refreshed by the serialized application
flow at OpenDoors API boundaries.

The structure is deliberately exposed for source and binary compatibility.
Applications must nevertheless use the declaration supplied by the installed
header. The header requests byte packing only for the compiler families which
the library explicitly supports that way, and the Win32-only tail fields are
absent on other platforms. Do not copy offsets from a different compiler,
architecture, or OpenDoors build.
