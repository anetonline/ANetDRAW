# `od_chat()`

Manually invokes sysop chat mode.

## Synopsis

```c
void od_chat(void);
```

## Return value

N/A

## Description

Normally, the OpenDoors sysop chat mode will only be invoked when the sysop explicitly requests it using the sysop chat key. However, there may be some cases where you wish to manually invoke the sysop chat mode. One example is when you are replacing the OpenDoors built-in chat mode with your own, but still wish to use the OpenDoors chat mode under some circumstances. For instance, you may wish to use your own split- screen chat routine if ANSI, AVATAR or RIP graphics mode is available, and use the OpenDoors line-oriented chat mode if only ASCII mode is available.

The function enters the same line-oriented chat mode used by the configured
sysop chat key. Both local and remote input are displayed to both participants.
Text entered by the sysop uses
[`od_control.od_chat_color1`](../control/customization.md#od_chat_color1), while
text entered by the caller uses
[`od_control.od_chat_color2`](../control/customization.md#od_chat_color2).
OpenDoors performs simple word wrapping near column 76 and recognizes
backspace and Enter. The sysop may end chat by pressing Escape; an Escape from
the remote caller does not end chat.

On entry, OpenDoors clears
[`od_control.user_wantchat`](../control/caller.md#user_wantchat), forces a
status update, and sets
[`od_control.od_chat_active`](../control/runtime.md#od_chat_active) to [`TRUE`](../constants/general.md#true).
It then invokes
[`od_control.od_cbefore_chat`](../control/customization.md#od_cbefore_chat), if
one has been installed. The callback may cancel chat by setting
[`od_chat_active`](../control/runtime.md#od_chat_active) to [`FALSE`](../constants/general.md#false). Otherwise, OpenDoors displays
[`od_control.od_before_chat`](../control/customization.md#od_before_chat) and
records the chat-start log message when logging is active.

Chat continues until the sysop presses Escape or application code sets
[`od_chat_active`](../control/runtime.md#od_chat_active) to [`FALSE`](../constants/general.md#false). During cleanup OpenDoors displays
[`od_control.od_after_chat`](../control/customization.md#od_after_chat), invokes
[`od_control.od_cafter_chat`](../control/customization.md#od_cafter_chat),
records the chat-end log message, restores the display attribute which was
active on entry, and clears [`od_chat_active`](../control/runtime.md#od_chat_active).

Multithreaded builds run the same chat processing in a dedicated thread and
wait for that thread before returning. If the thread cannot be started,
[`od_chat()`](od_chat.md) returns immediately, clears
[`od_control.od_chat_active`](../control/runtime.md#od_chat_active), and sets
[`od_control.od_error`](../control/runtime.md#od_error) to
[`ERR_GENERALFAILURE`](../constants/errors.md#err_generalfailure).

## Examples

For a complete example of a door which provides both split-screen and
line-oriented chat, see `ex_chat.c` in the source distribution. A door may
invoke the built-in mode directly with:

```c
if(!od_control.user_ansi && !od_control.user_avatar)
   od_chat();
```

## See also

[`od_page()`](od_page.md), [The `od_control` structure](../control/index.md)
