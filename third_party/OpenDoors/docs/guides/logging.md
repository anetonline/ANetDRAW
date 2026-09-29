# Log-file system

OpenDoors can maintain a FrontDoor-format activity log and allows applications
to add their own entries.

## Detailed reference

In order for the system operator to monitor system activity and diagnose
problems which occur while the system is unattended, BBS software and door
programs commonly record major events in a log file. The log may record events
such as a caller entering or leaving a door, paging the system operator, or
entering chat. A system operator may configure all software on one node to use
one log, or may provide a separate log for each program. Software serving
different nodes must not write concurrently to one file unless external
locking is provided; OpenDoors does not lock the activity log between nodes.

OpenDoors writes the traditional FrontDoor text-log format. Each logging
session begins with a separator containing the local date and program name.
Individual entries begin with the local time. A segment from a log produced by
OpenDoors is shown below:

```text
----------  Sat 08 Aug 26, Sample Door
> 14:42:23  Example User entering door
> 14:50:55  User paging system operator
> 14:51:02  Entering sysop chat mode
> 15:05:41  Terminating sysop chat mode
> 15:18:32  User's time limit expired, exiting door
```

To enable the OpenDoors log file system, simply include the following line before your first call to any OpenDoors function:

```c
od_control.od_logfile = INCLUDE_LOGFILE;
```

When OpenDoors is initialized, the log component calls
[`od_log_open()`](../reference/api/od_log_open.md), unless logging has been
disabled with
[`od_control.od_logfile_disable`](../reference/control/customization.md#od_logfile_disable).
The file name is taken from
[`od_control.od_logfile_name`](../reference/control/customization.md#od_logfile_name),
which may be set by the configuration file. If no name has been supplied,
OpenDoors uses `DOOR.LOG`. Opening the log appends a separator and an entry
indicating that the caller has entered the door.

The [`od_control.od_prog_name`](../reference/control/customization.md#od_prog_name) variable sets the program name that is written to the log file immediately after the current date information. If this variable is not set, OpenDoors will write its own name and version information in this place.

When the OpenDoors log-file system is enabled, it automatically records:

- the caller entering the door;
- the caller paging the system operator, including the stated reason;
- the beginning and end of sysop chat;
- entry to and return from an operating-system shell;
- carrier loss, inactivity timeout, or expiration of the caller's time;
- the sysop returning the caller to the BBS, hanging up, or locking out the
  caller; and
- termination through [`od_exit()`](../reference/api/od_exit.md), including
  the error level where applicable.

Applications may add entries with
[`od_log_write()`](../reference/api/od_log_write.md). The strings used for
automatic entries are configurable through
[`od_control.od_logfile_messages`](../reference/control/customization.md#od_logfile_messages),
and the day and month names are taken from
[`od_control.od_day`](../reference/control/customization.md#od_day) and
[`od_control.od_month`](../reference/control/customization.md#od_month).
Each complete separator or log entry is flushed before the writing function
reports success, so a reported file-I/O failure is not deferred until orderly
shutdown.

During orderly shutdown, OpenDoors attempts the final entry and then closes
the stream even if that entry fails. A formatting limit retains
[`ERR_LIMIT`](../reference/constants/errors.md#err_limit) in
[`od_control.od_error`](../reference/control/runtime.md#od_error); a final
write, flush, or close failure records
[`ERR_GENERALFAILURE`](../reference/constants/errors.md#err_generalfailure).
The stream and its internal hooks are released in every case.
