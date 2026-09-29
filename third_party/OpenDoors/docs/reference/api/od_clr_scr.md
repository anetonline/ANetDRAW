# `od_clr_scr()`

Clears the current OpenDoors screen

## Synopsis

```c
void od_clr_scr(void);
```

## Return value

N/A

## Description

[`od_clr_scr()`](od_clr_scr.md) clears the active output window on the caller's terminal and in
OpenDoors' screen model, then positions the cursor at its upper-left corner.
The status line and any local display area outside the output window are not
cleared. Every cleared cell receives the current display attribute.

For extended `EXITINFO.BBS` records and custom door-information handlers,
OpenDoors honors bit `0x02` of
[`od_control.user_attribute`](../control/caller.md#user_attribute). If that bit
is clear, screen clearing is suppressed unless
[`od_control.od_always_clear`](../control/customization.md#od_always_clear) is
[`TRUE`](../constants/general.md#true). Traditional door-information formats do
not supply a dependable screen-clearing preference and are cleared normally.
This preference is independent of ANSI, AVATAR, or RIP availability.

When clearing is permitted, OpenDoors sends the appropriate terminal output:
RIP clear and window commands where applicable, an ANSI erase-display and
home sequence for ANSI callers, or form feed for plain-ASCII callers. It then
clears the complete virtual session screen and refreshes the portion visible
on the local presentation. In local mode the active local text window is
cleared directly.

After the clear, OpenDoors reasserts the current color in a graphics mode so
that its local, virtual, and remote attribute state remains synchronized. The
function does not select a new color.

If an application has independently determined that the terminal can clear
its screen and deliberately wishes to bypass the stored caller preference, it
may emulate a form-feed character directly:

```c
od_disp_emu("\x0c", TRUE);
```

Normally, setting [`od_always_clear`](../control/customization.md#od_always_clear) before initialization and continuing to
use [`od_clr_scr()`](od_clr_scr.md) is clearer and keeps the policy in one place.

The function reports no status and returns no value.

## Example

The following routine can be used when a BBS does not provide the caller's
screen-clearing preference. Its answer may be stored in the door's own user
record and applied on later calls:

```c
BOOL user_supports_screen_clearing(void)
{
   char answer;

   od_disp_str("OpenDoors will now test screen clearing.\n\r");
   od_disp_str("Press [Enter] when ready.\n\r");
   while(od_get_key(TRUE) != '\r')
      ;

   od_clr_scr();
   od_disp_str("Did your screen clear? (Y/N) ");

   for(;;)
   {
      answer = od_get_key(TRUE);
      if(answer == 'Y' || answer == 'y')
         return TRUE;
      if(answer == 'N' || answer == 'n')
         return FALSE;
   }
}
```

## See also

[`od_clr_line()`](od_clr_line.md), [`od_scroll()`](od_scroll.md),
[`od_set_attrib()`](od_set_attrib.md)
