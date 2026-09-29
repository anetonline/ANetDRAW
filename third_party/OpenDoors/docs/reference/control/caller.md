# Caller and system information

OpenDoors normalizes several door-information formats into a common set of
fields. String arrays are fixed-size C buffers and are always subject to the
capacity declared in [`OpenDoor.h`](../api/index.md).

Unless an entry says otherwise, the field has static-storage initialization:
numeric and Boolean fields begin at zero, character arrays begin as empty
strings, and pointers begin as `NULL`. During initialization OpenDoors fills
the fields which are available from the selected door-information file. A
field which is not supplied by that format normally retains its initial value.
Use [`od_info_type`](connection.md#od_info_type) when a program depends upon
format-specific information; the presence and precise representation of each
such field are stated below.

Most fields on this page are information for the door to read. OpenDoors also
reads some of them while the session is running, and writes a smaller subset
back to formats which support updates. Each entry identifies those uses. A
door should not alter an informational field merely to change its own local
copy unless the entry specifically documents a write-back effect.

## System events

### `event_days`

```c
BYTE od_control.event_days;
```

This byte is the `EXITINFO.BBS` day mask for the next BBS event:

| Mask | Day |
| ---: | --- |
| `0x01` | Sunday |
| `0x02` | Monday |
| `0x04` | Tuesday |
| `0x08` | Wednesday |
| `0x10` | Thursday |
| `0x20` | Friday |
| `0x40` | Saturday |
| `0x80` | Every day |

The field begins at zero and is populated only when an `EXITINFO.BBS` record
is read. OpenDoors does not interpret it independently, but includes an
application replacement when rewriting that record. See
[bit-mapped flags](../glossary.md#bit-mapped-flags) for the operations used to
test individual bits.

### `event_errorlevel`

```c
BYTE od_control.event_errorlevel;
```

This is the error level associated with the next event in `EXITINFO.BBS`. It
begins at zero and is populated only from that format. The RemoteAccess-style
DOS status personality displays it when the event is enabled; other OpenDoors
code does not interpret the value. Application changes are written back to
`EXITINFO.BBS`.

### `event_force`

```c
BYTE od_control.event_force;
```

This `EXITINFO.BBS` Boolean indicates whether the BBS regards the next event as
forced. A true value means that the caller must be taken offline for the event;
a false value permits the event to wait until the caller leaves normally. It
begins false and is populated only from `EXITINFO.BBS`. OpenDoors does not
enforce the event itself. It preserves an application replacement when the
record is rewritten.

### `event_last_run`

```c
char od_control.event_last_run[9];
```

This is the `EXITINFO.BBS` date on which the event last ran. OpenDoors converts
the source Pascal string, which contains at most eight bytes, to a
null-terminated C string without interpreting its date format. The array is
initially empty and remains empty for other drop-file formats. OpenDoors does
not otherwise use it and writes an application replacement back to
`EXITINFO.BBS`.

### `event_starttime`

```c
char od_control.event_starttime[6];
```

This is the scheduled event time as a five-character, null-terminated string.
It is read from `EXITINFO.BBS` and from the event field of a Wildcat-style
`DOOR.SYS`; the array is initially empty when neither source supplies it.
OpenDoors displays it in the RemoteAccess-style DOS status personality and can
substitute it while displaying a RemoteAccess/QuickBBS file. An application
replacement is written back to `EXITINFO.BBS` and to the supported
Wildcat-style `DOOR.SYS` output.

### `event_status`

```c
BYTE od_control.event_status;
```

This is the event-status byte from `EXITINFO.BBS`. When it equals
[`ES_ENABLED`](../constants/session.md#es_enabled), the associated event
fields describe an enabled event. The byte begins at zero and is populated
only from `EXITINFO.BBS`. The RemoteAccess-style DOS status personality reads
it to decide whether to show event information. Application changes are
written back to that record.

## BBS identity and traffic

### `sysop_name`

```c
char od_control.sysop_name[40];
```

This is the BBS system operator's name. OpenDoors obtains it from
`DORINFO?.DEF`, `DOOR.SYS`, `SFDOORS.DAT`, `CALLINFO.BBS`, `CHAIN.TXT`, a
custom door-information format, or a command-line/configuration override. If
no source supplies a name, initialization changes the initial empty string to
`"Sysop"`.

OpenDoors displays the name on its local interface and exposes it to
applications. It writes an application replacement to formats whose output
contains the sysop name, including `DORINFO?.DEF` and the supported
`DOOR.SYS` variants.

### `system_calls`

```c
INT32 od_control.system_calls;
```

This is the BBS-wide call count stored in `EXITINFO.BBS`. It begins at zero and
is populated only from that format. The RemoteAccess-style DOS status
personality and the RemoteAccess/QuickBBS display-file substitutions may show
it as an unsigned decimal count. OpenDoors does not increment it. Application
changes are written back to `EXITINFO.BBS`.

### `system_last_caller`

```c
char od_control.system_last_caller[36];
```

This is the previous caller's name from `EXITINFO.BBS`, converted from a Pascal
string of at most 35 bytes to a null-terminated C string. The array is empty
when that format is not available. The RemoteAccess-style DOS status
personality and display-file substitutions may show it. An application
replacement is written back to `EXITINFO.BBS`.

### `system_last_handle`

```c
char od_control.system_last_handle[36];
```

This is the previous caller's handle from the RemoteAccess 2.x extended
`EXITINFO.BBS` record, converted from a Pascal string of at most 35 bytes. It
is initially empty and remains so for formats which do not supply the field.
OpenDoors does not display or interpret it and writes an application
replacement only to that extended record.

### `system_name`

```c
char od_control.system_name[40];
```

This is the BBS name. It may be populated from every supported text drop-file
family, a custom door-information format, or a command-line/configuration
override. On Unix, if the field is still empty after those sources have been
processed, OpenDoors uses the host name. Otherwise an unavailable value remains
the initial empty string.

OpenDoors displays the name on its local interface and exposes it to
applications. It writes an application replacement to formats whose output
contains the system name, including `DORINFO?.DEF` and supported `DOOR.SYS`
variants.

### `timelog_start_date`

```c
char od_control.timelog_start_date[9];
```

This is the `EXITINFO.BBS` traffic-log starting date, exposed as an
eight-character, null-terminated string in the representation stored by the
BBS record. The array is initially empty and is populated only from an
`EXITINFO.BBS` format which contains the traffic log. OpenDoors does not parse
or display the date. An application replacement is included when that record
is rewritten.

## Caller record

### `user_ansi`

```c
BYTE od_control.user_ansi;
```

This Boolean states whether the caller's terminal is to receive ANSI display
sequences. It begins false, then may be populated by the door-information file,
a custom format, a command-line override, or terminal autodetection. Formats
which do not report terminal capabilities leave the effective value selected
by those other sources.

OpenDoors reads the field whenever it selects ANSI, AVATAR, or plain-text
output and displays the mode on supported local status interfaces. An
application may change it after initialization to record a caller's selection.
The new value affects subsequent output and is written back only to
door-information formats whose caller record contains a corresponding
graphics-mode setting.

Whether the connection can carry eight-bit character data is a separate
capability available through [`od_get_user_8bit()`](../api/od_get_user_8bit.md)
and [`od_set_user_8bit()`](../api/od_set_user_8bit.md). It is not a member of
`tODControl`, so applications can use it without changing this structure's
ABI.

### `user_attribute`

```c
BYTE od_control.user_attribute;
```

This is the first `EXITINFO.BBS` caller-attribute byte:

| Mask | Meaning when set |
| ---: | --- |
| `0x01` | Caller record is deleted |
| `0x02` | Screen clearing is enabled |
| `0x04` | End-of-page prompts are enabled |
| `0x08` | ANSI mode is enabled |
| `0x10` | The caller has the BBS “no kill” setting |
| `0x20` | Transfer priority is enabled |
| `0x40` | Full-screen message editor is enabled |
| `0x80` | Quiet mode is enabled |

Before reading a non-custom format, OpenDoors initializes the byte to `0x06`,
enabling screen clearing and page prompts. `EXITINFO.BBS` replaces the complete
byte; custom-format settings can set the two presentation bits. OpenDoors reads
`0x02` for clear-screen policy, `0x04` for listing pauses and display-file
substitutions, and derives [`user_ansi`](#user_ansi) from `0x08` after reading
`EXITINFO.BBS`. The entire byte is written back to supported `EXITINFO.BBS`
records. See [bit-mapped flags](../glossary.md#bit-mapped-flags).

### `user_attrib2`

```c
BYTE od_control.user_attrib2;
```

This is the second `EXITINFO.BBS` caller-attribute byte:

| Mask | Meaning when set |
| ---: | --- |
| `0x01` | User hot keys are enabled |
| `0x02` | AVATAR mode is enabled |
| `0x04` | Full-screen message reader is enabled |
| `0x08` | Caller is hidden from the user list |

The byte begins at zero and is populated only from `EXITINFO.BBS`. OpenDoors
derives [`user_avatar`](#user_avatar) from `0x02` and uses several bits in
RemoteAccess/QuickBBS display-file substitutions; it does not otherwise enforce
the BBS preferences. Application changes are written back to supported
`EXITINFO.BBS` records.

### `user_attrib3`

```c
BYTE od_control.user_attrib3;
```

This is the third caller-attribute byte in a RemoteAccess 2.x extended
`EXITINFO.BBS` record. OpenDoors copies the byte without defining or
interpreting its individual BBS-owned bits. It begins at zero, is populated
only from that extended format, and is written back to the same format after
any application change. Consult the applicable RemoteAccess record definition
before testing a bit.

### `user_avatar`

```c
BYTE od_control.user_avatar;
```

This Boolean states whether OpenDoors may use AVATAR terminal commands. It
begins false and is populated from formats which report AVATAR, including the
second attribute byte of `EXITINFO.BBS`; a custom format or application may
also set it. OpenDoors reads the field when selecting its output protocol and
when drawing supported local status displays.

An application may change the value after initialization when the caller
selects a terminal mode. The change affects subsequent output. It is reflected
in BBS writeback only when the active record has a corresponding AVATAR flag.

### `user_birthday`

```c
char od_control.user_birthday[9];
```

This variable is a string, in the same format as the od_control.user_lastcall variable, which stores the date of the user's birthday, if it is available. This variable is only available under systems that produce an RA 1.00 and later style extended EXITINFO.BBS or Wildcat style DOOR.SYS file.

### `user_callsign`

```c
char od_control.user_callsign[12];
```

This variable is a string which contains the user's amateur radio call sign, if any. This variable is only available under systems that produce a CHAIN.TXT file.

### `user_combinedrecord`

```c
BYTE od_control.user_combinedrecord[200];
```

This array is the BBS caller record's combined-message-area bitmap. Bit zero
of element zero represents the first message area, bit one the second, and so
forth; a set bit enables that area for combined access. The interpretation of
combined access itself belongs to the producing BBS.

The array begins filled with zero bytes. OpenDoors copies all 200 bytes from a
RemoteAccess 2.x extended `EXITINFO.BBS` record and copies the 25-byte
QuickBBS/older extended form into the beginning of the array, leaving the
remainder zero. OpenDoors does not interpret the bits. It writes the applicable
200- or 25-byte portion back to the same format. The compatibility name
[`ra_combinedrecord`](../compatibility.md#renamed-control-fields) designates
this same field. See
[bit-mapped flags](../glossary.md#bit-mapped-flags).

### `user_comment`

```c
char od_control.user_comment[81];
```

This variable is a string which contains the sysop's comment about the user that is currently online. This comment may be displayed on the OpenDoors status line, if this variable is available. This variable is available under systems that produce an RA 1.00 and later style extended EXITINFO.BBS or Wildcat style DOOR.SYS file.

### `user_credit`

```c
INT32 user_credit; /* physical member in tODControl */
```

`tODControl` physically contains this member, but after the structure
declaration [`OpenDoor.h`](../api/index.md) defines the source-compatibility macro
`user_credit` as `user_net_credit`. Consequently, ordinary source such as
`od_control.user_credit` is preprocessed into
`od_control.user_net_credit` and does not access this physical [`INT32`](../types.md#int32) member.

The current drop-file readers and writers do not populate or consume the
physical member. It remains part of the structure layout for binary
compatibility and should not be addressed by undefining the compatibility
macro. Use [`user_net_credit`](#user_net_credit) for the NetMail credit value
handled by OpenDoors.

### `user_dataphone`

```c
char od_control.user_dataphone[16];
```

This string contains the user's data or business phone number, if available. This value is only available under system that produce EXITINFO.BBS, PC-Board/GAP style DOOR.SYS and WildCat DOOR.SYS format door information files.

### `user_deducted_time`

```c
INT16 od_control.user_deducted_time;
```

This variable contains a signed integer value, which indicates the total amount of time that has been deducted from the user during this call. This variable is only available under systems that produce an RA 1.00 and later style extended EXITINFO.BBS door information file.

### `user_downk`

```c
DWORD od_control.user_downk;
```

This variable contains the total kilobytes of files that the current user has downloaded from the BBS, and is available under systems that produce EXITINFO.BBS, Wildcat style DOOR.SYS or SFDOORS.DAT format door information files.

### `user_downlimit`

```c
INT16 od_control.user_downlimit;
```

This variable contains the total number of kilobytes that the caller is permitted to download during this call. If your door allows files do be downloaded, you will probably want to compare the value of this variable to the size of any file to be transferred and the total kilobytes already downloaded, as stored in the [`od_control.user_todayk`](#user_todayk) variable. This variable is only available under systems that produce an EXITINFO.BBS file.

### `user_downloads`

```c
DWORD od_control.user_downloads;
```

This variable contains the total number of files that the current user has downloaded from the BBS, and is available under systems that produce EXITINFO.BBS, PC-Board/GAP style DOOR.SYS, WildCat style DOOR.SYS or SFDOORS.DAT format door information files.

### `user_echomailentered`

```c
BYTE od_control.user_echomailentered;
```

This variable is a Boolean value, indicating whether or not the user has entered new EchoMail during this call. If this variable has a value of TRUE, then EchoMail has been entered, and if it has a value of FALSE, then EchoMail has not been entered. This variable will contain a valid value only after [`od_init()`](../api/od_init.md) or some OpenDoors function has been called. Any changes made to this variable will be reflected within the BBS software when control is returned to the BBS. This variable is accessible only under systems which produce an EXITINFO.BBS door information file.

### `user_error_free`

```c
BYTE od_control.user_error_free;
```

This variable contains a Boolean value indicating whether or not the user is connected to the BBS via an error free connection (eg. a V.42/MNP or similar modem protocol). This variable is only available under systems that produce an SFDOORS.DAT, Wildcat style DOOR.SYS or RA 1.00 or later style extended EXITINFO.BBS door information file.

### `user_firstcall`

```c
char od_control.user_firstcall[9];
```

This variable is a string which contains the date of the user's first call, in the same format as the od_control. user_lastcall variable. This variable is only available under systems which produce an RA 1.00 and later style extended EXITINFO.BBS door information file.

### `user_flags`

```c
BYTE od_control.user_flags[4];
```

The [`od_control.user_flags`](#user_flags) variable is an array of four sysop defined bit-mapped flags, which represent some sort of information about the user. [`od_control.user_flags`](#user_flags)[0] stores flags A1 - A8 in bits 0 through 7, respectively. Likewise, [`od_control.user_flags`](#user_flags)[1] stores flags B1 - B8, and so on. This variable is only available under systems that produce EXITINFO.BBS format door information files.

### `user_handle`

```c
char od_control.user_handle[36];
```

This variable contains the user's alias or handle name, if any. If the user does not have an alias or handle, this variable will be blank. Door-information formats which supply it include `CHAIN.TXT`, RA 1.00 and later extended `EXITINFO.BBS`, and Wildcat-style `DOOR.SYS`.

During a Unix forced-local login, OpenDoors replaces this field with the login
name from the current user's account record. If that record or its login name
is unavailable, the existing field is retained. OpenDoors reads the handle for
status display and writes it back to supported door-information formats. An
application may supply a value before initialization or change it afterward.

### `user_homephone`

```c
char od_control.user_homephone[16];
```

This string contains the user's home or data phone number, if available. This value is only available under system that produce one of the following door information files: EXITINFO.BBS, PC-Board/GAP style DOOR.SYS, WildCat style DOOR.SYS or SFDOORS.DAT.

### `user_last_pwdchange`

```c
BYTE od_control.user_last_pwdchange;
```

This variable contains the number of calls that the user has made since they last changed their password. This variable is only available under EXITINFO.BBS files.

### `user_lastdate`

```c
char od_control.user_lastdate[9];
```

This variable is a string containing the date of the user's last call to the BBS, and should always be of the format:

```c
"MM-DD-YY"
```

Where MM is two digits representing the number of the month of the user's call, with 1 being January, 2 being February, and so on. DD should be two digits representing the day of the month of the user's last call, beginning with 1, and MM should be the last two digits of the year of the user's last call.

This variable is only available under systems that produce one of the following door information files: CHAIN.TXT, EXITINFO.BBS, PC-Board/GAP style DOOR.SYS or WildCat style DOOR.SYS files.

### `user_lastread`

```c
DWORD od_control.user_lastread;
```

This variable contains the number of the highest message number that the user has read, and is only available under EXITINFO.BBS format door information files.

### `user_lasttime`

```c
char od_control.user_lasttime[6];
```

This variable contains a string representing the time of the user's last call to the BBS, and should always be of the format:

```c
"HH:MM"
```

Where HH is two digits representing the 24-hour format hour of the user's last call, and MM is two digits representing the minute of the user's last call. Thus, the following strings would be valid entries for this string:

"00:01"    (12:01 am) "03:47"    (3:47 am) "18:20"    (6:20 pm)

This variable is only available under systems that produce an EXITINFO.BBS or Wildcat style DOOR.SYS format door information file.

### `user_location`

```c
char od_control.user_location[26];
```

This string contains the name of the location from which the current user is calling from. This will usually be the name of the city, region (province, state, etc.) and sometimes country where the user lives. The contents of this variable are displayed on the OpenDoors status line. The value of this variable is valid after [`od_init()`](../api/od_init.md) or any other OpenDoors function has been called. Also, you may change the value of this variable if you wish. However, not that these changes may not immediately be reflected in the status line, and may or may not cause the setting to be changed after the user returns to the BBS. This variable is available under systems that produce one of the following door information files: DORINFO?.DEF, EXITINFO.BBS, PC-Board/GAP style DOOR.SYS, WildCat style DOOR.SYS SFDOORS.DAT and CALLINFO.BBS, but is not available under CHAIN.TXT or DoorWay style DOOR.SYS files.

### `user_loginsec`

```c
INT32 od_control.user_loginsec;
```

This variable contains the user's security at login, and can be used to detect changes by the sysop or other programs during the course of the call, by comparing it's value with the [`od_control.user_security`](#user_security) variable. This variable is only available under systems which produce an EXITINFO.BBS file.

### `user_logintime`

```c
char od_control.user_logintime[6];
```

This variable contains the time of day at which the current call to the BBS
began, in the same 24-hour `"HH:MM"` format as
[`od_control.user_lasttime`](#user_lasttime). For `SFDOORS.DAT`, line 15 is a
decimal count of minutes: OpenDoors divides it by 60 for `HH` and uses the
remainder for `MM`. Thus, a value of 754 becomes `"12:34"`. This variable is
available under systems which produce an `EXITINFO.BBS`, a Wildcat-style
`DOOR.SYS`, or an `SFDOORS.DAT` file.

### `user_logonpassword`

```c
char od_control.user_logonpassword[16];
```

This variable is a string which contains the user's password at the time at which the current call to the BBS began. This variable can be used to detect changes by the sysop or other programs to the user's password, which have taken place during the course of the call. In order to detect such changes, simply compare the contents of this string with the contents of the [`od_control.user_password`](#user_password) variable. This variable is only available under systems which produce an EXITINFO.BBS format door information file.

### `user_menustack`

```c
char od_control.user_menustack[50][9];
```

This variable is an array of 50 strings, containing the stack of BBS menus that have been executed, and is used to record the current position of the user within the BBS's menu system. Each string contains just the base portion of the filename of the menu, without the extension. The od_control.ra_menustackpointer variable points to the top of the menu stack. However, a complete discussion of the menu stack is beyond the scope of this manual. This variable is only available under systems that produce an RA 1.00 and later style extended EXITINFO.BBS door information file.

### `user_menustackpointer`

```c
BYTE od_control.user_menustackpointer;
```

This variable points to the top of the current menu stack. For more information on the menu stack, please refer to the od_control.ra_menustack variable, above. This variable is only available under systems that produce an RA 1.00 and later style extended EXITINFO.BBS door information file.

### `user_messages`

```c
WORD od_control.user_messages;
```

This variable contains a value representing the total number of messages that have been written by the user, and is available under EXITINFO.BBS or Wildcat style DOOR.SYS format door information files.

### `user_name`

```c
char od_control.user_name[36];
```

This string contains the name of the user who is currently online. OpenDoors
uses it on the status line, and a door will commonly use it to distinguish
among callers. All supported door-information formats provide a caller name.
The field begins empty when the control structure has static storage and is
populated during initialization unless the application supplied the session
information itself.

Forced-local initialization uses the configured sysop name as its initial
caller name. On Unix, OpenDoors replaces that value with the GECOS name from
the current user's account record when one is available. If the account record
or its GECOS string is unavailable, the existing caller name is retained.

In most cases, a door should not change this value because a user's name does
not normally change and some BBS formats write the modified value back when
the door exits. For an example of reading this field, see the `EX_VOTE.C`
example program.

### `user_net_credit`

```c
DWORD od_control.user_net_credit;
```

Contains the caller's remaining NetMail credit from supported
`EXITINFO.BBS` records. OpenDoors reads the field during initialization and
writes its current value back when updating those records. The public
compatibility name `user_credit` expands to this member, as do the applicable
older caller-credit aliases.

The value is a [`DWORD`](../types.md#dword) in `tODControl`. Individual legacy records can store a
narrower credit field; for example, the primitive `EXITINFO.BBS` writer casts
the value to [`WORD`](../types.md#word). A door targeting such a format must not assume that a
value above 65,535 will survive the write-back conversion.

### `user_netmailentered`

```c
BYTE od_control.user_netmailentered;
```

This variable is a Boolean value, indicating whether or not the user has entered new NetMail or GroupMail during this call. If this variable has a value of TRUE, then NetMail/GroupMail has been entered, and if it has a value of FALSE, then NetMail/GroupMail has not been entered. This variable will contain a valid value only after [`od_init()`](../api/od_init.md) or some OpenDoors function has been called. Any changes made to this variable will be reflected within the BBS software when control is returned to the BBS. This variable is accessible only under systems which produce an EXITINFO.BBS door information file.

### `user_num`

```c
WORD od_control.user_num;
```

This variable contains the number of the user's record in the user database file, where 0 is the first record. This can be useful for changing user settings that are not re-read by the BBS, such as the user's phone number or security level which might be altered by a call back verification door. However, the value of this variable itself should not be altered.

This variable is available under systems which produce any of the following door information file formats: CHAIN.TXT, PC- Board/GAP style DOOR.SYS, Wildcat style DOOR.SYS SFDOORS.DAT and EXITINFO.BBS.

### `user_numcalls`

```c
DWORD od_control.user_numcalls;
```

This variable contains the total number of calls that the current user has placed to the BBS, and is available under systems that produce EXITINFO.BBS or PC-Board/GAP and Wildcat style DOOR.SYS door information files.

### `user_numpages`

```c
INT16 od_control.user_numpages;
```

The value of this variable contains the total number of times that the user has paged the sysop, and can be used to limit the number of times that the user is permitted to page the sysop. OpenDoors increments this variable every time that the user pages the sysop, via the [`od_page()`](../api/od_page.md) function. This variable is used with all types of door information files. However, this variable will only reflect the value within the BBS if an EXITINFO.BBS file is produced. Otherwise, the variable will only contain the number of times that the user has paged within the door, but not the total number of times the user has paged. Under EXITINFO.BBS systems, changes to the value of this variable will be reflected within the BBS upon return by the DOOR.

### `user_password`

```c
char od_control.user_password[16];
```

This variable contains the user's password for accessing the BBS. OpenDoors does not use this value itself. This variable will contain a valid value only after [`od_init()`](../api/od_init.md) or some OpenDoors function has been called. You may change the value of this variable. Note, however, that changes in this variable may or may not cause the setting to be changed when control returns to the BBS - this will depend upon the particular BBS system your door is running under. This variable is only available under systems that produce one of the following door information files: EXITINFO.BBS, PC-Board/GAP and Wildcat style DOOR.SYS, SFDOORS.DAT, and CALLINFO.BBS.

### `user_pending`

```c
DWORD od_control.user_pending;
```

This variable represents the total value of NetMail that has been written by the current user, but not yet exported from the message base. This variable is only available under systems that produce an EXITINFO.BBS door information file.

### `user_reasonforchat`

```c
char od_control.user_reasonforchat[78];
```

This variable is a string, containing the reason for which the user wishes to chat with the sysop, as they entered at the time of paging the sysop. This variable will contain an empty string if the user has not paged the sysop, or if the reason the user wishes to chat is unknown. See also the [`od_control.user_wantchat`](#user_wantchat) variable. This variable is available under all BBS systems, regardless of what style of door information file they produce. However, this variable will not be passed between the door and BBS, and thus the user's reason for chat within the door will not necessarily correspond to their reason for chat outside the door.

### `user_screen_length`

```c
WORD od_control.user_screen_length;
```

This value of this variable represents the total number of lines that can be displayed on the user's screen at once, and is usually either 24 or 25. You may wish to make use of this variable to allow your door to pause the display of long pieces of text after every screen length, in order to allow the user to read this information before it passes off of their screen. In this case, you would simply maintain a counter of the total number of lines displayed, and when this value reaches one less than the length of the user screen, display a prompt asking the user to whether or not they wish to continue.

This variable is set to the user's setting within the BBS under systems that produce any of the following door information file formats: CHAIN.TXT, EXITINFO.BBS, PC-Board/GAP and Wildcat style DOOR.SYS and CALLINFO.BBS files.

This variable is used by the OpenDoors function, [`od_list_files()`](../api/od_list_files.md). If this variable contains a valid value, OpenDoors will pause the listing of files after every screen, and give the user the option of continuing, aborting, or disabling the "Continue?" prompt for the rest of the file listing. Thus, if you are using the [`od_list_files()`](../api/od_list_files.md) under a system that does not produce one of the door information files listed above, you may wish to obtain the user's screen length from the user themselves. If the screen length is not available from the particular type of door information file that is found, and you do not set this value yourself, this variable will default to 23. A nonzero value assigned before [`od_init()`](../api/od_init.md) is preserved unless the selected door-information format supplies a replacement.

### `user_screenwidth`

```c
BYTE od_control.user_screenwidth;
```

This variable contains a value representing the width of the user's screen, and will most often be equal to 80. This variable is only available under systems that produce a CHAIN.TXT or RA 1.00 and later style extended EXITINFO.BBS door information file.

When no width is supplied, [`od_init()`](../api/od_init.md) defaults this field
to 80. A nonzero value assigned before initialization is preserved unless the
selected door-information format supplies a replacement.

### `user_security`

```c
WORD od_control.user_security;
```

This variable contains a numerical value representing the user's security access level on the BBS. You may wish to use this value to determine whether or not the current user of your door should have access to certain sysop-only functions. In this case, you may wish to have a configuration file used by your door, in which the sysop may define the minimum security level for sysop access. You would then be able to compare this configuration setting to the security level stored in this variable, in order to determine whether or not sysop function should be available. An alternative method, used by the EX_VOTE.C sample door, of determining whether or not the current user is the sysop is to compare the user's name with the value of the [`od_control.sysop_name`](#sysop_name) variable. This method has the advantage of not requiring a configuration program, but the disadvantage that the door will not function correctly under all BBS systems, as the [`od_control.sysop_name`](#sysop_name) variable is not available under all BBS systems.

The [`od_control.user_security`](#user_security) variable is available under BBS systems that produce any of the following door information file

formats: CHAIN.TXT, EXITINFO.BBS, PC-Board/GAP and Wildcat style DOOR.SYS, SFDOORS.DAT or CALLINFO.BBS.

### `user_sex`

```c
char od_control.user_sex;
```

This variable contains a single character representing the gender of the user that is currently online. This variable will contain an upper-case 'F' if the user is female, and an upper- case 'M' if the user is male. This variable is available under systems that produce a CHAIN.TXT or RA 2.x style EXITINFO.BBS file.

### `user_subdate`

```c
char od_control.user_subdate[9];
```

This variable is a string, in the same format as the [`od_control.user_lastdate`](#user_lastdate) variable, which stores the date of expiry of the user's subscription to the BBS. This variable is only available under systems which produce a PC-Board/GAP and Wildcat style DOOR.SYS or RA 1.00 and later style extended EXITINFO.BBS door information file.

### `user_timelimit`

```c
INT16 od_control.user_timelimit;
```

This variable contains the amount of time, in minutes, that the user has left in the door. Note that this value may or may not be equal to the total amount of time that the user has left on the BBS, depending upon whether the BBS or a third-party door manager program only allows a limited amount of time in this door. This variable contains a valid value after [`od_init()`](../api/od_init.md) or some OpenDoors function has been called. OpenDoors uses this variable to keep track of how much time the user has left in the door, and will automatically warn the user when nearly all of his or her time has been used up. OpenDoors will also force the user out of the door when their time in the door has expired. OpenDoors automatically subtracts one minute from this variable every minute that OpenDoors is active, unless chat mode has been activated (in which case the user's time will freeze), and also adjusts the value of this variable when the sysop uses the time adjustment function keys. Hence, you will not normally have any need to alter the value of this variable yourself. However, there may be some cases in which you wish to subtract a penalty or add a bonus to the user's time, such as in a "timebank" door or a door game that permits the user to "gamble time". Depending on which BBS system your door is running under, the value of this variable may or may not effect the user's time left upon return to the BBS. The BBS system will either reset the user's time to the value re-written to the door information file (this variable), or will always subtract the amount of time spent in the door from the user's remaining time.

When rewriting a primitive `EXITINFO.BBS` record, OpenDoors compensates for
the elapsed time in the door so that only a sysop or application adjustment is
applied to the time limit stored by the BBS. If the system clock was
unavailable at initialization or write-back, OpenDoors cannot distinguish
elapsed time from an adjustment and leaves the record's original time limit
unchanged. The same fallback applies if the system clock has moved backwards.

This variable is available under all door information file formats.

### `user_todayk`

```c
DWORD od_control.user_todayk;
```

This variable contains the total kilobytes of files that the current user has downloaded from the BBS during the current day, and is available under systems that produce EXITINFO.BBS, PC- Board/GAP and Wildcat style DOOR.SYS, or SFDOORS.DAT format door information files.

### `user_upk`

```c
DWORD od_control.user_upk;
```

This variable contains the total kilobytes of files that the current user has uploaded to the BBS, and is available under systems that produce EXITINFO.BBS, Wildcat style DOOR.SYS or SFDOORS.DAT files.

### `user_uploads`

```c
DWORD od_control.user_uploads;
```

This variable contains the total number of files that the current user has uploaded to the BBS, and is available under systems that produce EXITINFO.BBS, PC-Board/GAP and Wildcat style DOOR.SYS, or SFDOORS.DAT format door information files.

### `user_wantchat`

```c
BYTE od_control.user_wantchat;
```

This variable is a Boolean value which indicates whether or not the user wishes to chat with the sysop (ie, the user has paged the sysop, but has yet to receive a chat with the sysop). This variable is used under all door information file formats. However, changes to this variable are only reflected on the BBS when the door is running under a system that produces an EXITINFO.BBS door information file.

This variable is automatically turned on (ie., set to TRUE), when the user begins to page the sysop for chat, within the [`od_page()`](../api/od_page.md) function, and is automatically turned off (ie., set to FALSE), when the sysop breaks in for chat via the chat function key. Also, setting this variable to TRUE will turn on the flashing want-chat indicator on the OpenDoors status line.

### `user_xi_record`

```c
DWORD od_control.user_xi_record;
```

This variable contains the number of the user's record in the USERXI.BBS file, if any. This variable is only available under system that produce a Remote Access 1.00 and later style extended door information file.

## Traffic-log fields

### `timelog_busyperhour`

```c
INT16 od_control.timelog_busyperhour[24];
```

Each element contains the count recorded by `EXITINFO.BBS` for the corresponding
hour of the day: element 0 covers 00:00 through 00:59, element 1 covers 01:00
through 01:59, and so forth through element 23. The array is zero-initialized
and is populated only when OpenDoors reads an `EXITINFO.BBS` record containing
the traffic log. OpenDoors does not otherwise use these counts. All 24 values
are copied back when a standard, RemoteAccess 1.x, QuickBBS 2.75, or
RemoteAccess 2.x `EXITINFO.BBS` record is rewritten.

### `timelog_busyperday`

```c
INT16 od_control.timelog_busyperday[7];
```

These are the `EXITINFO.BBS` traffic counts for Sunday through Saturday, in
that order. The array is zero-initialized and unavailable values therefore
cannot be distinguished from valid zero counts. OpenDoors does not interpret
the counts. All seven values are copied back with the hourly values when a
supported `EXITINFO.BBS` record is rewritten. Historic RemoteAccess versions
were known not to maintain this array consistently, so the presence of an
`EXITINFO.BBS` record does not imply that nonzero data will be present.

## Extended RemoteAccess identity

### `user_org`

```c
char od_control.user_org[51];
```

This is the caller's organization from a RemoteAccess 2.x extended
`EXITINFO.BBS` record, converted from a Pascal string of at most 50 bytes to a
null-terminated C string. It is empty for formats which do not provide the
field. OpenDoors does not display or interpret it, but writes an application
replacement back to a RemoteAccess 2.x record.

### `user_address`

```c
char od_control.user_address[3][51];
```

These are the three postal-address lines in a RemoteAccess 2.x extended
`EXITINFO.BBS` record. Each source Pascal string can contain at most 50 bytes;
OpenDoors exposes it as a null-terminated C string. All elements are empty when
the active format does not supply them. OpenDoors does not otherwise read the
address and writes all three elements back only to the RemoteAccess 2.x
record.

### `user_pwd_crc`

```c
INT32 od_control.user_pwd_crc;
```

This is the signed 32-bit `password_crc` value stored in a RemoteAccess 2.x
extended `EXITINFO.BBS` user record. OpenDoors copies the value without
calculating, validating, or using it. Its initial zero can be either an
unavailable value or a value supplied by the BBS; only
[`od_info_type`](connection.md#od_info_type) establishes whether the RA2 field
was read. Application changes are written back to that format.

### `user_logon_pwd_crc`

```c
INT32 od_control.user_logon_pwd_crc;
```

This is the RemoteAccess 2.x `logonpasswordcrc` value captured at the start of
the current BBS session. It permits an application which understands that BBS
field to compare the login value with [`user_pwd_crc`](#user_pwd_crc).
OpenDoors does not calculate or compare either checksum. The field begins at
zero, is populated only by a full RemoteAccess 2.x record, and is written back
to the same format.

### `user_last_cost_menu`

```c
char od_control.user_last_cost_menu[9];
```

This is the base menu name stored in the RemoteAccess 2.x
`last_cost_menu` field, exposed as a C string of at most eight bytes plus the
terminator. It is empty when unavailable. OpenDoors does not use the name and
writes an application replacement back only to the RemoteAccess 2.x record.

### `user_menu_cost`

```c
WORD od_control.user_menu_cost;
```

This is the unsigned 16-bit RemoteAccess 2.x `menu_cost_per_min` value
associated with [`user_last_cost_menu`](#user_last_cost_menu). OpenDoors
preserves the numeric value but does not apply it to time or credit accounting.
It is zero-initialized and written back only to the full RemoteAccess 2.x
record.

## Areas, groups, and preferences

### `user_group`

```c
WORD od_control.user_group;
```

This is the caller's RemoteAccess group number. It is populated by
`EXITINFO.BBS` formats which contain the group field and otherwise remains
zero. The RemoteAccess personality displays the value on one local status
screen; the remainder of OpenDoors treats it as application data. On write-back
to the older record layout it is narrowed to one byte, while the RemoteAccess
2.x layout retains the full [`WORD`](../types.md#word).

### `user_msg_area`

```c
WORD od_control.user_msg_area;
```

This is the number of the caller's current message area. OpenDoors can obtain
it from SFDOORS.DAT and from the extended RemoteAccess 1.x and 2.x
`EXITINFO.BBS` layouts. It is zero when unavailable. OpenDoors does not use it
outside door-information input and output; the RemoteAccess 1.x extended
layout stores only one byte and therefore narrows values above 255.

### `user_file_area`

```c
WORD od_control.user_file_area;
```

This is the number of the caller's current file area. Its sources and
write-back behavior are the same as [`user_msg_area`](#user_msg_area):
SFDOORS.DAT and extended RemoteAccess `EXITINFO.BBS`, with a one-byte field in
the RemoteAccess 1.x extension. OpenDoors otherwise leaves the value for the
application.

### `user_file_group`

```c
WORD od_control.user_file_group;
```

This is the current file-group number from a full RemoteAccess 2.x extended
`EXITINFO.BBS` record. It is copied as an unsigned 16-bit value, is not used by
OpenDoors, and is written back to that record. Zero means either group zero or
that the active format did not provide the field.

### `user_msg_group`

```c
WORD od_control.user_msg_group;
```

This is the current message-group number from a full RemoteAccess 2.x extended
`EXITINFO.BBS` record. OpenDoors preserves it as an unsigned 16-bit value but
does not otherwise read it. It is written back to that format; zero is
ambiguous when another format is active.

### `user_protocol`

```c
char od_control.user_protocol;
```

This is the single-byte `default_protocol` selector from a RemoteAccess 2.x
extended `EXITINFO.BBS` record. It is the BBS's protocol-selection character,
not an OpenDoors enumeration. OpenDoors neither selects a transfer protocol
from it nor changes it automatically; application changes are written back to
the same format.

### `user_last_birthday_check`

```c
BYTE od_control.user_last_birthday_check;
```

This is the raw one-byte `last_dob_check` value from RemoteAccess 2.x. The
current OpenDoors implementation defines no date conversion or constants for
the field and does not inspect it. It remains zero for other formats and is
preserved when the RA2 record is rewritten.

### `user_language`

```c
BYTE od_control.user_language;
```

This is the RemoteAccess language-table index from an extended 1.x or 2.x
`EXITINFO.BBS` record. OpenDoors does not use the index to translate its own
prompts; an application must select any corresponding strings itself. The
field begins at zero and is written back to either extended layout.

### `user_date_format`

```c
BYTE od_control.user_date_format;
```

This is the RemoteAccess `dateformat` selector from an extended 1.x or 2.x
`EXITINFO.BBS` record. OpenDoors copies the one-byte value without interpreting
or normalizing date strings and supplies no manifest constants for its values.
It begins at zero and is written back to either extended layout.

### `user_forward_to`

```c
char od_control.user_forward_to[36];
```

This is the RemoteAccess message-forwarding destination, converted from a
Pascal string of at most 35 bytes. Extended RemoteAccess 1.x and 2.x records
provide and accept it. OpenDoors does not perform forwarding or validate the
name; the application receives an empty string when the field is unavailable.

### `user_expert`

```c
BOOL od_control.user_expert;
```

TRIBBS.SYS line 5 supplies [`TRUE`](../constants/general.md#true) when its first character is `Y` and [`FALSE`](../constants/general.md#false)
otherwise. OpenDoors writes it back as `Y` or `N` when rewriting that format,
but does not alter menus, prompts, or other behavior in response to the value.
It remains the static value [`FALSE`](../constants/general.md#false) for formats which do not supply it unless
the application assigns it.

## IEMSI and held-message data

### `user_emsi_session`

```c
BYTE od_control.user_emsi_session;
```

This is the one-byte IEMSI-session flag supplied by extended RemoteAccess 1.x
and 2.x `EXITINFO.BBS` records. A nonzero value reports that the BBS had an
interactive EMSI session; OpenDoors does not establish or negotiate the
session itself. The value is zero when unavailable and is written back with
the other extended fields.

### `user_emsi_crtdef`

```c
char od_control.user_emsi_crtdef[41];
```

This is the IEMSI terminal-definition string from extended RemoteAccess
`EXITINFO.BBS`, converted from a Pascal field of at most 40 bytes. OpenDoors
does not parse the IEMSI token and writes the C string back to either supported
extended layout.

### `user_emsi_protocols`

```c
char od_control.user_emsi_protocols[41];
```

This is the IEMSI protocols string from the same extended records. It is a
null-terminated copy of the BBS's at-most-40-byte Pascal field. OpenDoors does
not use it for protocol selection.

### `user_emsi_capabilities`

```c
char od_control.user_emsi_capabilities[41];
```

This is the IEMSI capabilities string from the same extended records,
preserved as at most 40 bytes plus a null terminator. Capability interpretation
is left to the application; OpenDoors only reads and writes the field.

### `user_emsi_requests`

```c
char od_control.user_emsi_requests[41];
```

This is the IEMSI requests string from the same extended records, converted to
a null-terminated C string. OpenDoors does not act on the requests.

### `user_emsi_software`

```c
char od_control.user_emsi_software[41];
```

This is the IEMSI client-software identification string from the same extended
records. It can contain at most 40 bytes before the terminator. OpenDoors does
not parse it.

### `user_hold_attr1`

```c
BYTE od_control.user_hold_attr1;
```

This is the first raw held-message attribute byte from extended RemoteAccess
1.x and 2.x `EXITINFO.BBS`. OpenDoors defines no bit masks for it and does not
inspect it; the value is preserved on write-back.

### `user_hold_attr2`

```c
BYTE od_control.user_hold_attr2;
```

This is the second raw held-message attribute byte. Its availability and
handling are identical to [`user_hold_attr1`](#user_hold_attr1).

### `user_hold_len`

```c
BYTE od_control.user_hold_len;
```

This is the raw held-message length byte adjacent to the two held-message
attributes in extended RemoteAccess `EXITINFO.BBS`. OpenDoors copies it but
does not use it to address or allocate any buffer.

## Session and terminal extensions

### `user_logindate`

```c
char od_control.user_logindate[9];
```

This is the date on which the current BBS session began, from
`EXITINFO.BBS`, stored as an eight-character string plus its terminator. The
representation is supplied by the BBS and is normally the same as
[`user_lastdate`](#user_lastdate). OpenDoors does not parse it and writes a
replacement back when the active `EXITINFO.BBS` layout supports the field.

### `user_timeofcreation`

```c
char od_control.user_timeofcreation[6];
```

This is the creation time stored in an `EXITINFO.BBS` record, exposed as five
characters plus a terminator. It normally has the `HH:MM` form used by
[`user_lasttime`](#user_lasttime). OpenDoors does not parse the string and
writes it back with supported `EXITINFO.BBS` variants.

### `user_readthru`

```c
INT16 od_control.user_readthru;
```

This is the signed 16-bit `readthru` value from `EXITINFO.BBS`. It is copied
without interpretation and written back with that record. The field is
initially zero, and OpenDoors itself does not read it outside the format
conversion path.

### `user_time_used`

```c
WORD od_control.user_time_used;
```

This field begins with the `elapsed` minutes from `EXITINFO.BBS`; other formats
leave it zero. During [`od_exit()`](../api/od_exit.md), OpenDoors adds its
calculated change in used time before rewriting the door-information record.
If either session timestamp is unavailable, or the system clock has moved
backwards, OpenDoors cannot calculate that change and retains the existing
value.
The terminal-emulation control-code path can also display the current numeric
value. Applications may read or adjust it, but a zero value is ambiguous when
no `EXITINFO.BBS` elapsed field was available.

### `user_rip`

```c
BYTE od_control.user_rip;
```

This Boolean reports whether RIP graphics are active for the caller. Several
text drop-file formats, custom configuration definitions, QuickBBS and
RemoteAccess `EXITINFO.BBS` variants can supply it; otherwise it begins as
[`FALSE`](../constants/general.md#false). [`od_autodetect()`](../api/od_autodetect.md) can set it to [`TRUE`](../constants/general.md#true) after
a successful RIP probe.

OpenDoors reads this field when selecting RIP-specific clear-screen behavior,
terminal-emulation commands, status-line indicators, and `.RIP` file handling.
Application changes take effect immediately and are written back only by
door-information formats which have a RIP field.

### `user_rip_ver`

```c
BYTE od_control.user_rip_ver;
```

This is the raw RIP version byte supplied by a full RemoteAccess 2.x extended
`EXITINFO.BBS` record. OpenDoors preserves it on write-back but does not use it
to select or reject RIP commands; [`user_rip`](#user_rip) is the active-mode
switch. The field remains zero when the version is unavailable.
