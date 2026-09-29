# `od_set_dtr()`

Raises or lowers the remote connection's DTR state

## Synopsis

```c
void od_set_dtr(BOOL bHigh);
```

## Return value

N/A

## Description

Passing [`FALSE`](../constants/general.md#false) lowers Data Terminal Ready;
passing [`TRUE`](../constants/general.md#true) raises it. OpenDoors normally
keeps DTR raised. Most suitably configured modems disconnect when DTR is
lowered, so a callback-verification door can use `od_set_dtr(FALSE)` to end
the current call without terminating the door. After carrier disappears, the
door should normally raise DTR again before attempting another call.

Not every modem is configured to disconnect on loss of DTR. An application
which must support such equipment may also need an appropriate modem command
sequence.

OpenDoors normally exits or begins its hangup processing when carrier is lost.
A door which intends to continue after lowering DTR must first set
[`DIS_CARRIERDETECT`](../constants/session.md#dis_carrierdetect) in
[`od_control.od_disable`](../control/customization.md#od_disable), then use
[`od_carrier()`](od_carrier.md) to determine when the connection has ended.

For a FOSSIL, direct UART, or native Windows serial connection, this function
changes the actual DTR state. A socket has no DTR signal: lowering it closes
the socket, while raising it is unsupported. Door32 and standard-input/output
connections do not implement the operation.

In local mode, no remote object exists. The function makes no change and sets
[`ERR_NOREMOTE`](../constants/errors.md#err_noremote).

If the active communications method does not support the requested operation,
or if its driver reports a failure, the function sets
[`ERR_GENERALFAILURE`](../constants/errors.md#err_generalfailure) in
[`od_control.od_error`](../control/runtime.md#od_error). Because the function
has no return value, applications which need to detect failure should set
[`od_control.od_error`](../control/runtime.md#od_error) to
[`ERR_NONE`](../constants/errors.md#err_none) before calling it and inspect the
field afterward.

## Example

See the timed disconnect example under
[`od_carrier()`](od_carrier.md#example).

## See also

[`od_carrier()`](od_carrier.md), [`od_exit()`](od_exit.md)
