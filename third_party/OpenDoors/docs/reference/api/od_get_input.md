# `od_get_input()`

Retrieves one character or translated extended-key event.

## Synopsis

```c
BOOL od_get_input(tODInputEvent *pInputEvent,
    tODMilliSec TimeToWait, WORD wFlags);
```

## Description

[`od_get_input()`](od_get_input.md) reads from the common remote/local input
queue. Unlike [`od_get_key()`](od_get_key.md), it reports where the input
originated and can combine a multi-byte terminal sequence into one logical
function, cursor, or navigation key.

`pInputEvent` points to the [`tODInputEvent`](../types.md#todinputevent) which
receives the result:

```c
typedef struct {
    tODInputEventType EventType;
    BOOL bFromRemote;
    char chKeyPress;
} tODInputEvent;
```

[`EVENT_CHARACTER`](../constants/input.md#event_character) means that
[`chKeyPress`](../types.md#chkeypress) contains one ordinary input byte.
[`EVENT_EXTENDED_KEY`](../constants/input.md#event_extended_key) means that it
contains one of the [`OD_KEY_*`](../constants/input.md#extended-key-codes)
values. [`bFromRemote`](../types.md#bfromremote) is
[`TRUE`](../constants/general.md#true) for remote
input and [`FALSE`](../constants/general.md#false) for enabled local-keyboard
input.

The translated extended values include:

| Values | Logical keys |
| --- | --- |
| [`OD_KEY_F1`](../constants/input.md#function-keys) through [`OD_KEY_F12`](../constants/input.md#function-keys) | Function keys F1 through F12 |
| [`OD_KEY_UP`](../constants/input.md#cursor-and-navigation-keys), [`OD_KEY_DOWN`](../constants/input.md#cursor-and-navigation-keys), [`OD_KEY_LEFT`](../constants/input.md#cursor-and-navigation-keys), [`OD_KEY_RIGHT`](../constants/input.md#cursor-and-navigation-keys) | Cursor movement |
| [`OD_KEY_INSERT`](../constants/input.md#cursor-and-navigation-keys), [`OD_KEY_DELETE`](../constants/input.md#cursor-and-navigation-keys) | Insert and Delete |
| [`OD_KEY_HOME`](../constants/input.md#cursor-and-navigation-keys), [`OD_KEY_END`](../constants/input.md#cursor-and-navigation-keys), [`OD_KEY_PGUP`](../constants/input.md#cursor-and-navigation-keys), [`OD_KEY_PGDN`](../constants/input.md#cursor-and-navigation-keys) | Navigation keys |
| [`OD_KEY_SHIFTTAB`](../constants/input.md#cursor-and-navigation-keys) | Shift-Tab |

When normal translation is enabled, OpenDoors also recognizes these
single-byte control-key alternatives:

| Control key | Extended result |
| --- | --- |
| Ctrl-E | [`OD_KEY_UP`](../constants/input.md#cursor-and-navigation-keys) |
| Ctrl-X | [`OD_KEY_DOWN`](../constants/input.md#cursor-and-navigation-keys) |
| Ctrl-S | [`OD_KEY_LEFT`](../constants/input.md#cursor-and-navigation-keys) |
| Ctrl-D | [`OD_KEY_RIGHT`](../constants/input.md#cursor-and-navigation-keys) |
| Ctrl-V | [`OD_KEY_INSERT`](../constants/input.md#cursor-and-navigation-keys) |
| Ctrl-G or DEL | [`OD_KEY_DELETE`](../constants/input.md#cursor-and-navigation-keys) |

`TimeToWait` specifies the maximum initial wait in milliseconds. Zero performs
an immediate queue check. [`OD_NO_TIMEOUT`](../constants/general.md#od_no_timeout) waits
indefinitely. Any other value supplies a finite timeout. Once the first byte of
a possible extended sequence has been received, OpenDoors can wait up to an
additional 250 milliseconds between sequence bytes before deciding whether to
return a translated key or the leading character. On 16-bit DOS, timer
resolution is approximately 55 milliseconds.

Use [`od_get_input_until()`](od_get_input_until.md) when one absolute deadline
must bound both the initial wait and any extended-sequence translation wait.

`wFlags` selects translation behavior:

| Flag | Behavior |
| --- | --- |
| [`GETIN_NORMAL`](../constants/input.md#getin_normal) | Translate recognized terminal sequences and control-key alternatives. |
| [`GETIN_RAW`](../constants/input.md#getin_raw) | Return remote bytes individually as character events, without sequence translation. |
| [`GETIN_RAWCTRL`](../constants/input.md#getin_rawctrl) | Translate terminal sequences but return the control-key alternatives as ordinary character events. |

The flags may be combined, although
[`GETIN_RAW`](../constants/input.md#getin_raw) necessarily prevents all
extended-sequence translation and therefore takes precedence over
[`GETIN_RAWCTRL`](../constants/input.md#getin_rawctrl).

This function does not update
[`od_control.od_last_input`](../control/runtime.md#od_last_input); use the
returned [`bFromRemote`](../types.md#bfromremote) field when the source matters.

## Return value

The function returns [`TRUE`](../constants/general.md#true) when it has filled
`pInputEvent`. A timeout or an immediate empty-queue check returns
[`FALSE`](../constants/general.md#false) without manufacturing an event. A null
`pInputEvent` also returns [`FALSE`](../constants/general.md#false) and sets
[`od_control.od_error`](../control/runtime.md#od_error) to
[`ERR_PARAMETER`](../constants/errors.md#err_parameter).

## Example

```c
tODInputEvent event;

if(od_get_input(&event, OD_NO_TIMEOUT, GETIN_NORMAL))
{
    if(event.EventType == EVENT_EXTENDED_KEY)
    {
        switch((unsigned char)event.chKeyPress)
        {
            case OD_KEY_UP:
                move_selection_up();
                break;

            case OD_KEY_DOWN:
                move_selection_down();
                break;
        }
    }
    else
    {
        process_character((unsigned char)event.chKeyPress);
    }
}
```

Cast [`chKeyPress`](../types.md#chkeypress) to `unsigned char` before comparing values above 127, notably
[`OD_KEY_F11`](../constants/input.md#function-keys) and
[`OD_KEY_F12`](../constants/input.md#function-keys), on systems where plain
`char` is signed.

## See also

[`od_get_key()`](od_get_key.md), [`od_key_pending()`](od_key_pending.md),
[`od_clear_keybuffer()`](od_clear_keybuffer.md),
[`od_get_input_until()`](od_get_input_until.md),
[Input constants](../constants/input.md), [Types and callbacks](../types.md)
