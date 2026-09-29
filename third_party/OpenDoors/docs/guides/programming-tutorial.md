# OpenDoors programming tutorial

An OpenDoors program is an ordinary C or C++ application linked with the
OpenDoors library. The library reads the BBS door-information file, establishes
the caller connection, maintains local and remote screen state, handles session
timing and carrier detection, and provides the input, output, menu, chat, and
file-display functions used by the application.

This tutorial builds a small door and then examines the facilities used by the
larger examples in the repository. The [API reference](../reference/api/index.md)
and [`od_control` reference](../reference/control/index.md) give the complete
contract for each interface introduced here.

## Building a door

Every source file which uses the API includes the public header with its actual
case-sensitive name:

```c
#include <OpenDoor.h>
```

[`OpenDoor.h`](../reference/api/index.md) is the application API. Do not include
private source-tree headers. A DOS personality module additionally includes
[`ODStat.h`](../reference/personality/index.md), but an ordinary door does not.

### Installed CMake package

The current desktop SDK installs CMake package metadata and exports separate
shared and static targets. A minimal project is:

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyDoor LANGUAGES C)

find_package(OpenDoors 6.3 CONFIG REQUIRED COMPONENTS Static)

add_executable(mydoor mydoor.c)
target_link_libraries(mydoor PRIVATE OpenDoors::Static)
```

Configure it with the installation prefix or extracted SDK on the CMake search
path:

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/opendoors
cmake --build build
```

