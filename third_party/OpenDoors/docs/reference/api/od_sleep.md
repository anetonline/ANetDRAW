# `od_sleep()`

Suspends execution while yielding processor time

## Synopsis

```c
void od_sleep(tODMilliSec Milliseconds);
```

## Return value

N/A

## Description

[`od_sleep()`](od_sleep.md) suspends the calling thread for approximately `Milliseconds`
milliseconds while using the platform's cooperative or operating-system wait
facility. The delay is a minimum scheduling request rather than a precise
real-time deadline; timer resolution and other runnable work may make the
actual delay longer.

A value of zero requests a yield without an ordinary delay. On Windows this
is `Sleep(0)`. Unix-like targets request a 100-microsecond sleep. The 16-bit
DOS implementation performs one platform yield, and the 32-bit DOS
implementation issues multiplex interrupt `2Fh`, function `1680h`, through
its real-mode interrupt service.

For a nonzero DOS delay, OpenDoors rounds the request up to a whole BIOS tick,
then waits on its BIOS-tick-based timer and yields while necessary. Its
resolution is approximately 55 milliseconds, so even `od_sleep(1)` waits for
at least one tick rather than becoming a zero-delay yield.
Windows uses `Sleep()`. Unix-like targets use `nanosleep()` and resume the
remaining interval if a signal interrupts the call.

This function is useful in a polling loop which cannot call a blocking
OpenDoors input function. A busy loop which repeatedly tests input or carrier
can consume an entire processor timeslice; a short call to [`od_sleep()`](od_sleep.md) allows
other programs or threads to run. Excessive zero-delay yields may also reduce
the door's throughput, so they should be placed where the application truly
has no immediate work.

OpenDoors performs such yields internally while waiting for input, serial
output space, display-file pauses, popup and hotkey menus, editor input, and
chat. Application code does not need to add a yield around those blocking
interfaces.

Calling [`od_sleep()`](od_sleep.md) initializes OpenDoors if necessary. It does
not call [`od_kernel()`](od_kernel.md). A program which must continue kernel
processing should divide long work into appropriate intervals and call the
kernel explicitly. The function returns no value.

## Example

```c
while(!od_key_pending())
{
   perform_background_work();
   od_kernel();
   od_sleep(10);
}
```

## See also

[`od_kernel()`](od_kernel.md), [`od_key_pending()`](od_key_pending.md),
[`od_get_time()`](od_get_time.md), [`tODMilliSec`](../types.md#todmillisec)
