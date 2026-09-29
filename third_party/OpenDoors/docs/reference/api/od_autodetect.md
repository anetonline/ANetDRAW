# `od_autodetect()`

Attempts to determine the remote terminal's ANSI and RIP capabilities.

## Synopsis

```c
void od_autodetect(INT nFlags);
```

## Description

Most BBS door-information files report the caller's terminal capabilities, so
most doors do not need to call [`od_autodetect()`](od_autodetect.md). It is
provided for sessions in which that information is unavailable or unreliable.

The function tests ANSI first by sending the ANSI cursor-position query
`ESC [ 6 n`, followed by carriage return and spaces used to erase the query
from ordinary terminals. It waits up to 660 milliseconds for a response which
begins with `ESC [`.

It then tests RIP by sending carriage return, `ESC [ !`, and another carriage
return with erasing spaces. It waits up to 660 milliseconds for the characters
`RIP`, without regard to case, and discards the following eleven identification
bytes when that response is found. Each test is made only if the corresponding
capability is not already enabled.

Successful ANSI detection sets
[`od_control.user_ansi`](../control/caller.md#user_ansi) to
[`TRUE`](../constants/general.md#true). Successful RIP detection sets
[`od_control.user_rip`](../control/caller.md#user_rip) to
[`TRUE`](../constants/general.md#true). A failed or unanswered query does not
clear either field: many capable terminal programs do not implement these
queries, so failure is not evidence that the mode is unavailable.

In local mode, indicated by a zero
[`od_control.baud`](../control/connection.md#baud), OpenDoors enables ANSI and
returns without sending either query. The function cannot detect AVATAR because
there is no corresponding standard query; it never changes
[`od_control.user_avatar`](../control/caller.md#user_avatar).

The input queue is cleared before and after each remote test. Consequently,
input which was already waiting, response bytes which do not match the expected
prefix, and bytes received while a test is in progress may be discarded.

`nFlags` is reserved and ignored by the current implementation. Pass
[`DETECT_NORMAL`](../constants/display.md#detect_normal), the only defined
value, for compatibility with future versions.

## Return value

This function returns no value. Inspect
[`od_control.user_ansi`](../control/caller.md#user_ansi) and
[`od_control.user_rip`](../control/caller.md#user_rip) after the call.

## Example

Because a failed probe is inconclusive, a door which depends on ANSI should
still ask the caller when detection fails:

```c
od_autodetect(DETECT_NORMAL);

if(!od_control.user_ansi)
{
    od_printf("Does your terminal support ANSI graphics? (Y/N) ");
    if(od_get_answer("YN") == 'Y')
        od_control.user_ansi = TRUE;
}
```

## See also

[`od_clear_keybuffer()`](od_clear_keybuffer.md),
[Terminal and screen model](../../guides/terminal-screen.md),
[Display constants](../constants/display.md)
