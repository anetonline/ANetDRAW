# `od_parse_cmd_line_cons()`

Processes the OpenDoors command line for a Windows console-subsystem
application.

## Synopsis

```c
void od_parse_cmd_line_cons(INT nArgCount, char *papszArguments[]);
```

`nArgCount` and `papszArguments` are the `argc` and `argv` values supplied to
`main()`. Element zero, conventionally the executable name, is ignored.

A console application may call this function explicitly after including
[`OpenDoor.h`](index.md). Alternatively, it may define `OD_WINDOWS_CONSOLE`
before including the header; the header then aliases
[`od_parse_cmd_line()`](od_parse_cmd_line.md) to this function, allowing the
portable `od_parse_cmd_line(argc, argv)` spelling. The
`OpenDoors::SharedConsole`, `OpenDoors::StaticConsole`, and
`OpenDoors::StaticMTConsole` CMake targets propagate that definition. Both
forms call the same function and link the same OpenDoors binaries.

This interface is valid only when the host executable declares the Windows
console subsystem. A GUI-subsystem mismatch writes a diagnostic to standard
error and the debugger, sets
[`od_control.od_error`](../control/runtime.md#od_error) to
[`ERR_PARAMETER`](../constants/errors.md#err_parameter), and terminates before
processing the arguments or invoking command-line callbacks.

The recognized options, defaults, callback behavior, errors, and timing
requirements are documented with
[`od_parse_cmd_line()`](od_parse_cmd_line.md#when-to-call-it).
