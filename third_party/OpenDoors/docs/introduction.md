# Introduction to OpenDoors

Welcome to OpenDoors! OpenDoors is a powerful and easy-to-use online software
programming toolkit for C and C++. It is most often used to create add-on
"door" programs for bulletin-board systems, but its connection, terminal,
input, and session services can also be used by other interactive online
software.

OpenDoors supplies the common operating environment which would otherwise
have to be implemented separately by every door. A program can write to the
caller and local operator through one interface, receive translated input from
either side, read caller and system information from established drop-file
formats, enforce time and connection policies, and provide the local status
and operator controls expected of a traditional door. The application remains
free to replace or disable these services when it needs tighter control.

The public interface is deliberately small. Applications include
[`OpenDoor.h`](reference/api/index.md), configure the global
[`od_control`](reference/control/index.md) structure, and call the
[`od_*`](reference/api/index.md) functions. DOS personality modules use the
additional [`ODStat.h`](reference/personality/index.md) interface. The same
application-level API is maintained across current hosts and the supported DOS
targets, subject to the platform-specific facilities described throughout this
manual.

OpenDoors supports current Windows, macOS, and Unix-like systems, along with
16-bit and 32-bit DOS builds using Open Watcom and source compatibility with
the established Borland and Microsoft DOS compilers. Current host builds can
produce static and shared libraries. The DOS build remains separate so that
its memory models, calling conventions, assembler requirements, personalities,
and extender choices do not distort the modern build.

The following sections describe the facilities on which an OpenDoors program
can build.

## Door programming without transport-specific code

The core display and input functions allow a door to be written much like an
ordinary interactive C program. Output is sent through functions such as
[`od_printf()`](reference/api/od_printf.md),
[`od_disp_str()`](reference/api/od_disp_str.md), and
[`od_send_file()`](reference/api/od_send_file.md). Input is received through
[`od_get_key()`](reference/api/od_get_key.md),
[`od_get_input()`](reference/api/od_get_input.md), the single-line editors, or
the multi-line editor. OpenDoors performs the required local and remote work
behind those calls.

The connection may be a DOS FOSSIL driver, the traditional direct serial path,
a Win32 serial handle, a Door32 handle, a TCP socket, or standard input and
output. The door normally does not need a separate display or input loop for
each method. Connection details are normalized in
[`od_control`](reference/control/connection.md), and the rest of the program
uses the high-level API.

OpenDoors watches the active connection, runs the session timers, and processes
local operator controls while the application is using the API. During a long
calculation which makes no OpenDoors calls, the application calls
[`od_kernel()`](reference/api/od_kernel.md) so that the same services continue
to run. Carrier checking, time-limit enforcement, inactivity warnings, and
automatic termination can each be configured or disabled when the application
has its own policy.

Local mode is a first-class operating mode rather than a simulation which
requires a modem. It can be selected explicitly for development, diagnostics,
or software which uses OpenDoors' terminal and operator facilities without a
remote connection.

## Terminal and screen support

OpenDoors understands plain ASCII output, ANSI, AVATAR, and the RIP capability
reported by the calling system. It tracks the caller's screen separately from
the local presentation, so a remote terminal whose width or height differs
from the local console can still use cursor positioning, block operations,
scrolling, windows, and size-aware screen snapshots correctly.

Applications can select foreground and background colors, set a complete
IBM-PC text attribute, move or query the cursor, clear the screen or current
line, and draw or restore rectangular blocks. Higher-level functions provide
bordered windows, pop-up menus, file lists, and display-file output. The
size-aware screen snapshot functions save all cells and the associated window,
cursor, attribute, and scrolling state without imposing the original fixed
screen limit.

[`od_printf()`](reference/api/od_printf.md) combines normal formatted output
with OpenDoors color directives. [`od_disp_emu()`](reference/api/od_disp_emu.md)
passes a string through the terminal emulator. Display files can be maintained
as `.ASC`, `.ANS`, `.AVT`, and `.RIP` variants; OpenDoors selects the most
capable file suitable for the caller and provides an appropriate local
presentation. A section-oriented form allows several named screens to share
one file.

The terminal model is not limited to output. OpenDoors translates common ANSI,
DoorWay, and control-key representations into stable extended-key values.
Applications which need byte-for-byte input can request a raw mode, while
editors and menus can use translated arrows, function keys, Insert, Delete,
Home, End, Page Up, Page Down, and Shift-Tab.

## Door-information and caller data

BBS software normally starts a door with a small door-information or "drop"
file. OpenDoors recognizes the established DORINFO, EXITINFO, CHAIN.TXT,
SFDOORS.DAT, CALLINFO.BBS, DOOR.SYS, TriBBS, and Door32 forms. It converts the
available information into the common [`od_control`](reference/control/index.md)
fields rather than requiring the application to understand every source
layout.

