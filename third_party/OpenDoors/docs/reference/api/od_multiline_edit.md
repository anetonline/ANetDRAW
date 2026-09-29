# `od_multiline_edit()`

Edits text which spans more than one line.

## Synopsis

```c
INT od_multiline_edit(char *pszBufferToEdit,
    UINT unBufferSize, tODEditOptions *pEditOptions);
```

## Description

[`od_multiline_edit()`](od_multiline_edit.md) provides an interactive text
editor with cursor movement, paging, insert and overwrite modes, optional word
wrapping, and an optional application menu. It can occupy the default
80-column by 23-row screen area or a smaller caller-selected rectangle.

`pszBufferToEdit` points to the initial, null-terminated text and receives the
edited text. `unBufferSize` is the complete capacity of that allocation,
including the terminating null byte; it is not the initial string length. The
initial terminator must occur within that capacity.

For a fixed buffer and all default options, pass a null third argument:

```c
char text[4000] = "";

if(od_multiline_edit(text, sizeof(text), NULL) != OD_MULTIEDIT_SUCCESS)
    od_exit(10);
```

To customize the editor, zero a
[`tODEditOptions`](../types.md#todeditoptions) structure and then set only the
members which differ from their defaults:

```c
char text[4000] = "";
tODEditOptions options;

memset(&options, 0, sizeof(options));
options.TextFormat = FORMAT_LINE_BREAKS;

od_multiline_edit(text, sizeof(text), &options);
```

## Editing keys

The editor recognizes the extended keys returned by
[`od_get_input()`](od_get_input.md) and their normal control-key alternatives.
The following operations are provided:

| Key | Operation |
| --- | --- |
| Arrow keys | Move by one character or one displayed line. |
| Home and End | Move to the beginning or end of the current line. |
| Page Up and Page Down | Move through the buffer by an edit-area page. |
| Insert | Toggle insert and overwrite modes. The initial mode is insert. |
| Delete | Delete the character at the cursor. |
| Backspace | Move left and delete the preceding character when one exists. |
| Ctrl-Y | Delete the current line. |
| Tab | Advance to the next eight-column tab stop, inserting spaces in insert mode. |
| Enter | Insert the selected line or paragraph break in insert mode; in overwrite mode, move to the following line unless already at the end. |
| Escape or Ctrl-Z | Invoke the menu callback, or leave the editor successfully when no callback is installed. |

Only bytes with values of 32 or greater are inserted as ordinary text. The
editor does not itself provide a separate accept-versus-cancel result. Without
a menu callback, Escape and Ctrl-Z finish successfully and leave all edits in
the buffer.

## Options

The [`tODEditOptions`](../types.md#todeditoptions) members have the following
purposes. See [Types and callbacks](../types.md#todeditoptions) for their exact
declarations and ownership rules.

### Edit area

[`nAreaLeft`](../types.md#narealeft-nareatop-narearight-nareabottom),
[`nAreaTop`](../types.md#narealeft-nareatop-narearight-nareabottom),
[`nAreaRight`](../types.md#narealeft-nareatop-narearight-nareabottom), and
[`nAreaBottom`](../types.md#narealeft-nareatop-narearight-nareabottom) are
one-based, inclusive coordinates. Their defaults are 1, 1, 80, and 23. In a
supplied options structure, each individual zero coordinate is replaced with
its corresponding default.

After zero values have been replaced by defaults, every coordinate must be at
least 1, the right edge must be greater than the left edge, and the bottom edge
must be greater than the top edge. The edit area therefore has at least two
columns and two rows. The right and bottom edges must not exceed the active
output window, which is the authoritative virtual session window when one is
present and the current local output window otherwise. AVATAR coordinates
must also fit in its byte-sized cursor commands and cannot exceed 255.

An invalid rectangle returns an error before OpenDoors derives its unsigned
dimensions or allocates editor bookkeeping.

### Text format

[`TextFormat`](../types.md#textformat) selects how logical lines and paragraphs
are represented:

| Value | Stored representation and wrapping |
| --- | --- |
| [`FORMAT_PARAGRAPH_BREAKS`](../constants/input.md#format_paragraph_breaks) | Store a break only at the end of a paragraph and word-wrap each paragraph for display. This is the default. |
| [`FORMAT_LINE_BREAKS`](../constants/input.md#format_line_breaks) | Store a break at the end of every logical line and word-wrap newly entered text. |
| [`FORMAT_NO_WORDWRAP`](../constants/input.md#format_no_wordwrap) | Store a break at the end of every logical line but do not word-wrap new text. |
| [`FORMAT_FTSC_MESSAGE`](../constants/input.md#format_ftsc_message) | Treat carriage return as a paragraph break, ignore line feeds, and skip FTSC kludge lines beginning with Ctrl-A when building the visible line index. |

For the first three formats, OpenDoors examines existing end-of-line sequences
to select carriage return, line feed, or the pair for newly inserted breaks.
If the buffer supplies no usable precedent, a line feed is used. FTSC mode
always uses carriage return for a new paragraph.

An unrecognized format returns
[`OD_MULTIEDIT_ERROR`](../constants/input.md#od_multiedit_error) and sets
[`od_control.od_error`](../control/runtime.md#od_error) to
[`ERR_PARAMETER`](../constants/errors.md#err_parameter).

### Menu callback

When [`pfMenuCallback`](../types.md#pfmenucallback) is non-null, Escape or
Ctrl-Z calls it synchronously with a null argument. Returning
[`EDIT_MENU_DO_NOTHING`](../constants/input.md#edit_menu_do_nothing) resumes
editing; returning
[`EDIT_MENU_EXIT_EDITOR`](../constants/input.md#edit_menu_exit_editor) finishes
successfully. After a resume result, OpenDoors restores the editor cursor
position, allowing the callback to draw a popup menu temporarily.

### Growable buffers

When [`pfBufferRealloc`](../types.md#pfbufferrealloc) is null, input which
cannot fit is rejected and the editor sounds the terminal bell. When it is
non-null, OpenDoors calls it with the current buffer pointer and a requested
larger capacity. The callback must obey `realloc()` semantics: preserve the
contents and return suitable storage, or return null without invalidating the
old allocation.

The callback may move the buffer. After the function returns, use
[`pszFinalBuffer`](../types.md#pszfinalbuffer), not the original pointer, to
find the edited allocation.
[`unFinalBufferSize`](../types.md#unfinalbuffersize) reports the complete
capacity of that allocation, including space for the terminating null byte.
It equals the original `unBufferSize` when no growth occurred and the most
recent successful callback request after growth.

The text buffer remains owned by the application whether it is fixed or
growable. OpenDoors allocates its line index and redraw workspace internally
and releases both before returning, including when setup, indexing, or editing
ends with an error.

[`dwEditFlags`](../types.md#dweditflags) is reserved. Set it to
[`EFLAG_NORMAL`](../constants/input.md#eflag_normal); the current implementation
does not read it.

## Terminal requirements

The editor requires ANSI or AVATAR cursor addressing. If neither
[`od_control.user_ansi`](../control/caller.md#user_ansi) nor
[`od_control.user_avatar`](../control/caller.md#user_avatar) is enabled, the
function returns an error. RIP alone is not tested, although a RIP session
which also enables ANSI uses the ANSI path.

Before drawing the editor, OpenDoors waits briefly for previously queued
output to drain. During large changes it may discard still-pending editor
redraw output and send a complete redraw instead. This optimization does not
discard application output which was already drained before entry.

## Return value

[`OD_MULTIEDIT_SUCCESS`](../constants/input.md#od_multiedit_success) means the
user left through Escape, Ctrl-Z, or a menu callback which requested exit.
[`OD_MULTIEDIT_ERROR`](../constants/input.md#od_multiedit_error) indicates a
parameter, graphics, or allocation failure. In the latter case,
[`od_control.od_error`](../control/runtime.md#od_error) is normally one of:

| Error | Condition |
| --- | --- |
| [`ERR_PARAMETER`](../constants/errors.md#err_parameter) | The buffer is null, its capacity is zero, or the text format is invalid. |
| [`ERR_NOGRAPHICS`](../constants/errors.md#err_nographics) | Neither ANSI nor AVATAR mode is enabled. |
| [`ERR_MEMORY`](../constants/errors.md#err_memory) | Required editor bookkeeping or buffer indexing cannot be allocated. |

## See also

[`od_edit_str()`](od_edit_str.md), [`od_input_str()`](od_input_str.md),
[`od_get_input()`](od_get_input.md), [Input constants](../constants/input.md),
[Types and callbacks](../types.md)
