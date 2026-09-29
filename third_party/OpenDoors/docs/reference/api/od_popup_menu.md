# `od_popup_menu()`

Creates a popup menu which allows the user to make a selection by pressing a single key, or selecting the item with a highlight bar. After the user has made a selection, the menu may be removed from the screen, restoring the original screen contents "beneath" the window.

## Synopsis

```c
INT od_popup_menu(char *pszTitle, char *pszText,
    INT nLeft, INT nTop, INT nLevel, WORD uFlags);
```

## Return value

| Value | Meaning |
| --- | --- |
| [`POPUP_ERROR`](../constants/display.md#popup-menu-results) | The menu could not be created or displayed. [`od_control.od_error`](../control/runtime.md#od_error) gives the reason. |
| [`POPUP_ESCAPE`](../constants/display.md#popup-menu-results) | The user cancelled the menu, or a retained menu was destroyed with [`MENU_DESTROY`](../constants/display.md#popup-menu-flags). |
| [`POPUP_LEFT`](../constants/display.md#popup-menu-results) | The user requested the menu to the left. |
| [`POPUP_RIGHT`](../constants/display.md#popup-menu-results) | The user requested the menu to the right. |
| Positive integer | The one-based number of the selected menu item. |

## Description

[`od_popup_menu()`](od_popup_menu.md) creates a popup window with a menu of choices, for use in ANSI/AVATAR/RIP modes. The user is able to choose an item from the menu by moving the highlighted selection bar with the arrow keys, or by pressing a key associated with a particular menu item. The contents of the menu are defined by the string pointed to by the pszText parameter. This menu definition string contains each menu option, separated by a '|' (pipe) character. Keys associated with each menu entry can be defined by proceeding the letter with a '^' (carat) character. For example, the string:

"^Save|^Load|E^xit"

would produce a menu with three options: Save, Load and Exit. The user would be able to select the Save option by pressing the [S] key, the Load option by pressing the [L] key, and the Exit option by pressing the [X] key. Furthermore, the characters corresponding to each menu item would be displayed in a highlighted color.

Menus displayed with [`od_popup_menu()`](od_popup_menu.md) may optionally have a title, as specified by the pszTitle parameter. If this parameter is set to NULL, no title will be displayed. If this parameter is not NULL, the specified string will be displayed as a title on the window.

The `nLeft` and `nTop` parameters specify the left and top coordinates of the
menu window, where 1, 1 is the upper-left corner of the screen. The bottom and
right coordinates are determined from the number and width of the menu items.
The complete menu, including its border, must fit within columns 1 through 80
and rows 1 through 25.

The `nLevel` parameter specifies the menu level and must be an integer from 0
through 10. Each level can retain one active menu. Unless you are using
[`MENU_KEEP`](../constants/display.md#menu_keep), this parameter can always be zero.

The uFlags parameter specifies one or more of the following options, joined by the bitwise-OR operator (|).

| Flag | Effect |
| --- | --- |
| [`MENU_NORMAL`](../constants/display.md#popup-menu-flags) | No additional behavior. |
| [`MENU_ALLOW_CANCEL`](../constants/display.md#popup-menu-flags) | Allow the user to exit with Escape. |
| [`MENU_PULLDOWN`](../constants/display.md#popup-menu-flags) | Allow the user to exit with the left or right arrow key. |
| [`MENU_KEEP`](../constants/display.md#popup-menu-flags) | Leave the menu active after a menu item is selected. |
| [`MENU_DESTROY`](../constants/display.md#popup-menu-flags) | Remove a menu previously retained at this level. |

If you are not using any of the other flags, you can use MENU_NORMAL as a place-holder for this parameter. If you specify MENU_ALLOW_CANCEL, the user will be able to exit the menu without making a selection by pressing the [ESCape] key. If the user presses [ESCape], [`od_popup_menu()`](od_popup_menu.md) returns POPUP_ESCAPE.

You can use the MENU_PULLDOWN option with [`od_popup_menu()`](od_popup_menu.md) to implement a set of pulldown menus. In this case, if the user presses the left arrow key or right arrow key while the menu is being displayed, [`od_popup_menu()`](od_popup_menu.md) returns with POPUP_LEFT or POPUP_RIGHT, allowing you to display a different menu.

Normally, [`od_popup_menu()`](od_popup_menu.md) will remove the menu from the screen as soon as the user makes a selection. However, there may be some cases when you want the menu to continue to be visible after the user makes a selection. For example, you may want some menu options to lead to further sub-menus, or you may wish to display a popup window, and return to this menu after the user has exited from the popup window. If the MENU_KEEP flag is specified, the menu will remain active (on-screen) after the user makes a selection. However, the menu will still be destroyed if the user cancels out of the menu (this will only happen if you have specified MENU_ALLOW_CANCEL), or if the user moves to another menu by pressing the left or right arrow keys (this will only happen if you have specified MENU_PULLDOWN). If MENU_KEEP has been specified, and the user makes a selection, you must eventually either return to the menu, or destroy it by calling MENU_DESTROY. If you want to return to the menu, simply call [`od_popup_menu()`](od_popup_menu.md) again with the same level value that was used to originally create the menu. The user will now be able to make another selection from the menu, and [`od_popup_menu()`](od_popup_menu.md) will once again return that selection to you. If you want to destroy the menu, simply call [`od_popup_menu()`](od_popup_menu.md) with the MENU_DESTROY flag set, and the same level value that was used to create the original menu. If you wish to create another popup menu while the first one is still active, simply call [`od_popup_menu()`](od_popup_menu.md) again, this time with a different level value. The colors used by the [`od_popup_menu()`](od_popup_menu.md) function are set by the following OpenDoors control structure settings:

When a retained menu is entered again, OpenDoors uses the title, text,
coordinates, flags and highlighted item saved when that menu was created. With
the exception of [`MENU_DESTROY`](../constants/display.md#menu_destroy), newly supplied values for those parameters do
not replace the saved menu. [`MENU_DESTROY`](../constants/display.md#menu_destroy) is therefore meaningful only when a
menu is active at the specified level. It restores the saved screen area,
releases the retained menu, and returns [`POPUP_ESCAPE`](../constants/display.md#popup_escape).

The menu may contain up to 21 items. Each item retains up to 76 displayed
characters; additional characters are ignored. A pipe character (`|`) ends an
item. The final pipe may be omitted. A caret (`^`) selects the following
display position as that item's hotkey and is not itself displayed. If no caret
is supplied, the first character is the hotkey. Hotkey comparisons are not
case sensitive. The user may also move with the up and down arrow keys, or with
the `8` and `2` keys, and may select the highlighted item with Enter.

The colors used by [`od_popup_menu()`](od_popup_menu.md) are set by the following
[`od_control`](../control/index.md) fields:

[`od_menu_title_col`](../control/customization.md#od_menu_title_col),
[`od_menu_border_col`](../control/customization.md#od_menu_border_col),
[`od_menu_text_col`](../control/customization.md#od_menu_text_col),
[`od_menu_key_col`](../control/customization.md#od_menu_key_col),
[`od_menu_highlight_col`](../control/customization.md#od_menu_highlight_col),
and [`od_menu_highkey_col`](../control/customization.md#od_menu_highkey_col).

ANSI or AVATAR support is required. A null or empty menu definition, a menu
which does not fit on the 80-by-25 display, or invalid coordinates produce
[`POPUP_ERROR`](../constants/display.md#popup_error) with
[`ERR_PARAMETER`](../constants/errors.md#err_parameter). A level outside the
range 0 through 10 produces [`ERR_LIMIT`](../constants/errors.md#err_limit),
and the absence of ANSI and AVATAR support produces
[`ERR_NOGRAPHICS`](../constants/errors.md#err_nographics). If OpenDoors cannot
allocate either the menu-item array or the storage used to save the underlying
screen, it reports [`ERR_MEMORY`](../constants/errors.md#err_memory). Saving
the underlying screen may also fail for a screen-related reason. Consult
[`od_control.od_error`](../control/runtime.md#od_error) after [`POPUP_ERROR`](../constants/display.md#popup_error) for
the reported reason.

## Examples

The following example shows the use of multiple-level menus:

```c
#include <stdlib.h>
#include "opendoor.h"
main()
{
   for(;;)
   {
      switch(od_popup_menu("Main Menu",
         "^Files|^Electronic Mail|^News|E^xit",
         20, 5, 0, MENU_NORMAL | MENU_KEEP))
      {
         case 1:
            od_popup_menu("Files Menu",
               "^Search For Files|^Download|^Upload",
               30, 7, 2, MENU_NORMAL | MENU_ALLOW_CANCEL);
            break;
         case 2:
            od_popup_menu("EMail Menu",
               "Get ^New Mail|^Send Mail|Send ^Fax",
               30, 8, 1, MENU_NORMAL | MENU_ALLOW_CANCEL);
            break;
         case 3:
            od_popup_menu("News Menu",
               "Choose News^Group|^Read News|^Post News",
               30, 9, 1, MENU_NORMAL | MENU_ALLOW_CANCEL);
            break;
         case 4:
            od_popup_menu(NULL, NULL, 0, 0, 0, MENU_DESTROY);
            return(0);
      }
   }
}
```

## See also

[`od_window_create()`](od_window_create.md),
[`od_hotkey_menu()`](od_hotkey_menu.md), [Menus and
screen](../constants/display.md)
