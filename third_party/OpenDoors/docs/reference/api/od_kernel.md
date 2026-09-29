# `od_kernel()`

Performs OpenDoors' periodic session processing.

## Synopsis

```c
void od_kernel(void);
```

## Description

OpenDoors functions call [`od_kernel()`](od_kernel.md) regularly in builds
which require explicit servicing. An application normally calls it directly
only during a computation, polling loop, or wait which can run for more than
about one second without making any other OpenDoors call.

Every invocation first calls
[`od_control.od_ker_exec`](../control/customization.md#od_ker_exec), when that
callback is installed. A recursive call made while the kernel is already
active returns immediately, preventing callbacks and functions invoked by the
kernel from re-entering the processing loop.

In a single-threaded build, periodic processing includes:

- reading available remote bytes into the common input queue;
- checking carrier detect unless
  [`DIS_CARRIERDETECT`](../constants/session.md#dis_carrierdetect) is set;
- processing enabled DOS local-keyboard input, sysop command keys, custom hot
  keys, chat, and DOS shell requests;
- updating or redrawing the DOS status line as required;
- deducting elapsed minutes from
  [`od_control.user_timelimit`](../control/caller.md#user_timelimit), issuing
  time warnings, and enforcing the session time limit; and
- issuing inactivity warnings and enforcing the configured inactivity limit.

Carrier loss, a sysop termination key, an expired time limit, or an inactivity
timeout can cause the normal OpenDoors shutdown path to terminate the door from
inside this function. Delaying kernel calls therefore delays detection of those
conditions and the handling of local keys; it does not disable them.

On Win32, the connection and time processing is performed by background
threads. A direct call still invokes
[`od_control.od_ker_exec`](../control/customization.md#od_ker_exec), but the
remainder of the single-threaded processing loop is omitted. Unix builds use
the single-threaded path unless configured to provide their own periodic kernel
mechanism.

## Return value

This function returns no value. It may terminate the session when an enabled
connection, time-limit, inactivity, or sysop policy requires shutdown.

## Example

The following structure services OpenDoors while a lengthy operation is split
into individual units:

```c
for(item = 0; item < item_count; ++item)
{
    process_one_item(item);
    od_kernel();
}
```

## See also

[`od_sleep()`](od_sleep.md), [`od_carrier()`](od_carrier.md),
[Session lifecycle](../../guides/session-lifecycle.md)
