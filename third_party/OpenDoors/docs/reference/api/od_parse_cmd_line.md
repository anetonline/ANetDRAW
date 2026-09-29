# `od_parse_cmd_line()`

Processes the command-line options understood by OpenDoors.

## Synopsis

```c
#ifdef ODPLAT_WIN32
void od_parse_cmd_line(LPSTR pszCmdLine);
void od_parse_cmd_line_cons(INT nArgCount, char *papszArguments[]);
#else
void od_parse_cmd_line(INT nArgCount, char *papszArguments[]);
#endif
```

## Parameters

### Windows

`pszCmdLine` is the argument string supplied to `WinMain()`. It does not include
the executable name. OpenDoors applies the Windows `CommandLineToArgvW` quote
and backslash grammar before parsing it. See
[`od_split_cmd_line()`](od_split_cmd_line.md) for the exact platform-specific
rules.

Console-subsystem applications may call
[`od_parse_cmd_line_cons(argc, argv)`](od_parse_cmd_line_cons.md) explicitly.
Alternatively, define `OD_WINDOWS_CONSOLE` before including
[`OpenDoor.h`](index.md); the header aliases this function to
[`od_parse_cmd_line_cons()`](od_parse_cmd_line_cons.md), so the
`od_parse_cmd_line(argc, argv)` spelling has the same
effect. CMake's `OpenDoors::SharedConsole`, `OpenDoors::StaticConsole`, and
`OpenDoors::StaticMTConsole` targets provide that definition automatically.
These are two equivalent source interfaces, not different library binaries.

