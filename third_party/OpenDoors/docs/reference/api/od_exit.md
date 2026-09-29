# `od_exit()`

The OpenDoors program termination function

## Synopsis

```c
void od_exit(INT nErrorLevel, BOOL bTermCall);
```

## Return value

N/A

## Description

Use this function whenever an initialized door is to terminate. Calling the C
`exit()` function directly bypasses OpenDoors cleanup. [`od_exit()`](od_exit.md) updates time
accounting, invokes the application's before-exit callback, rewrites supported
door-information files, closes the activity log, optionally disconnects the
caller, shuts down the kernel and communications objects, restores the local
display, and terminates the process with `nErrorLevel`.

Failure of the activity log's final entry, flush, or close does not prevent the
remaining shutdown work and does not replace `nErrorLevel`. OpenDoors records
the logging diagnosis in
[`od_control.od_error`](../control/runtime.md#od_error) before continuing.
The same rule applies when a supported door-information file cannot be opened,
written, flushed, or closed. Failure to open the file records
[`ERR_FILEOPEN`](../constants/errors.md#err_fileopen); an output or close
failure records
[`ERR_GENERALFAILURE`](../constants/errors.md#err_generalfailure). Because the
text formats are opened in replacement mode and `EXITINFO.BBS` is updated in
place, a failed rewrite may leave a truncated or partially updated file.

If `bTermCall` is [`TRUE`](../constants/general.md#true), OpenDoors treats the operation as termination of the
call. For a remote session it waits up to ten seconds for pending output,
lowers DTR or performs the equivalent disconnect operation supplied by the
active communications method, waits up to five seconds for carrier to
disappear, and then raises DTR again where that operation is supported. The BBS
can then perform its normal logoff processing. A value of [`FALSE`](../constants/general.md#false) leaves the
remote connection available for the BBS which launched the door.

If your program must always perform work before exiting, such as updating or
closing data files, install an
[`od_control.od_before_exit`](../control/customization.md#od_before_exit)
callback. OpenDoors invokes it even when the library initiates termination,
such as after the caller hangs up. The callback runs after OpenDoors has
calculated the caller's remaining and used time and restored the original
drop-file baud value, but before any door-information file is rewritten or
communications resource is closed. A recursive call to [`od_exit()`](od_exit.md) from that
callback is ignored.

Only formats for which OpenDoors has a rewrite implementation are updated. For
the extended RemoteAccess and QuickBBS `EXITINFO.BBS` variants, the fields
available in that record are copied back. Several supported text formats are
rewritten from the values retained during initialization and the current
values in [`od_control`](../control/index.md). Fields which are written for
each format are identified individually in the control-structure reference.
When the system clock is unavailable or has moved backwards, OpenDoors retains
the existing used-time value. A primitive `EXITINFO.BBS` rewrite also retains
the time limit read from the BBS rather than deriving an adjustment from an
invalid elapsed interval; the remaining fields are still rewritten.

Setting [`DIS_INFOFILE`](../constants/session.md#dis_infofile) in
[`od_control.od_disable`](../control/customization.md#od_disable) before
initialization prevents both reading and later rewriting a door-information
file. Setting
[`od_control.od_noexit`](../control/customization.md#od_noexit) causes
[`od_exit()`](od_exit.md) to perform the complete OpenDoors shutdown and then return instead
of terminating the process. The application may continue doing work which does
not use OpenDoors. The library is permanently terminal: it cannot initialize a
second session, and a later OpenDoors function call is rejected with its normal
neutral or failure value and
[`ERR_GENERALFAILURE`](../constants/errors.md#err_generalfailure). The exported
[`od_control`](../control/index.md) object remains available for direct reads
by the host after shutdown.

The before-exit callback runs before OpenDoors decides whether
[`od_noexit`](../control/customization.md#od_noexit) applies, so it may set or
clear that field based on application state. If a callback or nested kernel
operation requests a returning shutdown, OpenDoors records the first request,
causes cooperative waits to stop, and defers resource destruction until the
outermost public API call unwinds. A request made during initialization is
likewise retained; initialization completes before the one shutdown sequence
begins.

On non-Windows text-mode targets, the local output window is reset to the full
80-by-25 display and the attribute is reset to grey on black. If
[`od_control.od_clear_on_exit`](../control/customization.md#od_clear_on_exit) is
[`TRUE`](../constants/general.md#true), the local display is cleared; otherwise the cursor is moved to the
upper-left corner. Platform display resources are then closed in either case.

If [`od_exit()`](od_exit.md) is called before any other OpenDoors function, it first calls
[`od_init()`](od_init.md) so that the resources and session information needed
for an orderly shutdown exist. The function normally does not return. It can
return when [`od_noexit`](../control/customization.md#od_noexit) is set, when a recursive exit is suppressed, when shutdown
is deferred to an outer API boundary, or when it is running as part of an
already active process-exit handler.

## Examples

The example below demonstrates a function which a door could execute when the user chooses to exit the door. This function will ask the user whether they wish to exit the door and return to the BBS, simply logoff of the BBS, or continue using the door. The example function will then call [`od_exit()`](od_exit.md) if the user wishes to exit the door, or return control to the function which called it, if the user does not wish to exit:

```c
void goodbye(void)
{
   char pressed;

   od_disp_str("You have chosen to exit this door.\n\r");
   od_disp_str("Do you wish to:\n\r");
   od_disp_str("      [R]eturn to the BBS\n\r");
   od_disp_str("      [L]og off the BBS\n\r");
   od_disp_str("      [C]ontinue using the door\n\r");

   for(;;)
   {
      pressed = od_get_key(TRUE);

      if(pressed == 'R' || pressed == 'r')
         od_exit(40, FALSE);

      if(pressed == 'L' || pressed == 'l')
         od_exit(41, TRUE);

      if(pressed == 'C' || pressed == 'c')
         return;
   }
}
```

## See also

[`od_init()`](od_init.md), [`od_set_dtr()`](od_set_dtr.md),
[Session lifecycle](../../guides/session-lifecycle.md)
