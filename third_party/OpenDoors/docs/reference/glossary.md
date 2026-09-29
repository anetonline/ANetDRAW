# Glossary

This glossary defines terminology used by OpenDoors, door programs, and the
BBS environments in which they operate.

[ANSI](#ansi) · [API](#api) · [ASCII](#ascii) · [AVATAR](#avatar) ·
[baud](#baud) · [BBS](#bbs) · [bit-mapped flags](#bit-mapped-flags) ·
[Boolean values](#boolean-values) · [BPS](#bps) ·
[calling convention](#calling-convention) · [carrier detect](#carrier-detect) ·
[chat mode](#chat-mode) · [CP437](#cp437) · [DPMI](#dpmi) ·
[DOS extender](#dos-extender) ·
[compile](#compile) · [DLL](#dll) · [door](#door) ·
[door-information file](#door-information-file) · [DTR](#dtr) ·
[echo](#echo) · [FOSSIL driver](#fossil-driver) ·
[import library](#import-library) · [library](#library) · [link](#link) ·
[local mode](#local-mode) · [local echo](#local-echo) ·
[locked BPS rate](#locked-bps-rate) · [log file](#log-file) ·
[memory model](#memory-model) · [modem](#modem) ·
[object file](#object-file) · [online](#online) ·
[output window](#output-window) · [page](#page) · [parameter](#parameter) ·
[remote](#remote) · [RIP](#rip) · [source code](#source-code) ·
[socket](#socket) · [status line](#status-line) · [sysop](#sysop) ·
[sysop chat](#sysop-chat) · [sysop page](#sysop-page) · [user](#user) ·
[Win32](#win32)

## ANSI

ANSI is an acronym for *American National Standards Institute*. In BBS usage,
“ANSI” ordinarily means the family of terminal-control sequences derived from
ANSI X3.64 and compatible DEC terminal behavior. These sequences allow a BBS
or door to select colors, position the cursor, erase portions of the display,
and perform other screen operations.

The expression “ANSI graphics” is traditional BBS terminology. It describes
the greater visual control available compared with an ASCII TTY display; it
does not imply a bit-mapped or vector graphics protocol. OpenDoors translates
its screen operations into ANSI sequences when the caller's terminal supports
them. Compare [ASCII](#ascii), [AVATAR](#avatar), and [RIP](#rip).

## API

API is an acronym for *application programming interface*. An API is the set of
documented functions, variables, types, constants, and calling conventions
through which one program uses services supplied by another component.

For example, a C program uses the C library API when it calls `fopen()` or
`strcpy()`. A door uses the OpenDoors API when it calls
[`od_printf()`](api/od_printf.md) or [`od_get_key()`](api/od_get_key.md), or
when it reads or sets documented members of [`od_control`](control/index.md).
The public declarations for the OpenDoors door API are provided by
[`OpenDoor.h`](api/index.md); the DOS personality SDK is separately declared
by [`ODStat.h`](personality/index.md).

## ASCII

ASCII, pronounced “ass-key,” is an acronym for *American Standard Code for
Information Interchange*. The standard assigns values to 128 control
characters, letters, digits, punctuation marks, and symbols.

In BBS terminology, *ASCII mode* also means that the remote terminal is being
treated as a sequential teletype display rather than as an ANSI- or
AVATAR-capable screen. Text can be sent in order, but OpenDoors cannot rely on
remote color, cursor positioning, or selective erasure. Screen-oriented
functions therefore use the plain-text behavior documented for those
functions. Compare [ANSI](#ansi) and [AVATAR](#avatar).

## AVATAR

AVATAR is an acronym for *Advanced Video Attribute Terminal Assembler and
Recreator*. It is a terminal display protocol developed for BBS use. Like
ANSI, it supports cursor positioning, color selection, erasure, and other
screen operations. Its compact commands were intended to perform common BBS
display operations using fewer transmitted bytes than equivalent ANSI
sequences.

AVATAR is less widely implemented than ANSI and is distinct from ordinary
ASCII output. OpenDoors can select the appropriate representation after
learning the caller's capabilities. Compare [ANSI](#ansi) and [ASCII](#ascii).

## Baud

*Baud* is the number of signaling symbols transmitted per second. A symbol can
represent more than one bit, so baud and [BPS](#bps) are not generally
interchangeable technical units. Older modem and BBS documentation often uses
“baud rate” informally when it means bits per second; some OpenDoors field and
option names retain that established terminology for source compatibility.

## BBS

BBS is an acronym for *bulletin board system*. A BBS accepts local or remote
sessions and provides services such as messages, file transfer, chat, and door
programs. Historically, callers commonly reached a BBS through a modem and a
serial connection. Present-day systems may instead carry the terminal session
over Telnet, RLogin, SSH, WebSocket, or another network transport while still
presenting the same door interface.

## Bit-mapped flags

A bit-mapped flag variable stores several independent true-or-false states in
one integer. Each bit represents one condition. A set bit represents the
enabled or true state; a clear bit represents the disabled or false state.

For example, an illustrative one-byte `user_info` value might assign its three
low bits as follows:

```text
Bit:  7 6 5 4 3 2 1 0
                | | |
                | | +--- ANSI graphics
                | +----- Screen clearing
                +------- More prompts
```

Named masks make those assignments usable without embedding their numeric
positions throughout a program:

```c
#define ANSI_GRAPHICS   0x01
#define SCREEN_CLEARING 0x02
#define MORE_PROMPTS    0x04
```

Use bitwise operators to set, clear, and test an individual flag without
altering the other bits:

```c
user_info |= ANSI_GRAPHICS;       /* Set a flag. */
user_info &= ~SCREEN_CLEARING;    /* Clear a flag. */

if(user_info & MORE_PROMPTS)      /* Test a flag. */
{
    /* Wait for Return. */
}
```

The masks used by actual OpenDoors fields are defined by
[`OpenDoor.h`](api/index.md) and documented with the fields or constants to
which they apply. A program should use those names rather than assuming a bit
position from the storage type.

## Boolean values

A Boolean value represents one of two states, conventionally called true and
false. OpenDoors uses the [`BOOL`](types.md#bool) type and the
[`TRUE`](constants/general.md#true) and
[`FALSE`](constants/general.md#false) constants because the supported C
compilers do not all provide the same built-in Boolean type.

Assignment may use the named constants:

```c
wait = TRUE;
```

When testing a Boolean value, test whether it is zero rather than requiring it
to equal one particular nonzero representation. Any nonzero value is true in
a C conditional expression:

```c
if(wait)
{
    /* The value is true. */
}

if(!wait)
{
    /* The value is false. */
}
```

This rule matters for functions and fields whose true result may be any
nonzero value, even when [`TRUE`](constants/general.md#true) itself has a
specific numeric definition.

## BPS

BPS is an acronym for *bits per second*. It measures the rate at which bits are
transferred over a communications channel.

Traditional modem operation has two relevant rates. The serial-port or DTE
rate is the rate between the BBS computer and its modem. The connection or DCE
rate describes the modem link. A modem using compression can move data across
the serial port faster than the nominal connection rate, so the port is often
configured at a fixed, higher rate. OpenDoors stores its communication rate in
[`baud`](control/connection.md#baud) and, when the source format supplies one,
the reported connection rate in
[`od_connect_speed`](control/connection.md#od_connect_speed). See also
[baud](#baud) and [locked BPS rate](#locked-bps-rate).

## Calling convention

A calling convention is the part of an application binary interface which
defines how a function call passes arguments, returns a result, preserves
registers, and names its externally visible symbol. Source code can contain a
valid declaration and still fail to link or execute if the caller and library
compiled that declaration using different conventions.

This distinction is visible in OpenDoors' DOS32 libraries. Open Watcom's `-3r`
mode passes eligible arguments in registers, while `-3s` uses stack calls.
`ODOOR32R.lib` and `ODOOR32S.lib` provide the matching builds. Every object in
the door, and every callback invoked through the API, must use the convention
selected by the library. Hosted CMake targets and
[`OpenDoor.h`](api/index.md) propagate the required definitions for supported
current toolchains.

A calling convention is separate from a [memory model](#memory-model), although
both are ABI choices. It is also separate from whether the implementation is
stored in a static or shared [library](#library).

## Carrier detect

Carrier detect is the signal by which a serial modem or communications driver
reports that a remote connection is present. OpenDoors monitors that state so
that loss of the call can terminate the door cleanly instead of leaving it to
wait indefinitely for further input.

With a socket transport there is no physical modem carrier, but connection
closure serves the same purpose. In [local mode](#local-mode), OpenDoors can
operate without a remote carrier.

## Chat mode

Chat mode allows the sysop and caller to communicate interactively. Text typed
at the local keyboard is displayed to the caller, and text typed by the caller
is displayed locally. The sysop may enter chat directly with the configured
local key or in response to a [sysop page](#sysop-page). OpenDoors supplies the
chat interface and provides callbacks and replacement text through the
[customization fields](control/customization.md).

## CP437

CP437 is the original IBM PC character set. It assigns byte values to control
characters, letters, accented characters, mathematical symbols, and the line-
and box-drawing glyphs traditionally used by DOS BBS software. Its low 128
positions largely follow ASCII, while the display of the upper half depends
upon use of the CP437 mapping.

OpenDoors color attributes and default border bytes use the IBM PC text-mode
model. A terminal configured for CP437 can display those bytes as intended. A
UTF-8 connection cannot receive an arbitrary high CP437 byte and infer the
same glyph; a gateway or application must perform an encoding conversion.
Likewise, a plain-ASCII fallback should use characters such as `+`, `-`, and
`|` rather than depending upon CP437 corners and lines.

## DPMI

DPMI is an acronym for *DOS Protected Mode Interface*. It defines services by
which a protected-mode DOS program obtains memory, manages descriptors,
handles interrupts, and invokes real-mode services under a compatible host.
The interface allows a 32-bit application to use a flat protected-mode address
space while still cooperating with DOS and real-mode drivers.

Pointers in that flat address space cannot simply be passed to a real-mode
interrupt which expects a 16-bit segment and offset. The DOS32 FOSSIL
implementation therefore obtains conventional memory through DPMI, copies
block data through that buffer, and presents its real-mode address to the
driver. If a transfer buffer is unavailable, it can use FOSSIL operations
which transfer individual bytes. See [DOS extender](#dos-extender) and
[FOSSIL driver](#fossil-driver).

## DOS extender

A DOS extender is run-time support which allows a protected-mode application
to execute while DOS itself and many device interfaces remain in real mode. It
sets up the protected-mode environment, supplies DPMI or equivalent services,
and translates operating-system operations as required. The application can
use flat 32-bit pointers and objects larger than a 16-bit segment, but calls to
real-mode drivers still require an appropriate transition and addressable
transfer storage.

Open Watcom can link OpenDoors DOS32 applications for DOS/4GW or DOS/32A.
Release examples use native DOS/32A executables with the extender bound into
the executable, so a separate extender program need not be installed beside
them. The library ABI is determined by architecture and calling convention;
the supported linker system selects the executable form and extender used by
the completed door.

## Compile

Compiling translates C or C++ source code into machine code and associated
symbol information, usually producing an [object file](#object-file). Building
a complete program normally has at least two stages: the compiler processes
each source file, then the linker combines the resulting object files and
required libraries into an executable or library. In ordinary usage, “compile”
is also used loosely for that entire build process. See [link](#link).

## DLL

DLL is an acronym for *dynamic-link library*, the Windows name for a shared
library. Unlike code selected from a static library and copied into an
executable at link time, DLL code remains in a separate file which the
operating system loads for the program. Programs normally link against an
[import library](#import-library) which describes the DLL's exported symbols.

OpenDoors can be built as a shared or static library on supported modern
platforms. See the [building and installation guide](../guides/building.md) for
the available CMake targets and options.

## Door

A door is an application launched by a BBS to provide a service outside the
central BBS software. Games, voting systems, information services, special
message or file interfaces, and account utilities are common examples. The
caller continues to use the same terminal session, so the transition between
the BBS and door may not be visible to the caller.

The BBS commonly supplies the door with caller, system, connection, and time
information in a [door-information file](#door-information-file) or by command
line. The door returns control to the BBS when it exits.

## Door-information file

A door-information file—also called a *drop file*, *exit file*, or *chain
file*—is written by the BBS before it launches a door. It communicates facts
about the BBS, the caller, the terminal, the connection, and the permitted
session time. Some formats also allow the door to return changed information,
such as elapsed time or an updated caller record, when the door exits.

OpenDoors recognizes formats including `DOOR.SYS`, `EXITINFO.BBS`,
`DORINFO?.DEF`, `SFDOORS.DAT`, `CALLINFO.BBS`, and `CHAIN.TXT`. The formats do
not contain identical information. OpenDoors records the detected format in
[`od_info_type`](control/connection.md#od_info_type), populates the fields
available from that format, and documents format-specific availability with
each field. See the [caller and system information](control/caller.md) and
[connection](control/connection.md) field references.

## DTR

DTR is an acronym for *data terminal ready*. It is a signal driven by a
computer's serial port to indicate that its terminal side is ready. Modems are
commonly configured to hang up when DTR is lowered, making the signal a simple
way for communications software to terminate a call.

OpenDoors' handling of DTR applies to serial communications. Socket and local
sessions do not have a physical DTR circuit. The
[`od_disable_dtr`](control/customization.md#od_disable_dtr) setting controls
whether OpenDoors uses DTR when closing a supported serial connection.

## Echo

In this manual, *echo* generally refers to [local echo](#local-echo). In
communications literature it can also mean returning received characters to
their sender; that separate behavior depends upon the terminal, modem, or
communications protocol in use.

## FOSSIL driver

FOSSIL is an acronym for *Fido/Opus/SEAdog Standard Interface Layer*. A FOSSIL
driver is a DOS resident program or device driver which presents a standard
interrupt interface for serial communications. It insulates BBS and door
software from the details of a particular UART or multiport adapter.

The 16-bit DOS platform can use a FOSSIL driver instead of directly driving a
serial port. The driver must be installed before the BBS or door starts. This
term does not apply to the socket transport used by current hosted platforms.

## Import library

An import library is link-time metadata for a Windows [DLL](#dll). It supplies
the symbol information needed to link calls from an application while leaving
the actual implementation in the DLL loaded at run time. Despite its library
file extension, it does not contain the complete OpenDoors implementation in
the manner of a static library.

## Library

A library is a collection of compiled functions and data intended for use by
other programs. The C runtime library, for example, provides functions such as
`fopen()`, `printf()`, and `atoi()`; the OpenDoors library provides the door
API described by this manual.

A *static library* is an archive of object code from which the linker selects
the portions required by an application. The selected code becomes part of
the resulting executable. A *shared library* remains a separate run-time
component which can be loaded by one or more applications. Windows calls a
shared library a [DLL](#dll) and normally accompanies it with an
[import library](#import-library).

OpenDoors provides separately named static and shared CMake targets where the
platform supports them. DOS builds use static libraries. The exact build and
installation interface is described in the
[building guide](../guides/building.md).

## Link

Linking combines object files and libraries into an executable or another
library. The linker resolves symbolic references—for example, a call to
[`od_printf()`](api/od_printf.md)—to the corresponding compiled definition. It
also arranges code and data into the file format and address layout required by
the target platform. See [compile](#compile), [library](#library), and
[object file](#object-file).

## Local mode

Local mode runs a door without a remote caller connection. Input is obtained
from the local keyboard and output is presented on the machine running the
door. It is useful for sysop access, testing, and diagnosis.

OpenDoors can enter local mode from a door-information file, a command-line
option, or the [`od_force_local`](control/customization.md#od_force_local)
setting. The door should continue to use the normal OpenDoors input and output
functions; application code does not need a separate local user interface.
Compare [remote](#remote).

## Local echo

Local echo is the local presentation of output sent through OpenDoors to the
caller's terminal. OpenDoors first applies output to its authoritative virtual
screen, whose dimensions describe the remote session. A platform-specific
local display may then present that state within the space it has available.

The local and remote screens need not have the same dimensions. Direct writes
to the local display are not transmitted to the caller merely because they
alter the local display, and the local presentation must not be treated as the
storage for remote screen operations. See the
[terminal and virtual-screen guide](../guides/terminal-screen.md).

## Locked BPS rate

A locked BPS rate is a fixed serial-port speed used regardless of the modem's
current connection speed. Modem error correction and compression can make the
data rate between computer and modem greater than the rate across the telephone
connection. Selecting a sufficiently high fixed port speed avoids changing the
UART for every call and prevents the local link from becoming the bottleneck.

The door-information format or command line must report the rate expected by
the serial interface. See [BPS](#bps) and the
[`baud`](control/connection.md#baud) field.

## Log file

A log file is a chronological text record of important session activity. It
allows the sysop to review calls, door events, errors, and abnormal
terminations after the session has ended. OpenDoors' optional logging component
is selected and customized with the fields described under
[customization and callbacks](control/customization.md#od_logfile).

## Memory model

A memory model describes how compiled code addresses instructions and data.
Sixteen-bit DOS C compilers commonly offer tiny, small, compact, medium, large,
and huge models. These choose whether code and data pointers are near or far,
how many segments may be used, and which individual objects can exceed a
single 64-KiB segment. A library and its caller must agree on the relevant
model and calling convention.

Protected-mode 32-bit DOS targets and current hosted operating systems use a
flat address space for ordinary C objects, so application code does not use the
16-bit near, far, and huge distinctions. The address width and executable
format still remain part of the target ABI; a 32-bit and 64-bit library are not
interchangeable merely because both use flat pointers.

## Modem

A modem converts data into a form suitable for a communications circuit and
back again. Dial-up BBS systems normally use modems over telephone lines.
OpenDoors' DOS serial support retains the concepts of modem control signals,
carrier, and port speed; network sessions on hosted platforms provide an
equivalent byte stream without requiring a physical modem.

## Object file

An object file is the compiler or assembler's binary output for one source
translation unit. It contains machine code, data, relocation information, and
symbols which may be defined for or required from other object files.

Object files are not normally executable by themselves. The linker combines
them with other objects and the necessary portions of libraries to produce an
executable or library in the target platform's file format. DOS toolchains
commonly use the `.OBJ` extension; Unix-like toolchains commonly use `.o`.

## Online

In a BBS context, *online* means that a caller is actively using the BBS or a
door. The session may arrive over a modem or network transport, or may be an
operator using [local mode](#local-mode). Thus, online describes the active
session rather than requiring one particular physical connection.

## Output window

The output window is the portion of the local interface used to show the door
session. A status line or other local controls may occupy additional rows.
Historically, DOS doors often used the upper 23 rows of a 25-row text display
for output and the bottom two rows for status; that physical arrangement is not
a limit on the remote terminal.

OpenDoors now maintains the remote screen in its virtual-screen state. The
local output window is a presentation of that state and may be clipped or
otherwise adapted when its dimensions differ. See [local echo](#local-echo),
[status line](#status-line), and the
[terminal-screen guide](../guides/terminal-screen.md).

## Page

In the BBS user-interface sense, *page* means to request the sysop's attention.
See [sysop page](#sysop-page). It may also have its ordinary documentation or
display meaning where the context clearly refers to a page of text.

## Parameter

A parameter is information accepted by a function. In the function
declaration, parameters specify the expected types and names; in a function
call, the corresponding supplied expressions are also called arguments.

For example, [`od_set_color()`](api/od_set_color.md) accepts foreground and
background color parameters:

```c
od_set_color(D_GREEN, D_RED);
```

The first argument selects a dark-green foreground and the second selects a
dark-red background. C encloses the argument list in parentheses and separates
arguments with commas. A function which accepts no arguments has an empty list
in a call, as in:

```c
od_clr_scr();
```

## Remote

A remote caller uses the door through a communications connection rather than
the keyboard and display of the machine running it. The connection may be a
dial-up modem link or a network transport. OpenDoors sends output to and
receives input from that connection while optionally presenting the session to
the sysop through [local echo](#local-echo). Compare [local mode](#local-mode).

## RIP

RIP, RIPScrip, or *Remote Imaging Protocol* is a graphical terminal protocol
developed for BBS systems. Unlike the text-cell display controlled by ANSI or
AVATAR, RIP can describe bit-mapped graphics, lines, shapes, icons, mouse
regions, and related graphical-interface operations.

OpenDoors can identify and send RIP content to a capable remote terminal while
its ordinary text output and local status interface remain text-oriented.
Compare [ANSI](#ansi), [AVATAR](#avatar), and [ASCII](#ascii).

## Socket

A socket is an operating-system endpoint for network communication. To a door,
a connected stream socket can provide the same ordered byte stream once
carried by a serial modem connection, while connection closure serves the
purpose historically provided by carrier detect.

OpenDoors may receive a socket from a BBS through `DOOR32.SYS` or a standard
command-line option. Depending upon the launch interface, the value can be a
native socket or a descriptor or handle supplied by the parent process. The
door must not assume that a borrowed socket can be closed, reopened, or
configured as if it owned an independent serial port. The selected transport
and handle are reported by the [connection fields](control/connection.md).

The byte-stream abstraction does not by itself provide terminal negotiation.
A Telnet connection, for example, can contain Telnet command bytes as well as
terminal data unless the BBS or another layer performs that protocol handling.
The launching system and OpenDoors configuration must agree on which layer
owns such processing.

## Source code

Source code is the human-readable program text from which a compiler or
assembler produces object code. OpenDoors is written principally in C, with
platform-specific source where required. The public repository contains the
library sources, headers, examples, tests, build descriptions, and this
documentation.

## Status line

The status line is the local part of the OpenDoors interface which summarizes
the caller and session and identifies available sysop controls. DOS personality
modules can customize its appearance. It is local operator information, not
part of the remote terminal display, and may be disabled with
[`od_status_on`](control/customization.md#od_status_on).

The status line and output window share a physical local display on platforms
which provide one, but the remote screen remains represented independently.
See [output window](#output-window).

## Sysop

*Sysop* is a contraction of *system operator*. The sysop operates and maintains
the BBS, manages its users and content, and ordinarily has access to the local
keyboard and administrative controls which are not available to callers.

## Sysop chat

Sysop chat is OpenDoors' interactive conversation between the local sysop and
the caller. See [chat mode](#chat-mode).

## Sysop page

A sysop page is a caller's request for the sysop's attention, comparable to
ringing a telephone. The BBS or door normally produces an audible indication
which the sysop may answer by entering [chat mode](#chat-mode). Paging can be
restricted to configured hours or disabled when the sysop is unavailable.

OpenDoors supplies paging behavior and customizable paging text through the
fields documented under [customization and callbacks](control/customization.md).

## User

In general computing usage, a user is any person operating a computer or
program. In BBS usage, *user* or *caller* ordinarily means the person connected
to the BBS to read and write messages, transfer files, chat, or use doors. The
term is often contrasted with [sysop](#sysop), who operates the system.

## Win32

Win32 is the Windows application programming interface introduced for 32-bit
Windows. Much of that API, including its names, data types, and calling
conventions, remains available to native 64-bit Windows applications through
the Windows API. “Win32” in existing OpenDoors identifiers or conditional code
therefore does not necessarily mean that the resulting program is restricted
to a 32-bit processor or address space.

OpenDoors supports current 32- and 64-bit Windows toolchains as described in
the [building guide](../guides/building.md). DOS serial and text-mode platform
interfaces are separate from the Windows implementation.
