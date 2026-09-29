# Constants and macros

Include [`OpenDoor.h`](api/index.md) rather than copying numeric values into an
application. The reference is divided by purpose so every public value can be
described individually.

- [General and platform definitions](constants/general.md) covers the version,
  platform selection, booleans, timeouts, and child-process modes.
- [Errors and exit levels](constants/errors.md) covers API errors and configured
  process exit reasons.
- [Colors](constants/colors.md) covers foreground, background, blink, and
  compatibility color names.
- [Input and editors](constants/input.md) covers translated keys, input modes,
  editor flags, formats, and results.
- [Menus and screen](constants/display.md) covers popup menus, scrolling,
  status lines, box characters, and terminal detection.
- [Session and connection](constants/session.md) covers door-information
  formats, communication methods, paging, events, and disabled behaviors.
- [Components and personalities](constants/components.md) covers optional
  components, built-in personalities, and personality callback operations.

Compatibility aliases for older source are collected separately under
[Compatibility interfaces](compatibility.md).
