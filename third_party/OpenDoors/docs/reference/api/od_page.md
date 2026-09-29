# `od_page()`

Function to allow user to page the sysop

## Synopsis

```c
void od_page(void);
```

## Return value

N/A

## Description

This function can be called to allow the user to page the sysop. This function will ask the user why they wish to chat with the sysop, and then page the sysop. The sysop will then be free to break into chat at any time. Sysop paging will also be aborted by the user, simply by pressing [Enter] when asked for a reason for chat. When the user pages the sysop, the [Wants-Chat] indicator will begin to flash on the main status line, and the status line will switch to show the user's reason for wanting to chat. Also, the user's total number of pages will be incremented.

The screen is first cleared and the prompt in
[`od_control.od_chat_reason`](../control/customization.md#od_chat_reason) asks
the caller to supply a reason. The response is stored in
[`od_control.user_reasonforchat`](../control/caller.md#user_reasonforchat). A
blank response cancels the operation before the availability test, page count,
log entry, and paging signal are produced.

For a nonblank response, OpenDoors sets
[`od_control.user_wantchat`](../control/caller.md#user_wantchat) to [`TRUE`](../constants/general.md#true) and
then applies the policy selected by
[`od_control.od_okaytopage`](../control/runtime.md#od_okaytopage):

- [`PAGE_DISABLE`](../constants/session.md#page_disable) always refuses
  the page.
- [`PAGE_ENABLE`](../constants/session.md#page_enable) always permits it.
- [`PAGE_USE_HOURS`](../constants/session.md#page_use_hours) permits the
  page only during the interval selected by
  [`od_control.od_pagestartmin`](../control/runtime.md#od_pagestartmin) and
  [`od_control.od_pageendmin`](../control/runtime.md#od_pageendmin).

The hour values are minutes after midnight. When the start is less than the
end, the permitted interval begins at the start and ends immediately before
the end. When the start is greater than the end, the interval crosses
midnight. Equal values permit paging throughout the day. The normal defaults
are 8:00 a.m. through 10:00 p.m. If paging is refused, OpenDoors displays
[`od_control.od_no_sysop`](../control/customization.md#od_no_sysop) and waits
for Enter.

When the page is permitted, OpenDoors updates the local status display, writes
the standard page entry to the activity log, displays
[`od_control.od_paging`](../control/customization.md#od_paging), and increments
[`od_control.user_numpages`](../control/caller.md#user_numpages). If the active
personality defines a paging status line through
[`od_control.od_page_statusline`](../control/runtime.md#od_page_statusline),
that line is selected.

The page lasts for
[`od_control.od_page_len`](../control/customization.md#od_page_len) seconds,
with the standard door-information initialization defaulting this value to 15.
Each second OpenDoors displays another period, rings the bell on the local and
remote displays, and runs the kernel so that the sysop can enter chat
immediately. If chat begins, paging ends. Otherwise, after the configured
period OpenDoors displays
[`od_control.od_no_response`](../control/customization.md#od_no_response) and
waits for Enter. The display attribute which was active before the call is
restored before the function returns.

The function has no return value indicating whether the caller cancelled, the
page was refused, the sysop answered, or the page expired. Applications which
need a different result or presentation should implement their own page logic
using the documented state and callback interfaces.

## Examples

For an example of the use of [`od_page()`](od_page.md), see the `ex_vote.c` example program
included with the source distribution.

## See also

[`od_chat()`](od_chat.md), [The `od_control` structure](../control/index.md)
