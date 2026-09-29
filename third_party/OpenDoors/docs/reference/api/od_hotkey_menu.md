# `od_hotkey_menu()`

Function to display a menu file with hotkeys

## Synopsis

```c
char od_hotkey_menu(char *pszFileName, char *pszHotKeys, BOOL bWait);
```

## Return value

Key pressed in response to menu, or '\0' if none.

## Description

This function can be used to display a menu from an ASCII, ANSI, AVATAR or RIP
file, allowing the user to select an option at any time while the menu is being
displayed. The [`od_hotkey_menu()`](od_hotkey_menu.md) function is quite similar to
[`od_send_file()`](od_send_file.md), and you should familiarize yourself with
that function before using [`od_hotkey_menu()`](od_hotkey_menu.md). Like [`od_send_file()`](od_send_file.md),
[`od_hotkey_menu()`](od_hotkey_menu.md) displays the file specified by `pszFileName` using the
appropriate terminal emulation. If no extension is provided, OpenDoors searches
for compatible `.ASC`, `.ANS`, `.AVT` and `.RIP` files, selecting among the
available files according to the remote display mode and local display
capabilities.

The second parameter, `pszHotKeys`, is a string specifying the valid responses
to the menu, in the same format as the string passed to
[`od_get_answer()`](od_get_answer.md). If either the uppercase or lowercase
form of a listed character is pressed, OpenDoors immediately stops displaying
the menu and returns the corresponding character from `pszHotKeys`. The case
of the returned character is therefore the case used in the hotkey string, not
necessarily the case typed by the user. The Enter key may be included by
placing the carriage-return character (`\r` or `\x0d`) in `pszHotKeys`.

The third parameter, `bWait`, specifies whether OpenDoors should wait after
displaying the menu for the user to make a valid selection. A value of [`TRUE`](../constants/general.md#true)
causes the function to wait; this is normally the desired behavior because one
call both displays the menu and obtains the user's selection. A value of
[`FALSE`](../constants/general.md#false) causes the function to return as soon as the file has been displayed.
If no valid hotkey was pressed while the file was being sent, the return value
is `\0`.

`pszHotKeys` must not be `NULL`. If it is, the function returns `\0` and sets
[`od_control.od_error`](../control/runtime.md#od_error) to
[`ERR_PARAMETER`](../constants/errors.md#err_parameter). Errors opening or
displaying the menu file are reported in the same manner as
[`od_send_file()`](od_send_file.md), and also cause `\0` to be returned.

## Examples

As an example of the use of the [`od_hotkey_menu()`](od_hotkey_menu.md) function, consider the following code fragment:

```c
for(;;)                             /* Main program loop */
{
   char choice;

   /* Display menu and get user's choice. */
   choice = od_hotkey_menu("MAINMENU", "123Q", TRUE);

   switch(choice)                   /* Perform the selected action. */
   {
      case '1':
         od_printf("You selected one.\n\r");
         break;

      case '2':
         od_printf("You selected two.\n\r");
         break;

      case '3':
         od_printf("You selected three.\n\r");
         break;

      case 'Q':
         od_exit(10, FALSE);
   }
}
```

This is the main menu loop of a simple door. The program continues executing
the `for` loop until the user chooses to exit. On each iteration,
[`od_hotkey_menu()`](od_hotkey_menu.md) displays the door's menu from the compatible `MAINMENU`
display file and accepts one of the choices in `"123Q"`. If the user makes a
selection while the menu is still being displayed, file display stops at that
point. The `switch` statement then responds to the selected key. Keys `1`, `2`
and `3` display simple messages, while `Q` returns control to the BBS.

## See also

[`od_send_file()`](od_send_file.md), [`od_get_answer()`](od_get_answer.md),
[`od_popup_menu()`](od_popup_menu.md)
