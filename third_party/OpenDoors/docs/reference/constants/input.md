# Input and editors

OpenDoors translates local and remote input into ordinary characters or
extended key events. The single-line editor and multiline editor define their
own independent flag and result sets. Do not mix values merely because their
underlying integer representations overlap.

## Input event kinds

### `EVENT_CHARACTER`

[`tODInputEvent.EventType`](../types.md#eventtype) has this value when
[`od_get_input()`](../api/od_get_input.md) returns an ordinary input byte.
`chKeyPress` contains that byte and `bFromRemote` identifies its source.

“Character” does not imply printable ASCII. Control characters, Escape when it
is not translated as part of a longer key sequence, and bytes with the high bit
set can all be character events.

### `EVENT_EXTENDED_KEY`

The event has this value when OpenDoors recognized a function, cursor,
navigation, or Shift-Tab key and translated it to one `OD_KEY_*` code. Compare
`chKeyPress` after converting it to `unsigned char`, since F11 and F12 have
values above the positive range of a signed eight-bit `char`.

## Input translation flags

Pass these bit values to [`od_get_input()`](../api/od_get_input.md). Use
[`OD_NO_TIMEOUT`](general.md#od_no_timeout) for an indefinite wait or a finite
[`tODMilliSec`](../types.md#todmillisec) interval when other work must continue.

### `GETIN_NORMAL`

The zero/default setting. OpenDoors recognizes supported multi-byte terminal
key sequences and established control-key alternatives, returning one
`EVENT_EXTENDED_KEY` for a recognized logical key. Ordinary input remains an
`EVENT_CHARACTER`.

After receiving the beginning of a possible sequence, the current
implementation waits up to 250 milliseconds between bytes. This value is the
internal `MAX_CHARACTER_LATENCY` constant in `ODGetIn.c`; the retained
[`od_control.od_max_key_latency`](../control/runtime.md#od_max_key_latency)
member is not read.

### `GETIN_RAW`

Return incoming bytes individually without extended-key sequence translation.
This is appropriate for an application implementing its own byte-level
protocol or terminal parser. Each byte is delivered as `EVENT_CHARACTER`.

`GETIN_RAW` takes precedence over the translation behavior implied by
`GETIN_RAWCTRL`; combining both cannot preserve extended sequences while also
returning every byte separately.

Raw mode does not bypass carrier checks, session timing, the common input
queue, or local/remote origin reporting. It changes key interpretation only.

### `GETIN_RAWCTRL`

Continue to translate terminal escape sequences into `OD_KEY_*` events, but do
not map established single-byte control-key alternatives into those extended
keys. The original control byte is returned as `EVENT_CHARACTER`.

This setting is useful when Ctrl-letter input is meaningful to the door while
cursor and function keys should still be portable across terminals.

## Extended key codes

The values follow the established IBM PC BIOS scan-code representation used by
OpenDoors. They are logical OpenDoors key values, not Windows virtual-key codes,
ANSI final bytes, Unicode code points, or complete local `bioskey()` results.

### Function keys

| Name | Value | Logical key |
| --- | ---: | --- |
| `OD_KEY_F1` | `0x3b` | F1 |
| `OD_KEY_F2` | `0x3c` | F2 |
| `OD_KEY_F3` | `0x3d` | F3 |
| `OD_KEY_F4` | `0x3e` | F4 |
| `OD_KEY_F5` | `0x3f` | F5 |
| `OD_KEY_F6` | `0x40` | F6 |
| `OD_KEY_F7` | `0x41` | F7 |
| `OD_KEY_F8` | `0x42` | F8 |
| `OD_KEY_F9` | `0x43` | F9 |
| `OD_KEY_F10` | `0x44` | F10 |
| `OD_KEY_F11` | `0x85` | F11 |
| `OD_KEY_F12` | `0x86` | F12 |

Terminal software does not encode function keys uniformly. OpenDoors
recognizes the sequences supported by its input translator and normalizes them
to these values. A terminal which sends an unknown sequence may produce
ordinary Escape and character events instead.

### Cursor and navigation keys

| Name | Value | Logical key |
| --- | ---: | --- |
| `OD_KEY_HOME` | `0x47` | Home |
| `OD_KEY_UP` | `0x48` | Up arrow |
| `OD_KEY_PGUP` | `0x49` | Page Up |
| `OD_KEY_LEFT` | `0x4b` | Left arrow |
| `OD_KEY_RIGHT` | `0x4d` | Right arrow |
| `OD_KEY_END` | `0x4f` | End |
| `OD_KEY_DOWN` | `0x50` | Down arrow |
| `OD_KEY_PGDN` | `0x51` | Page Down |
| `OD_KEY_INSERT` | `0x52` | Insert |
| `OD_KEY_DELETE` | `0x53` | Delete |
| `OD_KEY_SHIFTTAB` | `0x0f` | Shift-Tab |

Plain Tab is the character byte `0x09`; Shift-Tab is extended because it has no
distinct ordinary ASCII character. Backspace and the ASCII Delete byte are
also not interchangeable with `OD_KEY_DELETE`.

## Single-line editor flags

Combine compatible flags with bitwise OR and pass them to
[`od_edit_str()`](../api/od_edit_str.md). The input-format string, initial
buffer, maximum length, and flags together define the field. Test every
combination used by an application; some policies intentionally change what
acceptance and cancellation mean.

### `EDIT_FLAG_NORMAL`

The zero/default editor behavior. The field is drawn normally, input begins
according to the supplied initial string rules, and Enter accepts the result.
It contributes no bits when combined with another flag.

### `EDIT_FLAG_NO_REDRAW`

Skip the normal initial field drawing and the ordinary final redraw. The
application is responsible for having placed a correct field image at the
cursor location. Characters entered during the call still update the field.

Cancellation and any final normalization which changes buffer characters jump
to the editor's redraw path even when this flag is set. It is therefore a
request to suppress the routine redraws, not a guarantee that the call never
draws the complete field.

### `EDIT_FLAG_FIELD_MODE`

Enable multi-field navigation results. Previous-field and next-field keys end
this edit with `EDIT_RETURN_PREVIOUS` or `EDIT_RETURN_NEXT` so the application
can move focus and invoke another editor.

Up Arrow or Shift-Tab requests the previous field; Down Arrow or Tab requests
the next. The navigation result is returned only after the current value passes
the same fill and format checks used for acceptance. Without field mode, these
keys are ignored by this editor. The application remains responsible for field
order and focus drawing.

### `EDIT_FLAG_EDIT_STRING`

Treat the supplied initial string as text to be edited. The caller can move
within it, insert, delete, and accept a modified value. Without this flag,
[`od_edit_str()`](../api/od_edit_str.md) sets `pszInput[0]` to nul before input begins; the previous
contents are neither displayed as a default nor preserved for cancellation.

Ensure that the initial string is nul terminated within the supplied capacity
and valid for the format before enabling strict input.

### `EDIT_FLAG_STRICT_INPUT`

Turn off insert mode and disable the Insert and Delete operations and
middle-of-string Backspace deletion. Newly typed characters must be valid for
their current format position, and the complete string is checked again before
acceptance or field navigation. This keeps character classes aligned without
shifting existing characters between positions.

### `EDIT_FLAG_PASSWORD_MODE`

Mask entered characters on the display using the `chBlank` argument passed to
[`od_edit_str()`](../api/od_edit_str.md).
The actual buffer still contains the entered value. This prevents casual local
or remote observation but does not encrypt memory, a drop file, a log, or the
transport.

Do not display or log the resulting buffer after the editor returns. Clear
sensitive storage when the application no longer needs it.

### `EDIT_FLAG_ALLOW_CANCEL`

Allow Escape to end editing with `EDIT_RETURN_CANCEL`. The editor restores the
original value rather than returning a partial edit. The application should
leave the underlying setting unchanged when it receives this result.

Without this flag, Escape is not a guaranteed cancellation action.

### `EDIT_FLAG_FILL_STRING`

Require the C string length to equal the total number of positions represented
by the format before the value can be accepted. That count includes permanent
literal positions as well as user-entered positions. This is appropriate for a
complete fixed-length representation but not for a field where trailing
positions are optional.

### `EDIT_FLAG_AUTO_ENTER`

Automatically accept when the last available input position is filled. The
user does not need to press Enter. The result is `EDIT_RETURN_ACCEPT` if all
other validation succeeds.

Combine this with `EDIT_FLAG_FILL_STRING` for a fixed-width field which should
advance immediately after completion. Avoid it where the user may reasonably
want to review or correct a full value before accepting it.

### `EDIT_FLAG_AUTO_DELETE`

When an existing value is displayed, the first ordinary input character clears
the editable contents and begins a replacement. Navigation or editing keys can
still allow the user to edit the existing value rather than replace it.

This reproduces the familiar “type to replace, move to edit” field behavior.
Preserve the original value separately if cancellation is also allowed.

### `EDIT_FLAG_KEEP_BLANK`

Leave field-background/fill characters visible in unused positions after the
edit ends. Without it, normal cleanup may remove those characters and leave
only the returned text and literals.

The fill characters are display state, not bytes appended to the returned C
string.

### `EDIT_FLAG_PERMALITERAL`

Keep literal characters from the format permanently displayed and prevent
editing operations from deleting or displacing them. The returned value
contains literals according to the editor's established format behavior.

Use this for punctuation such as date separators or telephone formatting.
The format and buffer capacity must include space for the representation that
the editor returns.

### `EDIT_FLAG_LEAVE_BLANK`

After validation, count literal format positions from the beginning of the
stored string. If the nonempty string ends before the first nonliteral position
is encountered—in other words, every stored character is an initial format
literal—replace it with an empty string. Fill characters are display state and
are never part of this test. This behavior is narrower than a general scan for
“no user-entered characters”; once a nonliteral position occurs, later content
is not examined by this flag.

### `EDIT_FLAG_SHOW_SIZE`

Display exactly the permitted input width instead of reserving the editor's
usual additional cursor cell. This is useful when the field must fit a fixed
screen rectangle. It changes presentation width, not the number of bytes the
application must allocate.

## Single-line editor results

Always inspect the return value before using the edited buffer.

### `EDIT_RETURN_ERROR`

The editor failed. Save
[`od_control.od_error`](../control/runtime.md#od_error) immediately. The
application must not treat the buffer as newly accepted input.

### `EDIT_RETURN_CANCEL`

The user cancelled an editor opened with `EDIT_FLAG_ALLOW_CANCEL`. The original
value has been restored according to the editor contract.

### `EDIT_RETURN_ACCEPT`

The user accepted the current value, or `EDIT_FLAG_AUTO_ENTER` accepted it when
the field became complete. The application may validate any higher-level rule
and store the result.

### `EDIT_RETURN_PREVIOUS`

Field mode ended with an Up Arrow or Shift-Tab request after the current value
passed the editor's acceptance checks. The application normally moves focus
backward and calls the appropriate field editor.

### `EDIT_RETURN_NEXT`

Field mode ended with a Down Arrow or Tab request after the current value
passed the editor's acceptance checks. It is not necessarily the same as
accepting the form: the application decides where focus moves and when the
complete form commits.

## Multiline text formats

These enumeration values are assigned to
[`tODEditOptions.TextFormat`](../types.md#textformat).

### `FORMAT_PARAGRAPH_BREAKS`

Word-wrap long text for the edit area, but store no line-ending bytes for rows
created only by wrapping. Explicit paragraph boundaries retain the first line
ending style encountered in the supplied buffer: CR, LF, CRLF, or LFCR. If the
buffer contains no such boundary, newly entered paragraphs use LF. A consumer
can therefore reflow the stored paragraphs at another display width.

### `FORMAT_LINE_BREAKS`

Word-wrap long text for presentation. Enter inserts the first line-ending style
encountered in the supplied buffer: CR, LF, CRLF, or LFCR; if no ending exists
in the initial text, it inserts LF. The current implementation does not insert
line-ending bytes merely because an existing long line wraps visually.

### `FORMAT_FTSC_MESSAGE`

Use CR as the stored paragraph terminator and no separate line-break sequence.
During indexing the editor removes LF (`0x0a`) and soft carriage return
(`0x8d`) bytes. Lines beginning with Control-A (`0x01`) are treated as FTSC
kludge lines and skipped by the editable display rather than parsed as message
text. Newly entered text is word-wrapped for presentation, while Enter inserts
a CR paragraph boundary. This is not a synonym for CRLF text.

### `FORMAT_NO_WORDWRAP`

Do not move a visual break back to the preceding space. Stored lines may remain
arbitrarily long; the editor divides their presentation at the edit width by
character instead of by word. Explicit line endings use the first CR, LF,
CRLF, or LFCR style found in the initial buffer, defaulting to LF when none is
present.

## Multiline callback results and flags

### `EDIT_MENU_DO_NOTHING`

The optional menu callback has completed its action and requests that the
multiline editor continue. This value is returned by the callback, not by
[`od_multiline_edit()`](../api/od_multiline_edit.md).

### `EDIT_MENU_EXIT_EDITOR`

The menu callback requests that the editor finish. The editor then completes
its normal final-buffer processing and returns its own success or error result.

### `EFLAG_NORMAL`

The zero/default value for [`tODEditOptions.dwEditFlags`](../types.md#dweditflags).
No other public multiline edit bits are defined, and the current implementation
does not inspect the member.

## Multiline editor results

### `OD_MULTIEDIT_ERROR`

[`od_multiline_edit()`](../api/od_multiline_edit.md) failed. Inspect [`od_error`](../control/runtime.md#od_error)
and do not assume that `pszFinalBuffer` contains an accepted result.

### `OD_MULTIEDIT_SUCCESS`

Editing completed successfully. Read `pszFinalBuffer` from the options
structure because reallocation may have changed the pointer.
`unFinalBufferSize` gives the complete capacity of the returned allocation as
described under [`tODEditOptions`](../types.md#unfinalbuffersize).
