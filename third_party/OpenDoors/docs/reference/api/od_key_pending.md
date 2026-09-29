# `od_key_pending()`

Tests whether input is waiting without removing it from the input queue.

## Synopsis

```c
BOOL od_key_pending(void);
```

## Return value

Returns [`TRUE`](../constants/general.md#true) when the OpenDoors input queue
contains an event which can be obtained by the input functions. Returns
[`FALSE`](../constants/general.md#false) when the queue is empty at the time of
the test.

## Description

[`od_key_pending()`](od_key_pending.md) provides a non-blocking test for caller or local-console
input. Unlike [`od_get_key(FALSE)`](od_get_key.md), it does not remove the next
event from the queue and does not discard its extended-key information. A true
result may therefore be followed by one of the normal input calls to obtain
the same pending input.

Before examining the queue, the function gives the OpenDoors kernel an
opportunity to run. The kernel transfers newly arrived local and remote input
into the queue and performs normal connection, time-limit, inactivity, and
status processing. Consequently, input which arrived just before the call may
be visible in its result even if it had not previously been queued.

The result is only a snapshot. Another part of the program may consume the
input after [`od_key_pending()`](od_key_pending.md) returns, and new input may arrive immediately
after a false result. Code must not use the function as a guarantee that a
later blocking input operation cannot wait.

A typical polling loop is:

```c
while(od_carrier())
{
    if(od_key_pending())
    {
        char key = od_get_key(FALSE);
        /* Process key. */
    }
    else
    {
        /* Perform a small unit of application work. */
        od_kernel();
    }
}
```

Applications which need the distinction between ordinary characters,
extended keys, and other translated input should follow the test with
[`od_get_input()`](od_get_input.md). Applications interested only in an
eight-bit character may use [`od_get_key()`](od_get_key.md).

Calling [`od_key_pending()`](od_key_pending.md) initializes OpenDoors if necessary. A program must
set all initialization options before making this call.

## Errors

The function has no error return. A false result means only that no input is
currently queued; it does not report loss of carrier. Use
[`od_carrier()`](od_carrier.md) when the connection state is required.

## Example

The following helper drains every character currently waiting without ever
blocking for another one:

```c
static void discard_waiting_input(void)
{
    while(od_key_pending())
        (void)od_get_key(FALSE);
}
```

[`od_clear_keybuffer()`](od_clear_keybuffer.md) is preferred when the intent is
simply to clear all local and remote input buffers. The loop is useful when an
application needs to inspect or count each queued character.

## See also

[`od_get_key()`](od_get_key.md), [`od_get_input()`](od_get_input.md),
[`od_clear_keybuffer()`](od_clear_keybuffer.md), [`od_kernel()`](od_kernel.md)
