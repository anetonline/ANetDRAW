# `od_carrier()`

Tests the current remote connection

## Synopsis

```c
BOOL od_carrier(void);
```

## Return value

Returns [`TRUE`](../constants/general.md#true) while the active remote
communications method reports that the caller is still connected. Returns
[`FALSE`](../constants/general.md#false) when the connection has been lost or
OpenDoors is operating in local mode.

## Description

OpenDoors normally monitors the connection itself. When a remote caller is
lost, the kernel performs its configured hangup handling without requiring
the door to poll [`od_carrier()`](od_carrier.md). An application normally needs this function
only after setting
[`DIS_CARRIERDETECT`](../constants/session.md#dis_carrierdetect) in
[`od_control.od_disable`](../control/customization.md#od_disable), thereby
taking responsibility for connection-loss handling.

For a serial or FOSSIL connection, the result represents the carrier-detect
signal. For a socket it represents whether the peer still appears connected;
for Door32 it reflects the offline event; and for standard input/output it
reflects the implementation's hangup indication. Consequently, “carrier” in
this interface means the current communications method's connection state,
not necessarily a physical modem signal.

Local mode has no remote connection. In that mode the function returns
[`FALSE`](../constants/general.md#false) and places [`ERR_NOREMOTE`](../constants/errors.md#err_noremote) in
[`od_control.od_error`](../control/runtime.md#od_error).

If the communications method cannot determine the connection state, the
function returns [`FALSE`](../constants/general.md#false) and places
[`ERR_GENERALFAILURE`](../constants/errors.md#err_generalfailure) in
[`od_control.od_error`](../control/runtime.md#od_error). Because an ordinary
loss of carrier also returns [`FALSE`](../constants/general.md#false), an
application which must distinguish the two cases should set
[`od_control.od_error`](../control/runtime.md#od_error) to
[`ERR_NONE`](../constants/errors.md#err_none) before calling
[`od_carrier()`](od_carrier.md), then inspect it if the function returns
[`FALSE`](../constants/general.md#false).

## Example

A callback door can lower DTR and wait a limited time for carrier to disappear.
Automatic carrier detection must be disabled before this sequence:

```c
BOOL hangup_and_wait(void)
{
   unsigned int tenths;
   BOOL disconnected = FALSE;

   od_control.od_error = ERR_NONE;
   od_set_dtr(FALSE);
   if(od_control.od_error != ERR_NONE)
      return FALSE;

   for(tenths = 0; tenths < 300; ++tenths)
   {
      od_control.od_error = ERR_NONE;
      if(!od_carrier())
      {
         if(od_control.od_error != ERR_NONE)
            break;
         disconnected = TRUE;
         break;
      }
      od_sleep(100);
   }
   od_set_dtr(TRUE);
   return disconnected;
}
```

This example waits for approximately 30 seconds without keeping the processor
in a tight polling loop.

## See also

[`od_set_dtr()`](od_set_dtr.md), [`od_kernel()`](od_kernel.md),
[`od_sleep()`](od_sleep.md)
