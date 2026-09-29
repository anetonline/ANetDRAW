# `od_get_key()`

Retrieves one byte from the common remote/local input queue.

## Synopsis

```c
char od_get_key(BOOL bWait);
```

## Description

[`od_get_key()`](od_get_key.md) returns remote keystrokes and enabled local
keyboard input in arrival order. Local keys consumed as OpenDoors sysop
commands do not enter this queue; other local input can therefore operate the
door in the same manner as remote input.

If `bWait` is [`TRUE`](../constants/general.md#true), the function waits until
an acceptable character is available. While waiting, a single-threaded build
continues normal [`od_kernel()`](od_kernel.md) processing and yields time to
other tasks. If `bWait` is [`FALSE`](../constants/general.md#false), an empty
queue causes an immediate zero return.

Line-feed bytes are discarded. This accommodates connections which supply the
carriage-return/line-feed pair for Enter: the carriage return is returned and
the following line feed is silently removed by the next call. After discarding
a line feed, a non-waiting call checks the queue again. It returns the next
non-line-feed byte when one is already queued, or zero when the queue is empty.

No extended-key translation is performed. DOS-style extended input may
therefore appear as a zero byte followed by a scan-code byte, and a genuine
zero input byte is indistinguishable from the zero returned by an empty
non-waiting call. Use [`od_get_input()`](od_get_input.md) when the source,
extended-key identity, or distinction between those cases is required.

After a byte is accepted, OpenDoors sets
[`od_control.od_last_input`](../control/runtime.md#od_last_input) to zero for
remote input or one for local input. This assignment occurs before the
line-feed test, so a discarded line feed records its source even when a
non-waiting call subsequently returns zero. A call which finds the queue empty
without removing an event does not change the field.

## Return value

The function returns the next non-line-feed byte. When `bWait` is
[`FALSE`](../constants/general.md#false) and no event is waiting, it returns
zero. There is no separate error result.

## Examples

Wait for an Enter key:

```c
char key;

do {
    key = od_get_key(TRUE);
} while(key != '\r');
```

Poll for an abort key while other processing continues:

```c
while(more_output())
{
    display_next_part();
    if(od_get_key(FALSE) == 27)
        break;
}
```

## See also

[`od_get_input()`](od_get_input.md), [`od_key_pending()`](od_key_pending.md),
[`od_input_str()`](od_input_str.md),
[`od_clear_keybuffer()`](od_clear_keybuffer.md)
