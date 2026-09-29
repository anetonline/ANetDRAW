# `od_edit_str()`

Inputs or edits one formatted field with full cursor editing.

## Synopsis

```c
WORD od_edit_str(char *pszInput, char *pszFormat, INT nRow,
    INT nColumn, BYTE btNormalColour, BYTE btHighlightColour,
    char chBlank, WORD nFlags);
```

## Description

[`od_edit_str()`](od_edit_str.md) provides insertion, overwrite, cursor
movement, character deletion, format validation, literal insertion, password
masking, and multi-field navigation. It is intended for cursor-addressable
terminal sessions. For simple input which also works in plain ASCII mode, use
[`od_input_str()`](od_input_str.md).

`pszInput` points to the destination buffer. When
[`EDIT_FLAG_EDIT_STRING`](../constants/input.md#edit_flag_edit_string) is set,
the buffer must initially contain a null-terminated value to edit; otherwise
the function immediately makes it an empty string. The buffer must have room
for every position represented by `pszFormat` plus the terminating null byte.

If an initial edit string is longer than the represented field, OpenDoors
truncates it to the field length before saving the value used by cancellation.
Consequently, [`EDIT_RETURN_CANCEL`](../constants/input.md#edit_return_cancel)
restores the truncated starting value, not bytes which were beyond the field.

`nRow` and `nColumn` are the one-based row and column of the field's first
position. `btHighlightColour` is the complete IBM text attribute used while
the field is active; `btNormalColour` is used for the normal final display.
`chBlank` fills unused field positions. In password mode, `chBlank` instead
masks each stored character and spaces fill the unused positions.

Unless [`EDIT_FLAG_SHOW_SIZE`](../constants/input.md#edit_flag_show_size) is
set, the displayed field includes one extra cursor cell after its maximum
stored length. The complete field must fit within the active local or virtual
screen window, and its trailing working cell must remain before the window's
last column. OpenDoors reserves that final column because writing it can wrap
the terminal cursor and writing the lower-right cell can scroll the screen.
The same placement requirement applies with
[`EDIT_FLAG_SHOW_SIZE`](../constants/input.md#edit_flag_show_size) and
[`EDIT_FLAG_NO_REDRAW`](../constants/input.md#edit_flag_no_redraw), since
editing and exceptional redraw paths can still address the trailing cell.

## Format string

`pszFormat` defines both the maximum stored length and the character class of
each position. Format letters are case-insensitive. Unquoted spaces are ignored
and may be inserted to make a format more readable; they do not occupy field
positions. At most 80 editable or literal positions may be represented.

The format character should describe the meaning of the field position, not
merely the characters which happen to pass validation. For example, `F` is
intended for a filename or path, while `W` is intended for a filename or path
pattern containing wildcards.

| Character | Intended input | Accepted input and conversion |
| --- | --- | --- |
| `#` | Numeric position | Decimal digit `0` through `9` |
| `%` | Numeric position which may be blank | Decimal digit or space |
| `9` | Floating-point number position | Decimal digit, `.`, `+`, or `-` |
| `?` | Unrestricted position | Any input byte not already handled as an editor command |
| `*` | Printable-text position | Any value which the implementation treats as 32 or greater |
| `A` | Alphabetic-text position | ASCII letter or space |
| `C` | City or location-name position | ASCII letter, space, comma, period, `*`, or `?`; the first letter of each word is converted to upper case and remaining letters to lower case |
| `D` | Date position | Decimal digit, `-`, or `/` |
| `F` | Filename or path position | ASCII letter or digit, the platform directory separator, or one of `: . ? * # $ & ' ( > - @ _ ! { } ~`; letters are converted to upper case |
| `H` | Hexadecimal-number position | Hexadecimal digit `0` through `9`, `A` through `F`, or `a` through `f` |
| `L` | Lower-case alphabetic position | ASCII letter or space; letters are converted to lower case |
| `M` | Name position | ASCII letter or space; the first letter of each word is converted to upper case and remaining letters to lower case |
| `T` | Telephone-number position | Decimal digit, space, `-`, `+`, `(`, or `)` |
| `U` | Upper-case alphabetic position | ASCII letter or space; letters are converted to upper case |
| `W` | Wildcard filename or path-pattern position | The same characters accepted by `F`, including `*` and `?`; letter case is retained |
| `X` | Alphanumeric-text position | ASCII letter, decimal digit, or space |
| `Y` | Yes-or-no position | `Y`, `N`, `y`, or `n`; the stored value is converted to upper case |

The accepted character sets of `F` and `W` overlap: both currently permit
`*` and `?`. Their intended uses and conversion differ. Use `F` for ordinary
filename or path input where upper-case storage is wanted. Use `W` for a file
selection pattern, such as `FILE2.*`, and when the entered letter case should
be retained.

An otherwise unrecognized unquoted format character behaves like an
unrestricted editable position in the current implementation. Applications
should use `?` explicitly instead of depending on that fallback.

Single or double quotes delimit literal characters. The opening and closing
delimiter must use the same quote:

```c
"###'-'###'-'####"
```

represents ten numeric positions and two literal hyphens. The delimiters do
not occupy stored positions, but the literal characters do. Literal text is
inserted automatically as the cursor reaches it. In a C string, a double quote
used as a format delimiter must itself be escaped.

[`EDIT_FLAG_PERMALITERAL`](../constants/input.md#edit_flag_permaliteral) draws
all literals from the beginning and prevents editing operations from moving or
deleting them. Without that flag, a literal appears when preceding input
reaches it.

## Editing keys

The function obtains input through [`od_get_input()`](od_get_input.md), so it
recognizes supported ANSI, VT-style, and DoorWay extended sequences as well as
the established control-key alternatives.

| Key | Operation |
| --- | --- |
| Home and End | Move to the beginning or end of the current value. |
| Left and Right Arrow | Move by one stored position. |
| Insert or Ctrl-V | Toggle insert and overwrite modes, unless strict or permanent-literal mode disables insertion. |
| Delete or Ctrl-G | Delete the character under the cursor when permitted. |
| Backspace or Ctrl-H | Delete the preceding character when permitted. |
| Ctrl-Y | Erase the entire field. |
| Enter or Ctrl-Z | Attempt to accept the field. Invalid or insufficient input remains in the editor. |
| Escape | Cancel only when [`EDIT_FLAG_ALLOW_CANCEL`](../constants/input.md#edit_flag_allow_cancel) is set. |
| Down Arrow or Tab | Request the next field when [`EDIT_FLAG_FIELD_MODE`](../constants/input.md#edit_flag_field_mode) is set. |
| Up Arrow or Shift-Tab | Request the previous field when [`EDIT_FLAG_FIELD_MODE`](../constants/input.md#edit_flag_field_mode) is set. |

The function temporarily claims the DOS Up and Down Arrow keys from the normal
sysop time-adjustment handling, so these editing operations are also available
from the enabled local keyboard.

## Flags

Combine compatible [`EDIT_FLAG_*`](../constants/input.md#single-line-editor-flags)
values with bitwise OR. The complete behavioral definitions are in
[Input and editors](../constants/input.md#single-line-editor-flags).

| Flag | Effect |
| --- | --- |
| [`EDIT_FLAG_NORMAL`](../constants/input.md#edit_flag_normal) | Default behavior; contributes no bits. |
| [`EDIT_FLAG_NO_REDRAW`](../constants/input.md#edit_flag_no_redraw) | Suppress the ordinary initial and final complete redraws. Cancellation and normalization can still require a redraw. |
| [`EDIT_FLAG_FIELD_MODE`](../constants/input.md#edit_flag_field_mode) | Permit previous-field and next-field results. |
| [`EDIT_FLAG_EDIT_STRING`](../constants/input.md#edit_flag_edit_string) | Edit the initial value instead of starting empty. |
| [`EDIT_FLAG_STRICT_INPUT`](../constants/input.md#edit_flag_strict_input) | Disable insertion, Delete, and middle-of-string Backspace so positions remain aligned with different format classes. |
| [`EDIT_FLAG_PASSWORD_MODE`](../constants/input.md#edit_flag_password_mode) | Display `chBlank` instead of the stored characters. |
| [`EDIT_FLAG_ALLOW_CANCEL`](../constants/input.md#edit_flag_allow_cancel) | Let Escape restore the starting value and return a cancel result. |
| [`EDIT_FLAG_FILL_STRING`](../constants/input.md#edit_flag_fill_string) | Require every represented position, including literals, to be present before acceptance. |
| [`EDIT_FLAG_AUTO_ENTER`](../constants/input.md#edit_flag_auto_enter) | Attempt acceptance automatically when the field reaches its maximum stored length. |
| [`EDIT_FLAG_AUTO_DELETE`](../constants/input.md#edit_flag_auto_delete) | Let the first ordinary input character replace an existing value while navigation/edit commands preserve it. |
| [`EDIT_FLAG_KEEP_BLANK`](../constants/input.md#edit_flag_keep_blank) | Leave fill characters in unused positions after editing. |
| [`EDIT_FLAG_PERMALITERAL`](../constants/input.md#edit_flag_permaliteral) | Keep format literals visible and fixed; insertion mode is disabled. |
| [`EDIT_FLAG_LEAVE_BLANK`](../constants/input.md#edit_flag_leave_blank) | Return an empty string when the only stored characters are leading literals. |
| [`EDIT_FLAG_SHOW_SIZE`](../constants/input.md#edit_flag_show_size) | Omit the usual extra displayed cursor cell. |

## Return value

Always inspect the result before using the edited value:

| Result | Meaning |
| --- | --- |
| [`EDIT_RETURN_ERROR`](../constants/input.md#edit_return_error) | A parameter or format error prevented editing. |
| [`EDIT_RETURN_CANCEL`](../constants/input.md#edit_return_cancel) | Escape was accepted and the saved starting value was restored. |
| [`EDIT_RETURN_ACCEPT`](../constants/input.md#edit_return_accept) | Enter, Ctrl-Z, or automatic entry accepted a valid value. |
| [`EDIT_RETURN_PREVIOUS`](../constants/input.md#edit_return_previous) | Field mode accepted the value and requested the previous field. |
| [`EDIT_RETURN_NEXT`](../constants/input.md#edit_return_next) | Field mode accepted the value and requested the next field. |

A null input or format pointer, a row or column below one, an empty format, a
format representing more than 80 positions, or a field which does not fit the
active screen as described above returns
[`EDIT_RETURN_ERROR`](../constants/input.md#edit_return_error) and sets
[`od_control.od_error`](../control/runtime.md#od_error) to
[`ERR_PARAMETER`](../constants/errors.md#err_parameter).

Despite its cursor-addressed design, the current implementation does not
perform an explicit ANSI/AVATAR capability check and does not define
[`ERR_NOGRAPHICS`](../constants/errors.md#err_nographics) as a reliable return
condition for this function.

## Examples

Input a name of at most 25 characters, applying word capitalization:

```c
char name[26];

od_edit_str(name, "MMMMMMMMMMMMMMMMMMMMMMMMM", 1, 1,
    0x03, 0x21, 176, EDIT_FLAG_NORMAL);
```

Require a complete North American telephone-number form:

```c
char phone[13];

od_edit_str(phone, "###'-'###'-'####", 1, 1,
    0x03, 0x21, 176,
    EDIT_FLAG_FILL_STRING | EDIT_FLAG_STRICT_INPUT);
```

Edit an existing value, allowing the first typed character to replace it:

```c
char description[21] = "Existing value";

od_edit_str(description, "????????????????????", 1, 1,
    0x03, 0x21, 176,
    EDIT_FLAG_EDIT_STRING | EDIT_FLAG_AUTO_DELETE |
    EDIT_FLAG_ALLOW_CANCEL);
```

Input a masked, upper-case password:

```c
char password[17];

od_edit_str(password, "UUUUUUUUUUUUUUUU", 1, 1,
    0x03, 0x21, 254, EDIT_FLAG_PASSWORD_MODE);
```

Display a fixed date layout from the start of editing:

```c
char date[12];

od_edit_str(date, "UUU'-'##'-19'##", 1, 1,
    0x03, 0x21, 176,
    EDIT_FLAG_PERMALITERAL | EDIT_FLAG_FILL_STRING);
```

## See also

[`od_input_str()`](od_input_str.md),
[`od_multiline_edit()`](od_multiline_edit.md),
[`od_get_input()`](od_get_input.md),
[Input and editors](../constants/input.md)