Those fields cover the system and operator, caller identity and location,
security and accounting values, time limits, terminal capabilities, message
and file areas, event state, and format-specific information supplied by
RemoteAccess and related systems. Not every drop-file format contains every
value; each field's reference explains its availability, default, mutability,
and valid lifetime.

The configuration component can define additional line-oriented drop-file
formats. This permits a door to run under software whose file format is not
built into OpenDoors, without recompiling the library. Socket and inherited
handle operation allow current servers to provide a connection without a
traditional serial port.

## Configuration and optional components

The [`od_control`](reference/control/index.md) structure is also the main point
of customization. Before initialization, a program can select its component
set, connection policy, drop-file location, program identity, buffers, timeouts,
callbacks, text, colors, keys, and display behavior. After initialization, the
same structure reports live caller and runtime state.

The configuration-file component reads standard system, modem, paging, color,
logging, and drop-file options. An application callback can recognize
program-specific keywords so one configuration file describes both OpenDoors
and the door itself. The standard command-line parser similarly recognizes the
OpenDoors options and can pass application options to custom handlers.

The logging component maintains a FrontDoor-style activity log. OpenDoors can
record session entry and exit, paging and chat activity, shell use, timeouts,
and other common events, while [`od_log_write()`](reference/api/od_log_write.md)
adds application-specific records. The log filename, message text, and logging
policy are configurable.

The DOS multiple-personality system controls the local status display and
operator key arrangement. Standard, PCBoard, RemoteAccess, and Wildcat
personalities are supplied. A door can install another personality through the
public DOS personality SDK, giving it the same ability to define status lines,
respond to status updates, and install local keys as the supplied modules.

## Operator interaction

OpenDoors provides the familiar local facilities expected while a caller is
inside a door. The operator can enter chat, adjust the caller's remaining time,
toggle the caller keyboard, select status displays, request a return to the
BBS, hang up, lock out the caller, or reserve the system for the operator's
next use. The default keys can be replaced, disabled, or extended with
application-defined local actions.

Chat and external-command operations have before-and-after callbacks so the
door can save application state, release resources, or redraw its display.
The paging function can observe configured paging hours, ask the caller for a
reason, sound the local alert, and enter chat when answered. Prompt strings,
response keys, colors, and status text can be localized or replaced.

Applications are not forced to expose these facilities. The status subsystem,
local input, operator keys, shell support, automatic carrier handling, and
other pieces can be selected independently. This allows OpenDoors to serve
both a conventional BBS door and a program with a deliberately different
local interface.

## Input, menus, and editing

For simple prompts, OpenDoors can accept one character from a set of valid
answers, read a key with optional waiting, or input a bounded string with
backspace editing. The formatted single-line editor adds insert and overwrite
modes, cursor movement, character classes, literal text, automatic case
conversion, field navigation, password display, validation, and automatic
acceptance.

The multi-line editor operates on application-supplied storage and supports
paragraph or line-oriented text, word wrapping, scrolling, deletion, and a
custom editor menu. Its options describe the editing rectangle, format, line
breaks, menu callback, and application-specific behavior.

Popup menus provide a movable highlighted selection bar, cancellation,
pull-down operation, and control over whether the menu remains visible after a
selection. Hotkey menus combine display-file output with caller input. Window
functions preserve the screen below a bordered region and restore it when the
window is removed.

## Files, subprocesses, and multi-node programs

OpenDoors can display terminal files, list files from FILES.BBS-style indexes,
run another program while suspending door services appropriately, and maintain
the activity log. The extended spawn interface supplies explicit argument and
environment arrays and supports waiting or non-waiting operation where the
platform permits it.

The library itself is suitable for multi-node use, but application data must
also be protected. A door which updates shared records needs platform-appropriate
file sharing or locking so that two nodes cannot overwrite one another's
changes. Configuration paths, log files, temporary files, and node-specific
resources likewise need names or locking arrangements suitable for concurrent
instances.

## Examples and reference material

The repository includes example doors covering a minimal session, diagnostics,
chat, music, skiing, and a voting application. They demonstrate both the basic
API sequence and larger combinations of screen, input, data-file, and session
services. All six examples build with OpenDoors alone.

The remainder of this manual documents every public function, control field,
type, callback, constant, macro, compatibility alias, and DOS personality
interface in the shipped headers. The guides explain complete workflows;
reference pages specify parameters, return values, state changes, errors,
interactions, restrictions, and examples. Together they are intended to be
sufficient for writing and maintaining an OpenDoors application without a
second API manual.
