# `od_input_str()`

Inputs and echoes a simple line of text.

## Synopsis

```c
void od_input_str(char *pszInput, INT nMaxLength,
    unsigned char chMin, unsigned char chMax);
```

## Description

[`od_input_str()`](od_input_str.md) provides line input in plain ASCII, ANSI,
AVATAR, and RIP sessions. It accepts characters until the user presses Enter,
supports destructive backspace, echoes accepted characters, and displays a
line break when input is complete. It does not require cursor addressing.

`pszInput` points to the destination array. Input begins at position zero;
existing contents are neither displayed nor edited. `nMaxLength` is the
largest number of input bytes which can be stored, excluding the terminating
null byte. The array must therefore provide at least `nMaxLength + 1` bytes.

`chMin` and `chMax` specify the inclusive range of unsigned byte values which
may be entered. The entire range from 0 through 255 may be specified,
including values above 127. Characters outside the range are ignored, as are
additional characters after `nMaxLength` has been reached. Carriage return and
line feed finish input, and backspace edits input, before the range test is
applied; those control characters cannot be stored as ordinary data through
this function.

Accepted bytes are stored unchanged in the `char` array. On an implementation
where plain `char` is signed, a stored byte above 127 consequently has a
negative value when read as `char`; convert it to `unsigned char` when its
numeric byte value is required.

For example, to accept at most two decimal digits:

```c
char number[3];

od_input_str(number, 2, '0', '9');
```

To accept a name of at most 35 printable seven-bit characters:

```c
char name[36];

od_input_str(name, 35, 32, 126);
```

Use [`od_edit_str()`](od_edit_str.md) when the application needs formatted
fields, cursor movement, editing of an existing value, password display, or a
distinguishable cancel result. Use
[`od_multiline_edit()`](od_multiline_edit.md) for text spanning several lines.

## Return value

This function returns no value. When Enter is pressed, it null-terminates the
accepted input in `pszInput`.

A null destination, a maximum length below one, or a minimum character greater
than the maximum causes the function to return without accepting input and set
[`od_control.od_error`](../control/runtime.md#od_error) to
[`ERR_PARAMETER`](../constants/errors.md#err_parameter). Existing destination
contents are not changed in that case.

## See also

[`od_edit_str()`](od_edit_str.md),
[`od_multiline_edit()`](od_multiline_edit.md),
[`od_get_key()`](od_get_key.md),
[`od_clear_keybuffer()`](od_clear_keybuffer.md)