The raw-string interface is valid only in a GUI-subsystem executable and the
argument-vector interface only in a console-subsystem executable. A mismatch
is diagnosed on standard error and through the debugger, sets
[`od_control.od_error`](../control/runtime.md#od_error) to
[`ERR_PARAMETER`](../constants/errors.md#err_parameter), and terminates before
the arguments or application callbacks are processed.

### DOS

`nArgCount` and `papszArguments` are the `argc` and `argv` values supplied to
`main()` by the compiler's C runtime. DOS itself supplies a raw command tail in
the PSP rather than an argument vector. OpenDoors does not parse that command
tail; division into arguments, including any treatment of quotes, is therefore
determined by the particular DOS compiler and runtime.

### Unix-like systems

`nArgCount` and `papszArguments` are the `argc` and `argv` values supplied to
`main()`. These arguments are ordinarily constructed by the invoking shell,
although another process may provide them directly. OpenDoors does not perform
shell parsing. Element zero, conventionally the executable name, is ignored.

Passing a null argument array sets
[`od_control.od_error`](../control/runtime.md#od_error) to
[`ERR_PARAMETER`](../constants/errors.md#err_parameter) and returns. The
function otherwise reports malformed command lines by printing a message and
terminating the process, as described below.

## Return value

This function does not return a value. The help options and missing required
values terminate the process rather than returning to the caller.

## When to call it

Call [`od_parse_cmd_line()`](od_parse_cmd_line.md) before [`od_init()`](od_init.md) or any API call that
can initialize OpenDoors implicitly. Initialization settings and command-line
callbacks must therefore be installed before this call. An
[`od_add_personality()`](od_add_personality.md) call may precede it because that
function does not initialize the library.

Each call records that command-line parsing has occurred and initializes two
caller settings before examining the arguments:

- [`od_control.user_ansi`](../control/caller.md#user_ansi) is set to [`TRUE`](../constants/general.md#true).
- [`od_control.user_timelimit`](../control/caller.md#user_timelimit) is set to
  60 minutes.

These assignments occur even if neither setting appears on the command line.
Configuration-file processing and door-information-file processing occur later
during initialization and may replace settings populated here. The command-line
parser specially records only the `BPS` and `PORT` values as preset connection
information; providing both permits initialization to continue without a drop
file when no-file handling would otherwise fail.

## General syntax

Option names begin with either `-` or `/` and are compared without regard to
case. Thus `-LOCAL`, `/local`, and `-L` select the same option. A value is a
separate argument; forms such as `-NODE=2` are not recognized.

Arguments without a `-` or `/` prefix are not options. They are consumed as
values where an option requires or accepts them and are otherwise ignored.
The parser does not reject an unrecognized option unless an application callback
chooses to do so.

## Recognized options

The following table gives every spelling recognized by the current parser.
Aliases in the same row have identical behavior.

| Options | Value | Effect |
| --- | --- | --- |
| `-C`, `-CONFIG`, `-CONFIGFILE`, `-CFGFILE`, `-CFG` | filename | Sets [`od_control.od_config_filename`](../control/customization.md#od_config_filename). The parser retains at most 79 characters in its internal filename buffer. |
| `-L`, `-LOCAL` | none | Sets [`od_control.od_force_local`](../control/customization.md#od_force_local) to [`TRUE`](../constants/general.md#true). |
| `-B`, `-BPS`, `-BAUD` | decimal number | Stores the result of `atol()` in [`od_control.baud`](../control/connection.md#baud) and marks the BPS setting as preset. |
| `-P`, `-PORT` | port | Sets [`od_control.port`](../control/connection.md#port) as an explicit override. A value beginning with `COM`, such as `COM1`, is converted to the zero-based value 0. A plain decimal value is stored directly, so `-P 0` also selects COM1. The override is retained when a door-information file names another port. |
| `-N`, `-NODE` | decimal number | Stores the result of `atoi()` in [`od_control.od_node`](../control/connection.md#od_node). |
| `-?`, `-H`, `-HELP` | none | Displays command-line help and exits. |
| `-MAXTIME` | decimal number | Stores the result of `atoi()` in [`od_control.od_maxtime`](../control/customization.md#od_maxtime). |
| `-ADDRESS` | hexadecimal number | Parses the value in base 16 and stores its low [`WORD`](../types.md#word) value in [`od_control.od_com_address`](../control/connection.md#od_com_address). |
| `-IRQ` | decimal number | Stores the result of `atoi()` in [`od_control.od_com_irq`](../control/connection.md#od_com_irq). |
| `-NOFOSSIL` | none | Sets [`od_control.od_no_fossil`](../control/connection.md#od_no_fossil) to [`TRUE`](../constants/general.md#true). |
| `-NOFIFO` | none | Sets [`od_control.od_com_no_fifo`](../control/connection.md#od_com_no_fifo) to [`TRUE`](../constants/general.md#true). |
| `-D`, `-DROPFILE` | path | Copies the path to [`od_control.info_path`](../control/connection.md#info_path), truncated to fit that field. The value may name a directory or a particular door-information file. |
| `-USERNAME` | one or more words | Stores the words in [`od_control.user_name`](../control/caller.md#user_name), retaining at most 35 bytes plus the terminating nul. |
| `-TIMELEFT` | decimal number | Stores the result of `atoi()` in [`od_control.user_timelimit`](../control/caller.md#user_timelimit). |
| `-SECURITY` | decimal number | Stores the result of `atoi()` in [`od_control.user_security`](../control/caller.md#user_security). |
| `-LOCATION` | one or more words | Stores the words in [`od_control.user_location`](../control/caller.md#user_location), retaining at most 25 bytes plus the terminating nul. |
| `-G`, `-GRAPHICS` | optional value | Enables or disables [`od_control.user_ansi`](../control/caller.md#user_ansi), as detailed below. |
| `-BBSNAME` | one or more words | Stores the words in [`od_control.system_name`](../control/caller.md#system_name), retaining at most 39 bytes plus the terminating nul. |
| `-HANDLE` | decimal number | On Windows and Unix-like systems, supplies an existing native communications handle in [`od_control.od_open_handle`](../control/connection.md#od_open_handle). |
| `-SOCKET` | decimal number | On Windows and Unix-like systems, supplies an existing connected socket in [`od_control.od_open_handle`](../control/connection.md#od_open_handle) and sets [`od_control.od_use_socket`](../control/connection.md#od_use_socket) to [`TRUE`](../constants/general.md#true). There is no `-S` alias. |
| `-SILENT` | none | Sets [`od_control.od_silent_mode`](../control/runtime.md#od_silent_mode) to [`TRUE`](../constants/general.md#true). |
| `-CP437UTF8` | none | Sets [`od_control.od_cp437_to_utf8_out`](../control/connection.md#od_cp437_to_utf8_out) to [`TRUE`](../constants/general.md#true). |
| `-PERSONALITY` | none in this parser | Recognized but deliberately ignored, allowing application code such as the diagnostic example to process it separately. This parser does not consume a following personality name. |

The `-SOCKET` spelling is the only socket option. In particular, `-S` is an
unknown application option, not an abbreviation for `-SOCKET` or `-SILENT`.

### Multi-word values

`-USERNAME`, `-LOCATION`, and `-BBSNAME` collect consecutive arguments that do
not begin with `-` or `/`, inserting one space between them. This joining takes
place after the platform-specific processing described under Parameters: the
Win32 build first applies the Windows command-line grammar, DOS uses the
argument array constructed by the compiler's C runtime, and Unix-like builds
use the argument array supplied to `main()`.

Collection stops before the next prefixed option. If the joined value is too
large for its destination, OpenDoors retains the initial bytes which fit and
discards the remainder, while still advancing to the next prefixed option.
The destination is always nul-terminated. Because a leading slash denotes an
option, a word beginning with `/` also ends collection on every platform.

For example:

```text
-USERNAME Jane Q Public -LOCATION Ann Arbor Michigan -NODE 2
```

sets the name to `Jane Q Public`, the location to `Ann Arbor Michigan`, and the
node number to 2.

### Graphics value

With no following plain argument, `-G` or `-GRAPHICS` enables ANSI mode. If a
plain argument follows, the current implementation passes it to `atoi()` and
disables ANSI when the result is zero; the single letter `N` also disables it.
Consequently, the reliable explicit forms are a nonzero decimal number to enable
graphics and `0` or `N` to disable it. Do not use words such as `Y` or `ANSI`:
`atoi()` also converts those strings to zero, which disables graphics.

The disabling value is consumed. A nonzero enabling value is subsequently seen
as an ordinary non-option argument and ignored, with the same final result.

### Numeric conversion

Except for `-ADDRESS`, `-HANDLE`, and `-SOCKET`, numeric options use `atoi()` or
`atol()` exactly as shown in the table. The parser does not check those values
for trailing characters, overflow, or a range meaningful to the destination
field. Applications should supply valid decimal values within the documented
range of that field. `-ADDRESS` uses `strtol(..., 16)` and then converts the
result to [`WORD`](../types.md#word).

On Windows and Unix-like systems, `-HANDLE` and `-SOCKET` accept an optional
leading `+` followed by one or more decimal digits. The entire value must fit
in [`DWORD_PTR`](../types.md#dword_ptr); this permits the full native handle
width in both 32-bit and 64-bit builds. An empty, negative, partially numeric,
or overflowing value sets [`od_control.od_error`](../control/runtime.md#od_error)
to [`ERR_PARAMETER`](../constants/errors.md#err_parameter). It does not change
[`od_control.od_open_handle`](../control/connection.md#od_open_handle), and a
rejected `-SOCKET` value does not change
[`od_control.od_use_socket`](../control/connection.md#od_use_socket).

DOS does not support adopting a native communications object through these
options. Its serial and FOSSIL interfaces are selected through the port-related
options instead.

## Application-defined options

Two callbacks can receive an option that starts with `-` or `/` but does not
match any name in the table:

1. If [`od_control.od_cmd_line_flag_handler`](../control/customization.md#od_cmd_line_flag_handler)
   is non-null, OpenDoors calls it with the original argument, including its
   prefix and original letter case. A [`TRUE`](../constants/general.md#true) return marks the option handled.
2. If the flag callback is absent or returns [`FALSE`](../constants/general.md#false), and
   [`od_control.od_cmd_line_handler`](../control/customization.md#od_cmd_line_handler)
   is non-null, OpenDoors calls it with the original option and the consecutive
   non-prefixed arguments that follow it, joined with spaces.

The second callback's option text is assembled in an internal 80-byte buffer.
OpenDoors retains at most the first 79 bytes of the joined text and supplies a
terminating nul. It discards any remaining non-option text while advancing to
the next prefixed option. The callback is called with an empty string when the
next argument is another prefixed option. If neither callback handles an
unknown option, it and any otherwise unclaimed plain arguments are ignored.

Recognized OpenDoors options are not passed to these callbacks. `-PERSONALITY`
is also not passed to them because the parser recognizes and ignores it.

Install callbacks before calling [`od_parse_cmd_line()`](od_parse_cmd_line.md):

```c
static BOOL ODCALL
handle_flag(const char *option)
{
    if (stricmp(option, "-FAST") == 0 || stricmp(option, "/FAST") == 0) {
        /* Enable the application's fast mode. */
        return TRUE;
    }
    return FALSE;
}

int
main(int argc, char **argv)
{
    od_control.od_cmd_line_flag_handler = handle_flag;
    od_parse_cmd_line(argc, argv);
    od_init();
    /* ... */
}
```

## Help and errors

Set [`od_control.od_cmd_line_help_func`](../control/customization.md#od_cmd_line_help_func)
before parsing to replace help handling completely. When a help option is
encountered, OpenDoors calls this function and then exits with status 0 if the
function returns.

Without a help function, [`od_control.od_cmd_line_help`](../control/customization.md#od_cmd_line_help)
replaces the built-in help text. Windows displays it in a message box; other
targets write it to standard output. OpenDoors then exits with status 1. The
built-in help text is not a complete list of the names accepted by the parser;
the table on this page is complete for the current implementation.

When an option that requires a separate value is the final argument, OpenDoors
prints `Missing parameter for option:` followed by the option and exits with
status 1. Multi-word options make the same check for the presence of a following
argument, although that argument can itself be another prefixed option and thus
produce an empty stored value.

On Windows, failure to divide the command line because memory could not be
allocated leaves [`od_control.od_error`](../control/runtime.md#od_error) set to
[`ERR_MEMORY`](../constants/errors.md#err_memory) and makes this function return
without parsing.

## Portable startup example

```c
#include "OpenDoor.h"

#ifdef ODPLAT_WIN32
int WINAPI
WinMain(HINSTANCE instance, HINSTANCE previous,
        LPSTR command_line, int show_command)
{
    (void)instance;
    (void)previous;

    od_control.od_cmd_show = show_command;
    od_parse_cmd_line(command_line);
    od_init();

    /* Door code goes here. */
    od_exit(0, FALSE);
    return 0;
}
#else
int
main(int argc, char **argv)
{
    od_parse_cmd_line(argc, argv);
    od_init();

    /* Door code goes here. */
    od_exit(0, FALSE);
    return 0;
}
#endif
```

## See also

[`od_split_cmd_line()`](od_split_cmd_line.md),
[`od_free_split_cmd_line()`](od_free_split_cmd_line.md),
[`od_init()`](od_init.md),
[Configuration and command lines](../../guides/configuration.md)
