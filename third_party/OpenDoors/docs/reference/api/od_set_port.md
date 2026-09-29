# `od_set_port()`

Selects the communications port before initialization

## Synopsis

```c
BOOL od_set_port(INT nPort);
```

## Return value

Returns [`TRUE`](../constants/general.md#true) when the port selection was
recorded. Returns [`FALSE`](../constants/general.md#false) when `nPort` is
outside the permitted range or OpenDoors has already been initialized.

## Description

[`od_set_port()`](od_set_port.md) records an explicit communications-port
selection. `nPort` is zero-based: 0 selects `COM1`, 1 selects `COM2`, and so
forth through 255.

This function is intended for the initialization sequence and, like
[`od_parse_cmd_line()`](od_parse_cmd_line.md), does not initialize OpenDoors.
It must be called before [`od_init()`](od_init.md) or any other function which
performs automatic initialization.

The selection is retained while a door-information file is read and takes
precedence over the port recorded there. This distinction is important for
`COM1`: the static value of
[`od_control.port`](../control/connection.md#port) is also zero, so assigning
zero directly to that field cannot tell OpenDoors whether the application
selected `COM1` or left the field untouched. Use `od_set_port(0)` when `COM1`
must override the drop file. A nonzero direct assignment made before
initialization continues to be recognized as an override.

The function selects the port which will be used when communications are
initialized; it does not open the port itself. After initialization, changing
[`od_control.port`](../control/connection.md#port) or calling this function
cannot move an established connection.

## Errors

A value below 0 or above 255 sets
[`ERR_PARAMETER`](../constants/errors.md#err_parameter). Calling the function
after OpenDoors has been initialized sets the same error. On either failure,
the existing port selection is unchanged.

## Example

```c
if(!od_set_port(0))
{
   /* The value was invalid or initialization has already occurred. */
   return 1;
}

od_init();
```

This explicitly selects `COM1`, even when the selected door-information file
names another port.

## See also

[`od_init()`](od_init.md), [`od_parse_cmd_line()`](od_parse_cmd_line.md),
[`od_control.port`](../control/connection.md#port)
