# Menus and screen

These constants control popup-menu lifetime and navigation, screen scrolling,
the local status display, box drawing, and terminal capability detection. The
zero-valued `*_NORMAL` definitions select default behavior and add no bits when
combined with other flags.

## Popup menu flags

Combine applicable flags with bitwise OR and pass them to
[`od_popup_menu()`](../api/od_popup_menu.md). The lifetime flags depend on the
menu level supplied to that call; the behavior flags affect the current input
operation.

### `MENU_NORMAL`

`MENU_NORMAL` has value zero. The menu waits for an ordinary valid selection,
does not return special left/right navigation, and removes its window after the
selection. Escape is not an accepted cancellation unless
`MENU_ALLOW_CANCEL` is also present.

Because the value is zero, `MENU_NORMAL | MENU_KEEP` is simply `MENU_KEEP`.

### `MENU_ALLOW_CANCEL`

Allow the user to cancel the active menu with Escape. The function returns
[`POPUP_ESCAPE`](#popup_escape), and no ordinary item has been selected. The
application decides whether cancellation returns to a previous screen, closes
a hierarchy, or is ignored by calling the menu again.

Without this flag, Escape does not provide the documented cancellation result.
Do not assume that a zero return always means the first item: popup selections
are positive and zero is reserved for cancellation.

### `MENU_PULLDOWN`

Enable pulldown-style horizontal navigation. Left and Right may close the
current menu and return [`POPUP_LEFT`](#popup_left) or
[`POPUP_RIGHT`](#popup_right). The application uses that result to select the
adjacent menu heading and opens its menu at the appropriate level.

This flag does not construct a menu bar or choose the adjacent menu itself. It
changes the returned input vocabulary so the application can coordinate a set
of popup menus.

### `MENU_KEEP`

Keep the popup window and its saved background after an ordinary selection.
The retained menu is associated with its level and can be reused or explicitly
destroyed. Keeping a menu avoids unnecessary restoration and recreation while
moving through a hierarchy, but it also means that its screen resources remain
owned by the popup system.

Every retained menu must eventually be resumed or destroyed at the correct
level. OpenDoors retains its allocated parsed-item array and saved screen
buffer; it does not retain pointers to the caller's title or menu-description
strings after the initial call returns.

### `MENU_DESTROY`

Destroy the retained menu at the supplied level instead of accepting input.
Its saved background is restored and its resources are released. Use this only
for a menu previously kept at that level.

`MENU_DESTROY` describes an operation, not a post-selection preference. It
should not be combined with `MENU_KEEP`, and selection/navigation flags have no
useful meaning while destroying a menu.

## Popup menu results

An ordinary successful choice is returned as a positive selection number.
These nonpositive values describe failure, cancellation, or pulldown
navigation and must be checked before indexing an item array.

### `POPUP_ERROR`

`POPUP_ERROR` is `-1`. The menu could not be created, displayed, resumed, or
used. Inspect [`od_control.od_error`](../control/runtime.md#od_error)
immediately for the reason.

### `POPUP_ESCAPE`

`POPUP_ESCAPE` is zero. It reports either cancellation accepted because
`MENU_ALLOW_CANCEL` was active or completion of a `MENU_DESTROY` operation on
a retained menu. In either case it is not an ordinary item number and does not,
by itself, indicate an API error; [`od_error`](../control/runtime.md#od_error) need not change.

### `POPUP_LEFT`

`POPUP_LEFT` is `-2` and reports leftward navigation from a menu opened with
`MENU_PULLDOWN`. It does not mean that item -2 was selected.

### `POPUP_RIGHT`

`POPUP_RIGHT` is `-3` and reports rightward navigation from a menu opened with
`MENU_PULLDOWN`.

## Scrolling flags

Pass one of these settings to [`od_scroll()`](../api/od_scroll.md). Direction
and distance come from that function's other parameters.

### `SCROLL_NORMAL`

Scroll the selected rectangle and clear each newly exposed cell using the
current display attribute. This produces the behavior normally expected when
moving a text region up or down.

### `SCROLL_NO_CLEAR`

Permit [`od_scroll()`](../api/od_scroll.md) to leave the newly exposed rows
uncleared when clearing them would take longer. The flag is an optimization,
not a promise that those rows are preserved: an AVATAR scrolling operation or
another implementation path may inherently clear them. Code using this flag
must not depend on either blank or retained contents and should overwrite the
exposed rows itself.

## Local status-line selections

Pass these values to
[`od_set_statusline()`](../api/od_set_statusline.md). They select a local
operator display implemented by the active personality. They do not send a
status line to the remote caller and do not change the remote screen size.

### `STATUS_NORMAL`

Select display slot 1, the personality's normal status display. The personality
receives `PEROP_DISPLAY1` when a full redraw is required and
`PEROP_UPDATE1` for its dynamic update.

### `STATUS_ALTERNATE_1`

Select display slot 2, corresponding to `PEROP_DISPLAY2` and
`PEROP_UPDATE2`.

### `STATUS_ALTERNATE_2`

Select display slot 3, corresponding to `PEROP_DISPLAY3` and
`PEROP_UPDATE3`.

### `STATUS_ALTERNATE_3`

Select display slot 4, corresponding to `PEROP_DISPLAY4` and
`PEROP_UPDATE4`.

### `STATUS_ALTERNATE_4`

Select display slot 5, corresponding to `PEROP_DISPLAY5` and
`PEROP_UPDATE5`.

### `STATUS_ALTERNATE_5`

Select display slot 6, corresponding to `PEROP_DISPLAY6` and
`PEROP_UPDATE6`.

### `STATUS_ALTERNATE_6`

Select display slot 7, corresponding to `PEROP_DISPLAY7` and
`PEROP_UPDATE7`.

### `STATUS_ALTERNATE_7`

Select display slot 8, corresponding to `PEROP_DISPLAY8` and
`PEROP_UPDATE8`.

### `STATUS_NONE`

Hide the currently selected local status display so the local screen can use
the full available area. The status subsystem remains enabled and a later
selection may restore a personality display. To disable status operation as a
policy, use
[`od_control.od_status_on`](../control/customization.md#od_status_on).

The compatibility names [`STATUS_USER1`](../compatibility.md#status-and-color-aliases) through [`STATUS_USER4`](../compatibility.md#status-and-color-aliases),
[`STATUS_SYSTEM`](../compatibility.md#status-and-color-aliases), and [`STATUS_HELP`](../compatibility.md#status-and-color-aliases) map to established alternate slots; see
[Compatibility interfaces](../compatibility.md#status-and-color-aliases).

## Box-character indexes

[`od_control.od_box_chars`](../control/customization.md#od_box_chars) contains
eight bytes used by [`od_draw_box()`](../api/od_draw_box.md). These constants
are indexes into that array, not character codes. Changing an element changes
the corresponding part of subsequently drawn boxes.

### `BOX_UPPERLEFT`

Index 0, used for the upper-left corner.

### `BOX_TOP`

Index 1, repeated across the top horizontal edge between its corners.

### `BOX_UPPERRIGHT`

Index 2, used for the upper-right corner.

### `BOX_LEFT`

Index 3, repeated down the left vertical edge.

### `BOX_LOWERLEFT`

Index 4, used for the lower-left corner.

### `BOX_LOWERRIGHT`

Index 5, used for the lower-right corner.

### `BOX_BOTTOM`

Index 6, repeated across the bottom horizontal edge.

### `BOX_RIGHT`

Index 7, repeated down the right vertical edge.

The default bytes are CP437 single-line box characters. If output conversion
is active they pass through the normal terminal encoding path. A replacement
array should contain bytes meaningful to that path and to the caller's
terminal.

## Terminal detection flag

### `DETECT_NORMAL`

`DETECT_NORMAL` is the only defined flag value for
[`od_autodetect()`](../api/od_autodetect.md) and has value zero. The current
implementation explicitly ignores the `nFlags` parameter, so this constant
does not select a separate code path. No optional detection bits are defined;
applications should pass `DETECT_NORMAL` rather than relying on the current
implementation also ignoring unknown nonzero bits.

Detection writes probe sequences to the remote connection and waits for
responses. It is not appropriate in local mode, after application input is
already pending, or when a launcher has supplied reliable terminal capability
information which should not be disturbed.
