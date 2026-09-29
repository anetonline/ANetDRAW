# Getting started

Every OpenDoors program includes [`OpenDoor.h`](reference/api/index.md),
supplies a small amount of program information through
[`od_control`](reference/control/index.md), and then calls the API functions it
needs. OpenDoors initializes itself when most API functions are first called,
but an explicit call to [`od_init()`](reference/api/od_init.md) makes the
program's startup sequence much clearer.

```c
#include "OpenDoor.h"

int main(int argc, char **argv)
{
    od_control.od_prog_name = "Hello Door";
    od_control.od_prog_version = "1.0";

#ifndef _WIN32
    od_parse_cmd_line(argc, argv);
#endif
    od_init();

    od_printf("`bright cyan`Welcome, %s!`white`\n\r",
        od_control.user_name);
    od_disp_str("Press a key to continue... ");
    (void)od_get_key(TRUE);

    od_exit(0, FALSE);
    return 0;
}
```

Settings which affect initialization must be assigned before
[`od_init()`](reference/api/od_init.md) or before any API function which
initializes OpenDoors automatically. Once the session is active, use the API for
remote input and output rather than reading the console or connection directly.

## Build and link

When OpenDoors is included with CMake, link against either `OpenDoors::Shared`
or `OpenDoors::Static`:

```cmake
add_subdirectory(path/to/OpenDoors)
target_link_libraries(mydoor PRIVATE OpenDoors::Shared)
```

An installed OpenDoors package provides the same target names:

```cmake
find_package(OpenDoors 6.3 CONFIG REQUIRED COMPONENTS Shared)
target_link_libraries(mydoor PRIVATE OpenDoors::Shared)
```

On Windows, `OpenDoors::Static` defines
[`OD_WIN32_STATIC`](guides/building.md) for its consumers automatically.
Programs which link the static library without that CMake target must define it
themselves. See [Building and linking](guides/building.md) for the standalone
build commands, library names, and DOS build.

## Where to go next

- [Session lifecycle](guides/session-lifecycle.md) explains initialization,
  the kernel, carrier handling, and orderly exit.
- [Terminal and screen model](guides/terminal-screen.md) explains local and
  remote output, ANSI/AVATAR requirements, and screen coordinates.
- [Configuration and command lines](guides/configuration.md) describes the
  standard configuration mechanisms.
- [API function groups](reference/api/index.md) provides the complete function
  index.
