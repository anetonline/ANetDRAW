# `od_spawnvpe()`

Executes a child program with an argument vector and an optional replacement
environment, searching for the executable when necessary.

## Synopsis

```c
INT16 od_spawnvpe(INT16 nModeFlag, const char *pszPath,
    const char *const papszArg[], const char *const papszEnv[]);
```

## Parameters

`nModeFlag`
: [`P_WAIT`](../constants/general.md#p_wait) waits for the child to finish.
  [`P_NOWAIT`](../constants/general.md#p_nowait) requests asynchronous
  execution on targets which support it. The DOS implementations accept only
  [`P_WAIT`](../constants/general.md#p_wait).

`pszPath`
: Name or path of the executable. If this value does not include a directory,
  OpenDoors checks the current directory and then the directories named by the
  current `PATH` environment variable.

`papszArg`
: Null-terminated argument array. Supply the executable name in element zero
  when required by the child program or host C runtime. On Windows and
  Unix-like systems, each string represents one raw argument; callers do not
  add quotation marks merely because an argument contains spaces. On DOS, the
  compiler runtime serializes the array into a raw command tail, whose eventual
  interpretation depends on the child program.

`papszEnv`
: Null-terminated array of `name=value` environment strings to be given to the
  child. Pass `NULL` to give the child a copy of the current environment.

## Return value

With [`P_WAIT`](../constants/general.md#p_wait), the function returns `-1` if
the child could not be launched. Otherwise, it returns the exit status supplied
by the child process. The public return type is
[`INT16`](../types.md#int16), so a wider native result is narrowed.

With [`P_NOWAIT`](../constants/general.md#p_nowait), `-1` reports that the
asynchronous child could not be launched. Any other value reports a successful
launch. The successful value is platform dependent and must not be treated as
a portable process identifier.

## Description

[`od_spawnvpe()`](od_spawnvpe.md) is the full-featured counterpart of
[`od_spawn()`](od_spawn.md). It accepts an argument vector, can replace the
child's environment, searches for an executable named without a directory, and
returns the child's exit status when called with
[`P_WAIT`](../constants/general.md#p_wait).

Before a waited-for launch, OpenDoors drains pending output and suspends the
communications and kernel resources which cannot remain active across the
child. On platforms with a local screen, it preserves the screen state as
described below. After the child finishes, OpenDoors reopens those resources,
clears stale input, restores the saved local state, and adjusts the caller's
remaining-time accounting according to
[`od_spawn_freeze_time`](../control/customization.md#od_spawn_freeze_time).

On Windows, OpenDoors quotes and escapes each raw argument before passing it to
the target's `_spawnvpe()`-compatible interface. Unix-like targets pass the
array directly to the operating system. DOS retains its established compiler
runtime launch path without imposing a command-line grammar that the operating
system does not define. The strings in a non-null `papszEnv` become the complete
child environment; they are not additions to the current environment. Each
environment entry must have the form `name=value`, and the final array element
must be `NULL`.

## Platform notes

On 16-bit DOS and DOS32, only [`P_WAIT`](../constants/general.md#p_wait) is
valid. A different mode returns `-1` and sets
[`od_control.od_error`](../control/runtime.md#od_error) to
[`ERR_PARAMETER`](../constants/errors.md#err_parameter). OpenDoors saves the
80-by-25 local screen and current directory around the child. If an executable
name has no extension, the 16-bit DOS launcher tries `.COM` and then `.EXE`;
DOS32 delegates the launch to the Open Watcom runtime. The 16-bit build can
also swap the door to EMS or disk while the child is running.

On Windows, OpenDoors delegates process creation and executable lookup to the
Microsoft-compatible `_spawnvpe()` runtime. Windows creates a process with one
command-line string, so OpenDoors serializes each raw array element using the
Microsoft C command-line quote and backslash rules. For example, an element
whose value is `two words` reaches a conventional C child as one argument
containing those two words; caller-supplied quotation marks are preserved as
literal characters in that argument. Programs which intentionally use a
different command-line decoder may instead use [`od_spawn()`](od_spawn.md) to
supply preformatted Windows command text. A successful
[`P_NOWAIT`](../constants/general.md#p_nowait) value originates as a native
process handle, but the public interface narrows it to
[`INT16`](../types.md#int16); portable code must use it only as a success
result. A waited-for child is accompanied by a local "Running sub-program..."
message.

On Unix-like targets, executable names do not receive DOS `.COM` or `.EXE`
suffixes. A bare name is checked in the current directory and then in the
current `PATH`; a name containing `/` is used directly. Failure of the final
execution attempt is returned as `-1` with its error in `errno`. For
[`P_NOWAIT`](../constants/general.md#p_nowait), OpenDoors uses a double fork
and reaps the intermediate process, so the asynchronous process cannot leave a
zombie owned by the door. This operation does not change the application's
`SIGCHLD` disposition, and a successful asynchronous launch returns zero.

Allocation of the DOS screen or directory buffers, or of the temporary Windows
quoted argument vector, can fail with
[`ERR_MEMORY`](../constants/errors.md#err_memory). On Windows, a null argument
array or a null element zero fails with
[`ERR_PARAMETER`](../constants/errors.md#err_parameter). Other native launch
failures are represented by the return value and the platform runtime's error
state; this function does not translate every native failure into
[`od_control.od_error`](../control/runtime.md#od_error).

## Example

This example searches for `TEST.EXE`, supplies two arguments, and gives the
child a copy of the current environment:

```c
const char *args[] = { "TEST.EXE", "argument one", "--check", NULL };
INT16 result = od_spawnvpe(P_WAIT, "TEST.EXE", args, NULL);

if(result == -1)
    od_log_write("Unable to start TEST.EXE");
else
    od_printf("TEST.EXE returned %d\n\r", (int)result);
```

## See also

[`od_spawn()`](od_spawn.md), [Child-process modes](../constants/general.md#child-process-modes)
