# `od_log_open()`

Opens the OpenDoors activity log

## Synopsis

```c
BOOL od_log_open(void);
```

## Return value

[`TRUE`](../constants/general.md#true) if logging is disabled or the log was
opened and its initial records were written successfully;
[`FALSE`](../constants/general.md#false) if the file could not be opened,
the current local time could not be obtained, or the initial records could not
be written or flushed.

## Description

The [`od_log_open()`](od_log_open.md) function begins OpenDoors log-file processing. It opens the
file named by
[`od_control.od_logfile_name`](../control/customization.md#od_logfile_name) in
append mode, writes a dated separator and an entry identifying the caller who
is entering the door, and enables the standard OpenDoors event messages. Later
calls to [`od_log_write()`](od_log_write.md), and events such as paging and
chat, then write to the same file.

Applications which enable the log-file component through
[`od_control.od_logfile`](../control/customization.md#od_logfile) do not
normally need to call this function. OpenDoors calls it while initializing that
component. It is public so that an application may begin logging explicitly
when the component was not enabled before [`od_init()`](od_init.md).

If [`od_control.od_logfile_disable`](../control/customization.md#od_logfile_disable)
is [`TRUE`](../constants/general.md#true), the function performs no file operation and returns [`TRUE`](../constants/general.md#true). If the
field is [`FALSE`](../constants/general.md#false), the configured file is opened with the C append mode `"a"`.
Failure to obtain or convert the current time, open the file, or commit the
separator and startup entry closes the new stream and returns
[`FALSE`](../constants/general.md#false). Standard
file-I/O error state, including `errno` where supplied by the C runtime, remains
available to the caller. The function does not assign an
[`ERR_*`](../constants/errors.md) value to
[`od_control.od_error`](../control/runtime.md#od_error) for this failure.
If expansion of the configurable startup template would exceed the internal
1,025-byte work buffer, the function instead sets
[`ERR_LIMIT`](../constants/errors.md#err_limit), closes the new stream, and
returns [`FALSE`](../constants/general.md#false).

The separator uses the current local date, the configurable day and month
names, and [`od_control.od_prog_name`](../control/customization.md#od_prog_name).
The first message is formatted from the entry template in
[`od_control.od_logfile_messages`](../control/customization.md#od_logfile_messages)
and [`od_control.user_name`](../control/caller.md#user_name). See the
[log-file system](../../guides/logging.md) for the complete format and the
events recorded automatically.

If a logging session is already open, another call returns
[`TRUE`](../constants/general.md#true) without opening another stream or writing
another separator and startup entry. The active stream remains associated with
the filename used by the first successful call; changing
[`od_control.od_logfile_name`](../control/customization.md#od_logfile_name) does
not switch an open logging session.

## See also

[`od_log_write()`](od_log_write.md), [`od_init()`](od_init.md),
[Log-file system](../../guides/logging.md)