Use the `Shared` component and `OpenDoors::Shared` target for the shared
library. On MSVC, an SDK built with the optional static Microsoft runtime also
provides `StaticMT` and `OpenDoors::StaticMT`. The imported targets propagate
the include directory, platform definitions, link dependencies, and the
[`OD_WIN32_STATIC`](../reference/constants/general.md#od_win32_static)
definition required by a Windows static build. Prefer them to spelling a
library filename in a build script.

The shared Windows library must be available to the executable at run time.
The installed or release SDK supplies the matching import library for linking;
MSVC and MinGW import libraries are different formats and are not
interchangeable. See [Building and linking](building.md) for library names,
build switches, installation, and compiler-specific details.

### DOS builds

The 16-bit and 32-bit DOS libraries are produced by the separate Open Watcom
project under `dos/`. The 16-bit build uses the large memory model. A door and
every object linked into it must use a compatible memory model and calling
convention.

The DOS32 build supplies one library for Open Watcom's `-3r` register calling
convention and one for `-3s` stack calls. Select the library matching the
application. The resulting flat-model program can be linked for DOS/4GW or for
a native DOS/32A executable with the extender bound into it. These choices do
not change the OpenDoors source API, but mixing calling conventions changes the
ABI and will not work.

Legacy Borland and Microsoft DOS projects may continue to link their matching
OpenDoors library in the usual compiler project or makefile. Source intended
for those compilers must not use modern language features unconditionally;
place platform-specific modern code behind the appropriate
[`ODPLAT_*`](../reference/constants/general.md#platform-selection) guards.

### What the build must agree upon

Including the correct header is only half of the build contract. The compiler
which compiles the door and the compiler which built the selected library must
agree upon the target architecture and application binary interface. In a
modern hosted build, use a library built for the same operating system,
processor architecture, and compiler family. A 64-bit program cannot link a
32-bit library. On Windows, an MSVC import or static library is not a substitute
for the corresponding MinGW file, even when both builds ultimately use the
same OpenDoors DLL name.

The same rule is especially important under DOS. A 16-bit large-model door
needs a large-model library and compatible structure packing and calling
conventions. A 32-bit Open Watcom door must select the register-call or
stack-call DOS32 library which matches its compiler switch. Far pointers and
segmented objects are part of the 16-bit interface; they must not be hidden by
source-level aliases intended to imitate a flat model.

The installed CMake targets encode the hosted-platform requirements for you.
When working with an older project file or makefile, the responsibility moves
to that build description. Recompile every application object after changing
architecture, memory model, runtime selection, structure-packing, or calling
convention; replacing only the library leaves old objects with the previous
ABI.

### Building the supplied examples

The examples release bundle is an independent CMake consumer of the installed
SDK. From the extracted examples directory, configure it as follows. This is
useful both as a set of demonstrations and as a check that the SDK can be
consumed without private source-tree headers:

```sh
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH=/path/to/opendoors \
  -DOPENDOORS_EXAMPLE_VARIANT=Static
cmake --build build
```

Select `Shared`, `Static`, or, in the matching MSVC package, `StaticMT` as the
example variant. All six examples require only OpenDoors. The Vote target
enables its built-in multi-node sidecar locking.

When building directly from this repository rather than from a release
bundle, the top-level project offers the same examples. This is convenient
while changing OpenDoors itself, but it is not a substitute for testing the
installed SDK: an in-tree target can accidentally see files which an installed
consumer cannot. The package-consumer test and separate examples project exist
to catch that class of mistake.

## A minimal door

The following program parses the standard command line, initializes the
session, displays two lines, waits for a key, and returns to the BBS:

```c
#include <OpenDoor.h>

#ifdef ODPLAT_WIN32
int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous,
    LPSTR command_line, int show_command)
#else
int main(int argc, char **argv)
#endif
{
#ifdef ODPLAT_WIN32
    (void)instance;
    (void)previous;
    od_control.od_cmd_show = show_command;
    od_parse_cmd_line(command_line);
#else
    od_parse_cmd_line(argc, argv);
#endif

    od_init();

    od_printf("Hello, %s!\n\r", od_control.user_name);
    od_printf("Press any key to return to the BBS.\n\r");
    od_get_key(TRUE);

    od_exit(0, FALSE);
    return 0;
}
```

The Windows entry point is useful for examples built as GUI-subsystem
applications so Windows does not create an unrelated console window. A console
subsystem application can use `main()` if its build and command-line handling
are arranged accordingly.

The example contains all of the required phases of a small door:

1. It includes [`OpenDoor.h`](../reference/api/index.md), which declares both
   the API functions and the exported [`od_control`](../reference/control/index.md)
   structure.
2. It passes the process command line to
   [`od_parse_cmd_line()`](../reference/api/od_parse_cmd_line.md) before any
   call can initialize OpenDoors. The Windows and non-Windows forms differ
   because Windows supplies a single command-line string to `WinMain`, while
   C supplies an argument count and vector to `main`.
3. It calls [`od_init()`](../reference/api/od_init.md) explicitly. At this
   point the drop file has been detected and read, communications have been
   selected, defaults have been installed, and caller fields may be used.
4. It writes terminal lines with carriage-return/line-feed sequences. A
   remote terminal normally requires both controls: `\n` advances to the next
   row and `\r` returns to the first column.
5. It waits with [`od_get_key()`](../reference/api/od_get_key.md), then calls
   [`od_exit()`](../reference/api/od_exit.md) with hangup disabled so the BBS
   can continue the caller's session.

Even this small program receives carrier monitoring, session-time enforcement,
local sysop keys, status handling, and terminal-aware output from the library.
Those services do not require application code in the main loop, but they do
depend upon the application entering OpenDoors often enough for periodic
processing to run.

### Command-line processing

The two public forms of
[`od_parse_cmd_line()`](../reference/api/od_parse_cmd_line.md) accept the
platform's natural command-line representation. The standard parser recognizes
local mode, drop-file location, port and speed overrides, Door32 handles,
sockets, node number, caller identity overrides, personality selection, and
the other options listed in its reference.

Call the parser before [`od_init()`](../reference/api/od_init.md). A program may
also parse its own arguments or install the documented callbacks for custom
options, but communications and door-information overrides must be established
before initialization consumes them.

### Initialization

Most OpenDoors functions call [`od_init()`](../reference/api/od_init.md)
automatically on first use. That convenience makes the smallest possible door
very short, but explicit initialization is preferable in a real program:

1. assign all pre-initialization settings in
   [`od_control`](../reference/control/index.md);
2. parse the command line;
3. call [`od_init()`](../reference/api/od_init.md); and
4. inspect caller, system, terminal, and connection fields only after the call
   succeeds.

An output call made too early can initialize the library before the
application has selected its configuration file, components, callbacks,
connection method, or program identity. The
[session lifecycle](session-lifecycle.md) lists the phases in their normal
order.

### Orderly shutdown

[`od_exit()`](../reference/api/od_exit.md) performs OpenDoors shutdown, updates
supported door-information files, closes the activity log, restores local
state, optionally hangs up, and terminates with the requested error level. A
normal “return to BBS” choice passes
[`FALSE`](../reference/constants/general.md#false) for hangup; an explicit
logoff choice passes [`TRUE`](../reference/constants/general.md#true).

Do not replace this call with the C runtime `exit()` or a return from `main()`
when an initialized session needs orderly shutdown. Those paths cannot perform
the complete OpenDoors cleanup contract.

## Running locally and under a BBS

OpenDoors can operate without a remote connection. Local mode is convenient
for program flow, menus, file access, and much display testing. With standard
command-line processing enabled, run a door with `-LOCAL`, or set
[`od_control.od_force_local`](../reference/control/customization.md#od_force_local)
before initialization.

On DOS and Windows, local mode sets
[`od_control.baud`](../reference/control/connection.md#baud) to zero. A
Unix-like local login uses standard input and output and records the terminal
speed, with a nominal 19,200 BPS value when that information is unavailable.
There is no remote carrier or modem object, but the normal screen and input
functions remain available. Terminal behavior which depends on a real byte
stream, latency, a particular emulator, Door32, a socket, or FOSSIL I/O must
still be tested in an appropriate remote environment.

In remote mode, the BBS normally writes a drop file and then starts the door in
the corresponding node directory or passes its location on the command line.
OpenDoors recognizes the supported `DORINFO?.DEF`, `DOOR.SYS`,
`EXITINFO.BBS`, `CHAIN.TXT`, `CALLINFO.BBS`, `SFDOORS.DAT`, `TRIBBS.SYS`, and
`DOOR32.SYS` forms. The selected type is reported by
[`od_control.od_info_type`](../reference/control/connection.md#od_info_type).

Modern hosts often pass a Door32 handle, a socket, or standard input/output
instead of granting a door direct ownership of a UART. The communications
method is reported by
[`od_control.od_com_method`](../reference/control/connection.md#od_com_method).
The BBS and door must agree on ownership: a borrowed handle must not be opened
as if it were an independently configured serial port.

### What happens when a BBS launches a door

The usual launch sequence begins while the caller is still inside the BBS. The
BBS writes a door-information file in the node's working directory, or updates
an existing one, then launches the door with any required path, node, handle,
or connection arguments. The door reads that state during initialization and
uses the existing terminal connection. It does not answer a new call.

While the door runs, the BBS normally waits. OpenDoors watches the live
connection and the time limit reported by the BBS. On orderly exit it updates
the formats which permit writeback, closes resources it owns, and returns an
error level to the launching process. The BBS can then reload the changed
record, continue the caller's session, disconnect the caller if the door
requested a hangup, or use the error level for BBS-specific routing.

This sequence is the reason a door must distinguish “return to BBS” from
“hang up.” Both choices terminate the door program; only the latter should
terminate the caller's connection. It is also why a door should not retain
assumptions about its initial working directory or a particular drop-file
format when the parser and the BBS provide explicit paths.

### Testing with the sample drop file

The source tree contains `examples/data/DORINFO1.DEF`, a local test record. Its twelve lines
contain, in order, the BBS name, sysop first and last names, port description,
communications description, a legacy unused field, caller first and last
names, caller location, ANSI setting, security level, and minutes remaining.
The supplied file specifies `COM0` and `0 BAUD,N,8,1`, so it describes a local
session rather than a modem connection.

Place a copy in the door's working directory or point the standard command
line at its directory, then run the door normally. Alternatively, `-LOCAL`
constructs local operation without requiring that test file. A local test is
appropriate for menus, file handling, color selection, callbacks, and orderly
shutdown. It cannot prove that a real terminal interprets escape sequences as
expected, that a passed socket remains live, or that a BBS correctly consumes
writeback, so a release should also be exercised through the environments it
claims to support.

## Output and the screen model

[`od_printf()`](../reference/api/od_printf.md) is the usual formatted output
function. It accepts C `printf()` conversions and OpenDoors color descriptions:

```c
od_printf("`bright white on blue`Welcome, %s!`white on black`\n\r",
    od_control.user_name);
```

[`od_disp_str()`](../reference/api/od_disp_str.md) sends a null-terminated
string without C formatting. [`od_disp()`](../reference/api/od_disp.md) sends a
specified byte count and is appropriate when the data is not null terminated.
[`od_putch()`](../reference/api/od_putch.md) displays one byte.

All of these functions update the authoritative session screen as appropriate
and transmit through the active connection. On Unix, Windows, and the modern
virtual-screen paths, this avoids making the remote terminal's dimensions
depend on the physical local console. Cursor-addressed operations use the
screen dimensions established for the caller.

Use [`od_set_color()`](../reference/api/od_set_color.md) or
[`od_set_attrib()`](../reference/api/od_set_attrib.md) for programmatic color
changes. Plain ASCII callers receive no terminal color controls, although the
local or virtual display still maintains its current cell attributes.

Screen-clearing policy is separate from color. The caller's preference and
[`od_control.od_always_clear`](../reference/control/customization.md#od_always_clear)
determine whether [`od_clr_scr()`](../reference/api/od_clr_scr.md) performs a
clear. The function reference describes its exact policy; do not assume that a
clear request is unconditional.

For saved areas, use the size-aware
[`od_save_screen_ex()`](../reference/api/od_save_screen_ex.md) and
[`od_restore_screen_ex()`](../reference/api/od_restore_screen_ex.md)
interfaces in new code. The established
[`od_gettext()`](../reference/api/od_gettext.md),
[`od_puttext()`](../reference/api/od_puttext.md), and popup-window interfaces
retain their historical coordinate and buffer contracts.

### Coordinates, scrolling, and windows

OpenDoors screen coordinates are one-based. A cursor position of row 1,
column 1 is the upper-left cell. Rectangle functions accept inclusive edges,
so a rectangle from `(1, 1)` through `(80, 24)` contains 1,920 cells rather
than 1,817. Validate computed rectangles before calling the API: reversing an
edge or allowing an unsigned calculation to wrap does not describe an empty
area.

[`od_set_cursor()`](../reference/api/od_set_cursor.md) positions subsequent
screen output. [`od_clr_line()`](../reference/api/od_clr_line.md) clears from
the current column through the right edge and leaves the cursor at its
documented final position; it is not an abbreviation for clearing the entire
row regardless of the starting column. [`od_scroll()`](../reference/api/od_scroll.md)
moves the contents of an inclusive rectangle and fills the vacated cells with
the selected attribute.

Popup windows created by
[`od_window_create()`](../reference/api/od_window_create.md) save the covered
area and return an opaque handle. Pass that handle to
[`od_window_remove()`](../reference/api/od_window_remove.md) once, in reverse
nesting order when windows overlap. A saved window represents remote screen
state; it must not be sized from assumptions about an 80-column local console.

Writing the last cell of a terminal line deserves care. Many terminals advance
to the next row after receiving a character in the rightmost column, and an
advance from the bottom-right cell may scroll the screen. OpenDoors maintains
its virtual cursor according to the documented terminal model, but an
application which draws borders or fixed grids should avoid emitting an extra
printable byte or newline after the final cell unless that movement is
intended.

## Input

[`od_get_key()`](../reference/api/od_get_key.md) returns one queued input byte.
Passing [`TRUE`](../reference/constants/general.md#true) waits; passing
[`FALSE`](../reference/constants/general.md#false) polls. It accepts remote
input and ordinary enabled local-keyboard input in arrival order.

Use [`od_get_answer()`](../reference/api/od_get_answer.md) when only a small set
of one-byte responses is valid:

```c
od_printf("Return to the BBS? (Y/N) ");
if(od_get_answer("YN") == 'Y')
    od_exit(0, FALSE);
```

[`od_get_input()`](../reference/api/od_get_input.md) is the structured input
interface. It reports whether the event came from the remote or local side and
translates recognized terminal sequences into
[`OD_KEY_*`](../reference/constants/input.md#extended-key-codes) events. Use it
for cursor keys, function keys, and applications which care who generated the
input.

[`od_input_str()`](../reference/api/od_input_str.md) provides simple
line-oriented input for every terminal mode.
[`od_edit_str()`](../reference/api/od_edit_str.md) provides a formatted,
cursor-addressed field editor, while
[`od_multiline_edit()`](../reference/api/od_multiline_edit.md) provides a text
editor with paging, wrapping, and optional callbacks.

Single-byte input functions return byte values, not Unicode characters. A door
which accepts extended CP437 input should use an unsigned-capable object or an
integer for the returned value and should follow each function's documented
range. Do not store every result in a signed `char` and then assume values from
128 through 255 remain positive on every compiler.

Polling deserves a different loop from blocking input. A nonblocking call can
return zero when no byte is ready; the application must perform useful work,
call [`od_kernel()`](../reference/api/od_kernel.md), or yield before polling
again. A tight loop which does nothing but ask for input consumes a processor
and can starve the rest of the BBS. Conversely, a blocking call is appropriate
for an ordinary menu prompt because OpenDoors continues its internal session
work while waiting.

Line and field editors require a destination with the capacity stated by the
function. The maximum input length does not allocate storage for the caller.
Include room for the terminating null byte, initialize any starting text when
the interface expects it, and do not pass a pointer to read-only string
storage. For cursor-addressed editors, also ensure that the field and its
prompt fit within the active remote screen.

## Using `od_control`

The exported [`od_control`](../reference/control/index.md) structure serves two
distinct purposes:

- settings assigned before initialization select components, callbacks,
  connection overrides, program identity, colors, prompts, and policies; and
- fields populated during initialization report the BBS, caller, terminal,
  connection, timing, and door-information state.

For example, after initialization the caller's name is in
[`od_control.user_name`](../reference/control/caller.md#user_name), location in
[`od_control.user_location`](../reference/control/caller.md#user_location),
security level in
[`od_control.user_security`](../reference/control/caller.md#user_security),
and remaining minutes in
[`od_control.user_timelimit`](../reference/control/caller.md#user_timelimit).

Not every drop-file format contains every value. Each field reference states
its initial value, the formats or conditions which populate it, and whether
OpenDoors itself reads or writes it. Test
[`od_control.od_info_type`](../reference/control/connection.md#od_info_type) or
the documented availability condition before treating a format-specific field
as authoritative.

Some caller fields are written back to supported `EXITINFO.BBS` variants during
shutdown. Do not modify a field merely because it is publicly visible; follow
the field's ownership and writeback description.

## Periodic processing

Normal API calls give [`od_kernel()`](../reference/api/od_kernel.md) regular
opportunities to process carrier state, input, local sysop keys, time limits,
inactivity, status updates, and callbacks. During a computation or external
wait which makes no OpenDoors calls, invoke it periodically:

```c
for(record = 0; record < record_count; ++record)
{
    process_record(record);
    od_kernel();
}
```

When a polling loop has nothing else to wait for, call
[`od_kernel()`](../reference/api/od_kernel.md) for periodic processing and use
[`od_sleep()`](../reference/api/od_sleep.md) between calls instead of an
operating-system busy wait. A long section which holds an application data-file
lock should normally finish and release the lock promptly rather than call
arbitrary callbacks or wait for caller input while shared data is unavailable.

## Optional components and callbacks

The built-in configuration reader is selected before initialization by
assigning [`INCLUDE_CONFIG_FILE`](../reference/constants/components.md#include_config_file)
to [`od_control.od_config_file`](../reference/control/customization.md#od_config_file).
The default configuration filename and complete keyword set are described in
[Configuration files](configuration.md).

An application can handle its own keywords or observe built-in settings through
[`od_control.od_config_function`](../reference/control/customization.md#od_config_function).
The callback receives every nonblank configuration line after OpenDoors has
processed any matching built-in option. It runs during initialization and must
honor the argument-lifetime and re-entry rules in the field reference.

The activity logger is selected with
[`INCLUDE_LOGFILE`](../reference/constants/components.md#include_logfile) in
[`od_control.od_logfile`](../reference/control/customization.md#od_logfile).
It records standard session events, while
[`od_log_write()`](../reference/api/od_log_write.md) adds application events.
Log messages should be short, factual, and useful to an operator diagnosing a
session.

The DOS multiple-personality selector is selected with
[`INCLUDE_MPS`](../reference/constants/components.md#include_mps) in
[`od_control.od_mps`](../reference/control/customization.md#od_mps).
Personalities affect only the DOS local status display and sysop keys. They do
not change drop-file parsing or the remote terminal protocol.

Lifecycle callbacks can replace or supplement chat, shell, time-warning,
configuration, and shutdown behavior. Install them before the phase in which
they are used, and avoid assuming that every callback runs on the application's
main thread on every platform.

## Menus and external display files

[`od_send_file()`](../reference/api/od_send_file.md) displays an explicitly
named file or selects `.rip`, `.avt`, `.ans`, and `.asc` variants according to
the caller's capabilities. It supports page pausing, interruption, local
emulation, and the implemented RemoteAccess/QuickBBS substitutions documented
in its reference.

[`od_hotkey_menu()`](../reference/api/od_hotkey_menu.md) adds a set of valid
menu keys and can return as soon as one is selected, even before the display
file has finished. This permits a sysop to replace a compiled menu screen
without changing application logic:

```c
char choice;

choice = od_hotkey_menu("MAINMENU", "PLQ", TRUE);
switch(choice)
{
    case 'P':
        od_page();
        break;

    case 'L':
        od_exit(0, TRUE);
        break;

    case 'Q':
        od_exit(0, FALSE);
        break;
}
```

Always retain a usable text path. A RIP or AVATAR screen alone is not a
substitute for an ASCII fallback when the caller does not advertise those
capabilities.

## Multi-node data

Two nodes can execute the same door simultaneously. Any shared score, poll,
user, message, or configuration data must therefore be designed for concurrent
access. The safe update pattern is:

1. acquire the file or record lock;
2. read the current value while the lock is held;
3. calculate and write the update;
4. flush as required; and
5. release the lock immediately.

Reading before acquiring the lock can lose another node's update. Holding a
lock while waiting for caller input can stall every other node. See
[Multi-node programming](multinode.md) for the detailed failure scenario and
the locking approaches used by the examples.

## A tour of `ex_vote.c`

The Vote example is large enough to resemble a practical door while remaining
organized around a small number of OpenDoors features. It maintains a set of
poll questions, answer totals, and a record of the questions on which each
caller has voted. The program demonstrates startup customization, caller
fields, an external menu, OpenDoors input and output, configuration callbacks,
logging, shutdown callbacks, portable records, and multi-node file locking.

The example is older application code retained as a demonstration; its own
displayed “Version 6.00” string is the example program's text and is not the
version of the library being linked. Read the API reference for the current
contract.

### Records and limits

Near the beginning of the file, Vote defines limits for 200 questions, 15
answers per question, and 30,000 caller records. `tUserRecord` contains the
caller's name and one Boolean byte for each possible question. The current
user record and its file index remain in memory for the session.

`tQuestionRecord` contains the question text, the possible answer strings,
counts, creator name, and creation time. `ex_votio.c` serializes both record
types explicitly: `vote.usr` uses 236-byte records and `vote.qst` uses the
historical 664-byte DOS record layout. Multi-byte values are little-endian,
and creation time is an unsigned 32-bit count of POSIX epoch seconds, covering
dates through 2106-02-07 06:28:15 UTC. Existing files with another record size
or invalid field values are rejected with a diagnostic; the example does not
attempt an ambiguous in-place migration.

The global `nViewResultsFrom` policy begins by allowing results for questions
already answered by the caller. `nQuestionsVotedOn` accumulates activity for
the session and is later included in the OpenDoors log. Neither variable
belongs in [`od_control`](../reference/control/index.md), because both describe
Vote's application state rather than the BBS session.

### Startup customization

Vote's `main`/`WinMain` first supplies a program name, version text, and
copyright text. These must be set before initialization if the local interface
and log are to display the application identity. It then calls
[`od_parse_cmd_line()`](../reference/api/od_parse_cmd_line.md), allowing the
sysop to use the normal connection, path, local-mode, node, and diagnostic
options.

Before calling [`od_init()`](../reference/api/od_init.md), the example selects
three optional components and installs two callbacks:

```c
od_control.od_config_file = INCLUDE_CONFIG_FILE;
od_control.od_config_function = CustomConfigFunction;
od_control.od_mps = INCLUDE_MPS;
od_control.od_logfile = INCLUDE_LOGFILE;
strcpy(od_control.od_logfile_name, "vote.log");
od_control.od_before_exit = BeforeExitFunction;
```

[`INCLUDE_CONFIG_FILE`](../reference/constants/components.md#include_config_file)
enables the configuration reader. The
[`od_config_function`](../reference/control/customization.md#od_config_function)
callback receives every nonblank setting after the configuration reader has
processed any matching built-in keyword.
[`INCLUDE_MPS`](../reference/constants/components.md#include_mps) enables the
DOS personality selector; it has no effect on the remote terminal or on
non-DOS local presentation. [`INCLUDE_LOGFILE`](../reference/constants/components.md#include_logfile)
enables the logger, and
[`od_before_exit`](../reference/control/customization.md#od_before_exit)
provides one final application callback during orderly shutdown.

All of these assignments precede initialization because each affects work
performed while OpenDoors is starting. Moving the configuration selector
after the first display call would be too late: that display could implicitly
initialize the library before the selector is seen.

The explicit initialization which follows makes caller information available.
Vote immediately calls `ReadOrAddCurrentUser`, which uses
[`user_name`](../reference/control/caller.md#user_name) as its application user
key. If the record cannot be obtained, the program reports the error and exits
through [`od_exit()`](../reference/api/od_exit.md), preserving normal OpenDoors
cleanup even on this early failure path.

### The main menu

The main loop continues until the caller chooses either return-to-BBS or
hangup. It begins by calling [`od_clr_scr()`](../reference/api/od_clr_scr.md),
then attempts to display an external menu:

```c
chMenuChoice = od_hotkey_menu("VOTE", "VRADPEH", TRUE);
```

[`od_hotkey_menu()`](../reference/api/od_hotkey_menu.md) looks for the suitable
`VOTE` display-file variant and accepts only the listed command characters. Its
final true argument permits the caller to interrupt the file as soon as a
valid choice is pressed. A zero return indicates that no suitable file could
be displayed, so Vote falls back to a menu compiled into the program. This
fallback is important: installing an ANSI or RIP file must not make the door
unusable for a plain-ASCII caller or on a fresh installation with no display
files.

The compiled menu uses color descriptions embedded in
[`od_printf()`](../reference/api/od_printf.md) strings. When ANSI or AVATAR is
available it draws a CP437 horizontal line; otherwise it uses an ASCII hyphen.
The current value of
[`user_timelimit`](../reference/control/caller.md#user_timelimit) is included
in the prompt. [`od_get_answer()`](../reference/api/od_get_answer.md) then
waits until one of the valid letters is entered.

A `switch` dispatches the choice. Vote, view results, and add question are
application functions. Page calls [`od_page()`](../reference/api/od_page.md).
The hangup path asks for confirmation before retaining the hangup choice. At
the end of the loop, Vote calls [`od_exit()`](../reference/api/od_exit.md) with
true only for that confirmed hangup; its ordinary exit message is followed by
a false hangup argument so the caller returns to the BBS.

### Configuration and logging callbacks

`CustomConfigFunction` is invoked for every nonblank configuration line. Vote
ignores keywords it does not use, compares the normalized keyword with
`ViewUnanswered`, and accepts `Yes` or `No` to update its local policy. The
callback does not retain either argument because both point into temporary
parser storage. A larger door should also diagnose invalid values for its own
keywords rather than silently treating them as the existing default.

`BeforeExitFunction` formats the session's vote count and passes it to
[`od_log_write()`](../reference/api/od_log_write.md). OpenDoors invokes this
callback during its shutdown sequence, including shutdown initiated from
inside library processing. That makes it a useful final accounting point, but
the callback must not call the C runtime `exit()` or recursively begin another
OpenDoors shutdown.

Vote also writes log entries when questions are created and other significant
application actions occur. This illustrates the useful division between
automatic component messages—startup, exit, chat, and connection events—and
application messages which explain what the caller actually did inside the
door.

### Caller input and question selection

The question-selection functions use
[`od_input_str()`](../reference/api/od_input_str.md) for short numeric and text
responses, [`od_get_answer()`](../reference/api/od_get_answer.md) for bounded
menu choices, and [`od_get_key()`](../reference/api/od_get_key.md) where any
single byte is acceptable. This is a useful division of responsibility: let
the narrowest suitable input function enforce the basic interaction, then
validate the application meaning after it returns.

Vote tracks a page location while listing questions so that the caller can
move through a set larger than the screen. It never assumes that an entered
number identifies an existing question merely because it fit in the input
buffer. The application still checks its range and reads the corresponding
record before using it.

When it displays results, Vote uses the caller's ANSI and AVATAR capability
fields to choose between CP437 line characters and an ASCII fallback. These
fields describe terminal capabilities, not a guarantee that every arbitrary
escape sequence will work. Code which uses the high-level screen API lets
OpenDoors perform the protocol selection; code which sends a custom terminal
extension must make its own capability and fallback decisions.

### Multi-node file access

Maintained builds define `MULTINODE_AWARE` for Vote. In that mode, startup
configures a shared OpenDoors reservation registry. `ExclusiveFileOpen`
requests `VoteUsers` or `VoteQuestions`, waits for at most 20 seconds, and then
opens the corresponding data file. A manual build which omits the definition
falls back to ordinary `fopen()` and does not provide multi-node exclusion.

The important pattern is broader than the particular wrapper. Vote acquires
exclusive access before searching or modifying a file, performs the complete
read/change/write transaction while ownership is held, and releases access
before returning to caller interaction. Its user lookup cannot safely scan an
unlocked snapshot and lock only for the append: another node could add the
same caller between those operations. Likewise, a question total must be read
again under the same lock which protects the increment.

Vote writes these fixed-size structures one record at a time. For such a call,
`fwrite()` reports success only by returning one; a return of zero is a short
or failed write and must take the error path before the caller is told that the
update succeeded.

The reservation wait services OpenDoors checkpoints while it is blocked, so
callbacks may run during the wait. The application must not pretend it owns a
file before the reservation is acquired, and should keep global state
consistent at those checkpoints. Closing the wrapper releases the named
reservation after closing the data file.

The example's wrappers are demonstrations, not a universal locking API.
Sharing semantics still depend on the mounted filesystem correctly
implementing shared byte-range locks and coherent file I/O. A production
format may instead prefer a database transaction or a single service which
owns the data.

### What to take from the example

Vote's most reusable lesson is its separation of responsibilities. OpenDoors
owns the terminal session, drop-file handling, time, carrier, standard
operator functions, and optional common components. Vote owns poll policy,
records, menus, and validation. The application passes only the information
needed at each boundary and performs orderly shutdown through the library.

Do not copy implementation details merely because they occur in a working
example. In particular, fixed 80-column decoration should be adapted for the
caller's screen, and every file-I/O result needs careful success testing. The
current API pages and the defects recorded in this repository take precedence
over comments inherited by the example source.

## Example programs

The source distribution and separate examples release artifact contain six
doors. Each concentrates on a different part of the API.

### `ex_hello.c`

Hello is the smallest example. It includes the public header, displays a
two-line message, waits for one key, and returns to the BBS without hanging up.
It deliberately relies on implicit initialization, showing that a complete
door can be only a few OpenDoors calls. The tutorial's minimal program adds
standard command-line handling and an explicit initialization call because
those are better starting points once the program needs settings or caller
information before its first output.

### `ex_diag.c`

The diagnostic door has its own parser for launch settings and displays the
resulting connection method, port, drop-file type, terminal capabilities,
caller identity, time, and serial-driver configuration. Its interactive tests
exercise typing, mode autodetection, clearing, carriage return, line feed,
color, and other terminal behavior.

This example is valuable when a door works locally but not beneath a
particular BBS. Run it using the same node directory, account, transport, and
launch command as the failing door, then compare the detected values with what
the BBS intended to pass. The custom parser is diagnostic application code;
ordinary doors should normally begin with
[`od_parse_cmd_line()`](../reference/api/od_parse_cmd_line.md) and add only
their own documented options.

### `ex_chat.c`

Chat implements a split-screen conversation. One window contains the caller's
typing and the other contains the sysop's. It tracks the cursor and current
word independently for both participants, wraps words at the window edge, and
uses [`od_scroll()`](../reference/api/od_scroll.md) when either half reaches
its bottom row. [`od_last_input`](../reference/control/runtime.md#od_last_input)
selects the window corresponding to the source of the most recent byte.

The file can be compiled as a stand-alone chat door or adapted as a replacement
for the built-in chat interface. In replacement mode it installs a before-chat
callback, saves the existing screen, suppresses the normal chat loop, and
restores the screen on completion. Its shell callbacks create and remove a
popup notice while the local command interpreter is active.

The example predates the size-aware screen-save API and its layout is fixed for
an 80-column terminal. New code should use
[`od_save_screen_size()`](../reference/api/od_save_screen_size.md),
[`od_save_screen_ex()`](../reference/api/od_save_screen_ex.md), and
[`od_restore_screen_ex()`](../reference/api/od_restore_screen_ex.md), and
should derive window geometry from the remote screen fields. The chat logic
remains a useful illustration of simultaneous local and remote input.

### `ex_music.c`

Music demonstrates an extension historically implemented by some ANSI
terminal programs. It sends the extension with
[`od_disp()`](../reference/api/od_disp.md) and disables local echo for those
bytes so the BBS machine does not play or display the remote-only sequence. It
first asks the caller whether a short test produced sound, then plays a sample
only when the caller confirms support.

The example is also a warning against inferring every extension from the
general ANSI flag. ANSI music is not part of the basic cursor-and-color
capability represented by
[`user_ansi`](../reference/control/caller.md#user_ansi), and many current
terminals do not implement it. Probe with consent, retain a silent path, and do
not send a potentially disruptive sequence solely because ANSI display is
enabled.

### `ex_ski.c`

Ski is a real-time terminal game. It draws a slalom course, polls for steering
input, advances the game on a timer, narrows and moves the course, and stores a
high-score table. Its main loop demonstrates the difference between an
ordinary blocking menu and gameplay which must continue when no key is ready.

The example uses [`od_sleep()`](../reference/api/od_sleep.md) for portable
timing. Its score file illustrates read/modify/write locking, but unlike Vote's
fixed encoding it directly stores C structures tied to the producing ABI. The
hard-coded terminal art also assumes a traditional CP437 display. Preserve a
text-mode fallback and define an explicit disk encoding before using the design
in a door expected to share files across targets.

### `ex_vote.c`

Vote is the poll door examined above. It combines menus, caller records,
configuration callbacks, logging, external display files, session callbacks,
shared data, and multi-node locking. Its greater size makes it the most useful
example for studying how OpenDoors fits around an application's own data and
control flow.

All six examples build using only the installed OpenDoors SDK. The examples
package contains its own CMake project and documents the `Shared`, `Static`,
and MSVC `StaticMT` selections.

The DOS release builds the examples which are compatible with each DOS target.
The personality sources are also built against only the public personality SDK
headers, making them suitable starting points for a custom DOS local interface.

Read the examples as demonstrations of individual techniques, not as a reason
to copy every historical design choice. In particular, use the current API
reference for buffer sizes, error handling, platform support, and behaviors
which have changed since those programs were first written.

## Preparing a door for distribution

A door which works from a developer's build directory is not yet necessarily
ready for a BBS installation. The installed files, launch command, writable
paths, terminal assumptions, and failure behavior form part of the door's
interface to the sysop.

### Program and run-time files

Ship the executable built for the intended operating system and architecture.
A shared Windows build also requires its matching OpenDoors DLL; a Unix-like
installation must make its shared library discoverable through the normal
system or application run-time search path. A statically linked door includes
OpenDoors code in the executable, but may still depend upon the platform C
runtime or other libraries used by the application. Test the package on a
machine which does not contain the source tree or build directory.

Include every configuration, help, menu, and data file which the door expects,
along with a plain-text display fallback. File names on DOS may be limited to
the 8.3 form and treated without case distinctions, while a Unix-like host has
case-sensitive names and paths. A reference to `VOTE.ANS` which succeeds on a
case-insensitive development system is not proof that `vote.ans` will be found
on every target.

Do not install a sample drop file as though it were permanent caller data. The
BBS writes its node-specific file before each launch. A sample such as the
repository's `DORINFO1.DEF` belongs in a test setup and should not overwrite or
mask the live BBS record.

### Working and data directories

Document whether relative paths are resolved from the node directory, the
door's program directory, or a path selected in configuration. BBS software
does not universally launch a door with the executable's directory as the
current working directory. Use an explicit data directory when persistent
files must live in one shared location, and keep per-node temporary files in a
node-specific location.

For multi-node operation, decide which files are immutable, which are private
to one node, and which require serialized updates. Ensure that a failed or
interrupted write cannot silently replace a valid data file with a partial
record. A robust update may write a complete temporary file, flush it, and
replace the old file using the guarantees of the target filesystem; a record
database may need a different transaction protocol. OpenDoors does not impose
an application data format or make ordinary C file I/O multi-node safe.

Permissions are also part of deployment. The account running the BBS needs
read access to the program and display files and write access only where logs,
scores, votes, or configuration changes require it. Failure to open a data
file should produce an operator-useful message and a controlled return to the
BBS, not an unchecked null pointer or a hang which consumes the node.

### Launch documentation

Give the sysop an exact launch example for each supported BBS interface. State
which drop-file format or handle is expected, how the node or drop-file path is
passed, which working directory is required, and which exit levels the BBS may
interpret. If the standard OpenDoors parser is used, link or reproduce the
relevant option names without inventing abbreviated aliases; similarly named
historical doors are not evidence that an option exists in this program.

Document the distinction between returning to the BBS and hanging up. A menu
label such as “Exit” should make clear whether it preserves the caller's BBS
session. Reserve connection termination for a confirmed logoff action, and
exercise both paths beneath the BBS rather than only in local mode.

### Test matrix

At minimum, test the door in local mode, through each claimed connection
method, and with plain ASCII and ANSI terminals. When supported, include AVATAR
and RIP tests. Exercise a narrow or nontraditional screen as well as 80 columns,
the last row and column, an expired or nearly expired time limit, loss of the
connection during input and output, empty and maximum-length caller fields,
missing optional files, and simultaneous nodes updating shared data.

Build warnings deserve attention on every supported compiler. The 16-bit DOS
build can reveal assumptions hidden by flat 64-bit hosts: automatic objects
which exceed a segment, pointer truncation, unavailable C library functions,
and accidental dependence upon a modern language feature. Conversely, a
64-bit build exposes assumptions that a pointer, file offset, or time value
fits in an `int` or `long`. Platform-specific implementations may use modern C
behind the appropriate platform guards, but the public declarations and code
shared with Borland or Microsoft DOS compilers must remain acceptable to those
tools.

Finally, run the packaged door long enough to trigger periodic status, time,
and inactivity processing, and verify the log and rewritten drop file after
shutdown. A successful greeting screen proves only that initialization and one
output path worked; it does not validate the complete session lifecycle.

## Where to continue

- [Session lifecycle](session-lifecycle.md) explains initialization and
  shutdown ordering.
- [Building and linking](building.md) covers every supported toolchain and
  library variant.
- [Command-line processing](../reference/api/od_parse_cmd_line.md) lists the
  standard launch options.
- [Terminal and screen model](terminal-screen.md) explains local, remote, and
  virtual presentation.
- [Configuration files](configuration.md), [Logging](logging.md), and
  [Personality modules](personalities.md) cover the optional subsystems.
- The [function reference](../reference/api/index.md),
  [`od_control` reference](../reference/control/index.md), and
  [constants reference](../reference/constants/general.md) are the authoritative
  interface descriptions.
