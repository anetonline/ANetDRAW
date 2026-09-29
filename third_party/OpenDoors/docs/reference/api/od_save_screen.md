# `od_save_screen()`

Saves the legacy 80-column local screen

## Synopsis

```c
BOOL od_save_screen(void *pBuffer);
```

## Return value

Returns [`TRUE`](../constants/general.md#true) when the screen has been saved,
or [`FALSE`](../constants/general.md#false) on failure.

## Description

[`od_save_screen()`](od_save_screen.md) saves the contents of the active local text window,
together with its cursor position and current display attribute, for later use
by [`od_restore_screen()`](od_restore_screen.md). It retains the original
OpenDoors fixed-width buffer format and is available in plain-ASCII, ANSI,
AVATAR, and RIP modes.

`pBuffer` must address at least 4,004 writable bytes. The saved representation
is public legacy behavior and has the following layout:

| Offset | Size | Contents |
| --- | ---: | --- |
| 0 | 1 byte | One-based cursor column relative to the active window. |
| 1 | 1 byte | One-based cursor row relative to the active window. |
| 2 | 1 byte | Current IBM-PC display attribute. |
| 3 | 1 byte | Number of saved rows. |
| 4 | 160 bytes per row | Eighty cells per row, each stored as a character byte followed by an attribute byte. |

The active local window must begin at column 1 and end at column 80. Its
height, up to the 25 rows supported by the legacy local screen, determines how
many cell rows are written after the four-byte header. Unused bytes at the end
of the 4,004-byte application buffer are not part of the saved screen.

This function always reads the local OpenDoors presentation. In a remote
session that presentation may contain only the portion of the virtual session
screen which fits the local console. Text outside that area, and text written
directly to the local display rather than through OpenDoors, make the legacy
snapshot unsuitable as a complete representation of the caller's screen. Use
[`od_save_screen_size()`](od_save_screen_size.md) and
[`od_save_screen_ex()`](od_save_screen_ex.md) when the remote and local screen
dimensions may differ.

The fixed-format buffer is not interchangeable with the row-major rectangle
used by [`od_gettext()`](od_gettext.md), or with the opaque snapshot used by
[`od_save_screen_ex()`](od_save_screen_ex.md). A buffer saved by this function must be restored only
with [`od_restore_screen()`](od_restore_screen.md).

RIP bitmap graphics are not saved. As with other OpenDoors screen-preservation
functions, only the textual cells and their IBM-PC attributes are available.
Saving does not change the visible display, cursor position, or current
attribute.

## Errors

[`ERR_PARAMETER`](../constants/errors.md#err_parameter) is placed in
[`od_control.od_error`](../control/runtime.md#od_error) if `pBuffer` is `NULL`
or the current local window is not exactly 80 columns wide. A successful call
returns [`TRUE`](../constants/general.md#true).

## Example

One common use is to preserve the display while the sysop enters chat mode.
The following callbacks save and clear the local screen before chat, then
restore it afterward:

```c
static unsigned char before_chat_buffer[4004];
static BOOL before_chat_saved = FALSE;

static void before_chat_function(void)
{
   before_chat_saved = od_save_screen(before_chat_buffer);
   od_clr_scr();
}

static void after_chat_function(void)
{
   if(before_chat_saved)
      od_restore_screen(before_chat_buffer);
}

int main(int argc, char *argv[])
{
   od_control.od_cbefore_chat = before_chat_function;
   od_control.od_cafter_chat = after_chat_function;

   od_parse_cmd_line(argc, argv);
   od_init();

   /* Door program continues here. */
   return 0;
}
```

For a remote screen which may not match the local presentation, the callbacks
should instead allocate the size returned by
[`od_save_screen_size()`](od_save_screen_size.md) and use the extended save and
restore functions.

## See also

[`od_restore_screen()`](od_restore_screen.md),
[`od_save_screen_size()`](od_save_screen_size.md),
[`od_save_screen_ex()`](od_save_screen_ex.md),
[`od_gettext()`](od_gettext.md)
