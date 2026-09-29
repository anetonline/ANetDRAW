# `od_get_input_until()`

Retrieves one input event before an absolute session-time deadline.

## Synopsis

```c
BOOL od_get_input_until(tODInputEvent *pInputEvent,
    DWORD dwSeconds, WORD wMilliseconds, WORD wFlags);
```

## Description

[`od_get_input_until()`](od_get_input_until.md) has the same input translation,
event structure, and flags as [`od_get_input()`](od_get_input.md), but its
timeout is an absolute session time obtained from
[`od_get_time()`](od_get_time.md). `dwSeconds` is the whole-second component and
`wMilliseconds` must be from 0 through 999.

The deadline bounds the complete operation. If the first byte might begin a
multi-byte terminal sequence, the normal inter-byte wait is shortened as
necessary rather than extending past the supplied deadline. When the deadline
has already been reached, the function returns immediately without removing
an event from the input queue.

An absolute deadline is useful when other work must share a fixed interval.
Unlike repeatedly passing relative timeouts, recomputing the remaining time
inside OpenDoors prevents input translation and intermediate work from adding
delay to every iteration.

## Return value

The function returns [`TRUE`](../constants/general.md#true) after filling
`pInputEvent`. It returns [`FALSE`](../constants/general.md#false) if the
deadline is reached first. A null `pInputEvent` or a `wMilliseconds` value of
1000 or greater also returns [`FALSE`](../constants/general.md#false) and sets
[`od_control.od_error`](../control/runtime.md#od_error) to
[`ERR_PARAMETER`](../constants/errors.md#err_parameter).

## Example

This loop processes input for the remainder of a 100-millisecond frame:

```c
tODInputEvent event;
DWORD deadline_seconds;
WORD deadline_milliseconds;

od_get_time(&deadline_seconds, &deadline_milliseconds);
deadline_milliseconds += 100;
if(deadline_milliseconds >= 1000)
{
    deadline_milliseconds -= 1000;
    ++deadline_seconds;
}

while(od_get_input_until(&event, deadline_seconds,
    deadline_milliseconds, GETIN_NORMAL))
{
    process_input(&event);
}
```

## See also

[`od_get_time()`](od_get_time.md), [`od_get_input()`](od_get_input.md),
[`tODInputEvent`](../types.md#todinputevent),
[Input constants](../constants/input.md)
