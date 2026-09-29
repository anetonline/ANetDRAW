# `od_spawn()`

To facilitate easy execution of child tasks from doors.

## Synopsis

```c
BOOL od_spawn(const char *pszCommandLine);
```

## Return value

[`TRUE`](../constants/general.md#true) when the platform launcher reports that the command was started;
[`FALSE`](../constants/general.md#false) when it reports a launch failure. The
child's ordinary nonzero exit status does not make the call fail.

## Description

This function allows you to easily run other programs from within your door programs, such as external file transfer utilities, compression utilities, and so on.

In a 16-bit DOS build, this function can swap OpenDoors and the door program to
expanded memory or disk before executing the child. Swapping is controlled by
[`od_control.od_swapping_disable`](../control/customization.md#od_swapping_disable),
[`od_control.od_swapping_noems`](../control/customization.md#od_swapping_noems),
and [`od_control.od_swapping_path`](../control/customization.md#od_swapping_path).
Unless EMS use has been disabled, OpenDoors first attempts to use EMS 3.2 or
later memory. If sufficient EMS is not available, it uses a disk swap file in
the configured directory. DOS32, Windows and Unix-like builds do not use the
16-bit swapping implementation.

On DOS and DOS32, OpenDoors saves the 80-by-25 local screen and the current
directory before executing a waited-for child, and restores them when the child
returns. The communications connection is closed around the child and then
reopened, pending input is cleared, and the local cursor, attribute and output
window are restored. On Windows, OpenDoors displays a local
"Running sub-program..." message while waiting. Unix-like builds invoke the
system shell directly and do not perform the DOS screen and directory
operations.

On the DOS, DOS32 and Windows paths, the user's remaining time normally
continues to decrease while the child is running. Set
[`od_control.od_spawn_freeze_time`](../control/customization.md#od_spawn_freeze_time)
to freeze the caller's time during that period. The Unix [`od_spawn()`](od_spawn.md) path
calls `system()` directly and does not apply this accounting option.

The manner in which `pszCommandLine` is interpreted is platform dependent:

- On DOS and DOS32, OpenDoors passes the command line to the command processor
  named by `COMSPEC` using its `/c` option. If that program cannot be found, it
  retries with `command.com`.
- On Windows, OpenDoors treats the text before the first space as the program
  name. All remaining text is retained as a preformatted command-line tail for
  the Windows C runtime. This preserves [`od_spawn()`](od_spawn.md) as the
  interface for a child which expects a custom Windows command-line grammar;
  use [`od_spawnvpe()`](od_spawnvpe.md) when starting a conventional child from
  an array of raw argument values. The simple program-name split does not
  recognize a quoted program path containing spaces.
- On Unix-like targets, the complete string is passed to `system()`. OpenDoors
  temporarily blocks `SIGALRM`, the signal used by its kernel timer, while
  `system()` is running and then restores the caller's complete signal mask.

DOS and Windows return [`FALSE`](../constants/general.md#false) only when the
spawn runtime returns `-1`. On Unix-like targets, OpenDoors decodes the wait
status returned by `system()` and returns [`FALSE`](../constants/general.md#false)
when `system()` itself fails or the command shell exits with status 127. Status
127 is the result specified by `system()` when its command shell could not be
executed; a command which deliberately returns the same status cannot be
distinguished from that condition. Other child exit statuses report
[`TRUE`](../constants/general.md#true), consistently with the DOS and Windows
launch-success result. The function does not assign an
[`ERR_*`](../constants/errors.md) value of its own.

On Unix-like targets, failure to block `SIGALRM` prevents the command from
being launched and returns [`FALSE`](../constants/general.md#false). Failure to
restore the caller's signal mask after the command returns also produces a
[`FALSE`](../constants/general.md#false) result.

## Examples

Below are a few examples of various uses of the [`od_spawn()`](od_spawn.md) function:

To run the command processor from within your door program, to allow the sysop access to the DOS shell, simply use the following line of code:

```c
od_spawn(getenv("COMSPEC"));
```

The following function is an example of using the [`od_spawn()`](od_spawn.md) function to call DSZ, allowing the user to download a file. You pass the name of the file that you wish to send to the user. This function will then ask the user what transfer protocol they would like to use, generate the appropriate DSZ command line, and then transmit the file to the user. Note that in order to use a door which implements this function, the external file transfer program "DSZ" must be available in the current search path. As an alternative, you may want to allow the sysop to specify the location of the DSZ file from within a configuration program. If you wish to receive a file (allow the user to upload), instead of sending one, simply change the "s" in the command line to a "r".

```c
BOOL download(const char *filename)
{
   char commandline[80];
   char protocol;

   od_printf("Select File Transfer Protocol:\n\r");
   od_printf("   [X] XModem\n\r");
   od_printf("   [Y] YModem\n\r");
   od_printf("   [Z] ZModem\n\r");
   od_printf("or press [A] to abort transfer\n\r");

   do
   {
      protocol = od_get_key(TRUE);
      if(protocol == 'a' || protocol == 'A')
         return(FALSE);
   } while(protocol != 'x' && protocol != 'y' && protocol != 'z'
      && protocol != 'X' && protocol != 'Y' && protocol != 'Z');

   od_printf("Begin receiving file now or press [CTRL]-[X] to abort\n\r");
   sprintf(commandline, "dsz port %d s%c %s",
      od_control.port + 1, protocol, filename);

   return(od_spawn(commandline));
}
```

This example reflects the command-line interface of the external DSZ utility.
Applications using a different transfer program must construct the command
expected by that program and must ensure that the destination buffer is large
enough for the resulting path and arguments.

## See also

[`od_spawnvpe()`](od_spawnvpe.md), [`od_save_screen()`](od_save_screen.md)
