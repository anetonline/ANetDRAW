# `od_printf()`

Performs formatted output (remote & local), with the ability to change display colors.

## Synopsis

```c
void od_printf(const char *pszFormat, ...);
```

## Return value

N/A

## Description

This is one of the OpenDoors functions which allows you to display a string of
characters, another being the [`od_disp_str()`](od_disp_str.md) function. For a
complete comparison of the various OpenDoors display functions, see the
description of [`od_disp_str()`](od_disp_str.md). Like that function,
[`od_printf()`](od_printf.md) displays its output on both the local screen and the remote
user's screen, unless the door is operating in local mode. However,
[`od_printf()`](od_printf.md) also provides formatted output in the same manner as the C
`printf()` function. In addition to the conversions supported by the C runtime,
[`od_printf()`](od_printf.md) allows color changes to be included in the resulting text. This
feature permits multi-colored text to be displayed without chains of
alternating [`od_disp_str()`](od_disp_str.md) and
[`od_set_color()`](od_set_color.md) calls.

As with `printf()`, the [`od_printf()`](od_printf.md) function accepts one or more parameters.
The first parameter is the format string to be displayed, and the remaining
parameters supply the values selected by that string. The formatting
conversions are those provided by the C runtime with which OpenDoors was built.
For example, to display the amount of time recorded in
[`od_control.user_timelimit`](../control/caller.md#user_timelimit), the
following is a valid use of [`od_printf()`](od_printf.md):

```text
od_printf("Time Left:%d\n\r", od_control.user_timelimit);
```

A full discussion of the C `printf()` format language is beyond the scope of
this reference. Consult the documentation for the C compiler and runtime used
to build the door for the conversions and modifiers it supports.

In addition to the normal control sequences, such as "%s", "%d", or "%12.12s", the [`od_printf()`](od_printf.md) function also allows you to include special color-setting codes within the format string. These color code sequences BEGIN and END with a delimiter character, which is used to indicate that the sequence is a color setting. Consider, for example, the following line of code, which displays text in various colors:

```text
od_printf("`blue`Blue `green`Green `red`Red  \n\r");
```

In this case (assuming of course that a color monitor is being used) the word "Blue" will be displayed in the color blue, the word "Green" will be displayed in the color green, and the word "Red" will be displayed in the color red. In this case, the sequence `blue` sets the display color to dark blue on black. Here, the back-quote (`) is the delimiter character which indicates the beginning and end of the color sequence. Be sure not to confuse the back-quote character (`) with the normal forward quote ('). THIS IS THE MOST COMMON DIFFICULTY EXPERIENCED WITH THE `OD_PRINTF()` FUNCTION. The text between the back-quote characters indicates the color that should be set. This text can include the name of the foreground color, the name of the background color, the "bright" keyword and the "flashing" keyword. The first color mentioned is taken to be the foreground color, and the second the background color. Case is not sensitive, additional words can be included for legibility. Thus:

```text
`bright white cyan`
```

is equivalent to:

```text
`Bright white on a cyan background`.
```

The "bright" keyword indicates that the foreground color should be displayed in high intensity, and the "flashing" keyword indicates that the text should be flashing. If no background is specified, the background color defaults to black. If no foreground or background colors are specified, the color defaults to white on black.

The [`od_printf()`](od_printf.md) function will automatically determine whether the user has ANSI, AVATAR or RIP graphics enabled, and will send the appropriate color codes to change the color of displayed text. If the user does not have either ANSI or AVATAR graphics modes turned on, then the [`od_printf()`](od_printf.md) function will not send any color codes. Thus, a door program using color codes would work just as well when ANSI/AVATAR/RIP graphics are not available, except that all text will appear in the same color.

You may prefer to set colors by using the [`od_set_color()`](od_set_color.md) or [`od_set_attrib()`](od_set_attrib.md) functions, instead of using these cryptic color codes imbedded in [`od_printf()`](od_printf.md) functions. In some cases, however, it will be much more advantageous to place the color codes within your [`od_printf()`](od_printf.md) strings. As a case in point, consider the single [`od_printf()`](od_printf.md) statement in the example, above. To accomplish the same result using the [`od_disp_str()`](od_disp_str.md) and [`od_set_color()`](od_set_color.md) functions, you would have to use the following SIX function calls:

```text
od_set_color(D_BLUE,D_BLACK);
od_disp_str("Blue ");
od_set_color(D_GREEN,D_BLACK);
od_disp_str("Green ");
od_set_color(D_RED,D_BLACK);
od_disp_str("Red  \n\r");
```

While this method MAY be easier understand, it certainly requires many more line of code to accomplish. However, either method will work, and the choice is up to you as to which method you prefer. Keep in mind, however, that if the color to be set is stored in a variable, instead of always being the same color, you must use either the [`od_set_color()`](od_set_color.md) or [`od_set_attrib()`](od_set_attrib.md) function to set the display color.

While the back-quote (`) character is normally used to delimit a color sequence
in [`od_printf()`](od_printf.md), you may wish to print that character as ordinary text. In
this case, OpenDoors can be configured to use a different delimiter through
[`od_control.od_color_delimiter`](../control/customization.md#od_color_delimiter).
For example, to use the tilde (`~`) character instead, place the following line
in your program after calling [`od_init()`](od_init.md), or after calling
another OpenDoors function which performs initialization:

```text
od_control.od_color_delimiter='~';
```

To disable delimiter-based color descriptions, set
[`od_control.od_color_delimiter`](../control/customization.md#od_color_delimiter)
to zero. OpenDoors also supports the separate single-byte color marker selected
by [`od_control.od_color_char`](../control/customization.md#od_color_char). To
disable all color interpretation, both fields must be zero. Since
[`od_color_char`](../control/customization.md#od_color_char) defaults to zero, clearing [`od_color_delimiter`](../control/customization.md#od_color_delimiter) is sufficient
when the other default has not been changed.

The [`od_printf()`](od_printf.md) function interprets color codes after processing the C format
conversions, such as `%d` or `%s`. Thus, if you use the command:

```text
od_printf("%s",string);
```

any color codes contained in `string` are also interpreted. If expanded text
must be displayed literally, temporarily set both
[`od_control.od_color_delimiter`](../control/customization.md#od_color_delimiter)
and [`od_control.od_color_char`](../control/customization.md#od_color_char) to
zero, or display the completed text with
[`od_disp_str()`](od_disp_str.md).

There is no longer a 511-character limit on the formatted result. OpenDoors
first determines the required size and enlarges an internal buffer as needed.
If the required size cannot be represented, the function sets
[`od_control.od_error`](../control/runtime.md#od_error) to
[`ERR_LIMIT`](../constants/errors.md#err_limit). If the buffer cannot be
allocated, it sets [`od_error`](../control/runtime.md#od_error) to
[`ERR_MEMORY`](../constants/errors.md#err_memory). If the second formatting
operation does not produce the size reported by the first, it sets [`od_error`](../control/runtime.md#od_error)
to [`ERR_GENERALFAILURE`](../constants/errors.md#err_generalfailure). In all
three cases, no formatted text is displayed.

When a delimiter is encountered, text preceding it is displayed and the text
through the matching delimiter is interpreted as a color description. The
delimiter and description are not displayed. If the string ends before a
closing delimiter is found, the remaining color description is consumed and
no further text is displayed. When [`od_color_char`](../control/customization.md#od_color_char) is nonzero, that marker and
the byte which immediately follows it are likewise removed from the output;
the following byte is passed directly to
[`od_set_attrib()`](od_set_attrib.md). A marker at the end of the formatted
string simply ends the output.

## Examples

Below is a simple example of a user statistics door program, which displays various pieces of information to the user, by using the [`od_printf()`](od_printf.md) function. Notice the use of color code sequences in order to display the titles in a different color from the information fields. Note that since the information available to this door will depend on the BBS system under which it is running, not all of the information displayed by this door will be available under all BBS systems. For a description of what information is available under what BBS systems, see the OpenDoors control structure portion of this manual.

```c
#include "OpenDoor.h"

int main(int argc, char *argv[])
{
   od_init();

   od_printf("`bright white` YOUR STATISTICS\n\r");
   od_printf("---------------\n\r\n\r");

   od_printf("`red`NAME :             `blue`%s\n\r",
      od_control.user_name);
   od_printf("`red`LOCATION :         `blue`%s\n\r",
      od_control.user_location);
   od_printf("`red`PHONE NUMBER :     `blue`%s\n\r",
      od_control.user_homephone);
   od_printf("`red`LAST CALL :        `blue`%s\n\r",
      od_control.user_lastdate);
   od_printf("`red`NUMBER OF CALLS :  `blue`%u\n\r",
      od_control.user_numcalls);
   od_printf("`red`NUMBER OF PAGES :  `blue`%u\n\r",
      od_control.user_numpages);
   od_printf("`red`REMAINING TIME :   `blue`%d\n\r",
      od_control.user_timelimit);
   od_printf("`red`# OF DOWNLOADS :   `blue`%u\n\r",
      od_control.user_downloads);
   od_printf("`red`# OF UPLOADS :     `blue`%u\n\r",
      od_control.user_uploads);
   od_printf("`red`KBYTES DL TODAY :  `blue`%u\n\r",
      od_control.user_todayk);

   od_printf("`bright green on green`"
      "Press [Enter] to return to BBS...\n\r");
   while(od_get_key(TRUE) != 13)
      ;

   od_exit(20, FALSE);
}
```

## Helpful hint

The preceding example demonstrates the use of [`od_printf()`](od_printf.md) color sequences
embedded directly in the format string, such as:

```c
od_printf("Hello `bright green` there!");
```

However, there are also other ways that you can take advantage of this feature. For example, the C programming language concatenates string constants that are separated only by white space or carriage returns. For instance,

```c
"Hello " "`bright green`" " there!"
```

is equivalent to:

```c
"Hello `bright green` there!"
```

For this reason, you can create macros for common color sequences in your program, such as:

```c
#define HIGHLIGHT "`bright green`"
```

You can then use such constants when calling the [`od_printf()`](od_printf.md) function, as follows:

```c
od_printf("Hello " HIGHLIGHT " there!");
```

You may find this method of setting the display color to be easier to read, and more easily configurable than including the color sequence directly in the format string. Below another use of the color sequences is describe, which allows the colors used by [`od_printf()`](od_printf.md) not be "hard-wired".

Since color control sequences are evaluated by [`od_printf()`](od_printf.md) after it evaluates all format sequences (such as "%d"). For this reason, it is possible to change the display color using a string variable, instead of using a fixed color in the string. For example, if you program had the variable:

```c
char highlight[40];
```

which was set at some point to be equal to:

```c
"`bright green`"
```

you would be able to use [`od_printf()`](od_printf.md) as follows:

```c
od_printf("Hello %s there!", highlight);
```

The display color would then be changed at the location where the `%s` appears
in the [`od_printf()`](od_printf.md) format string. The advantage of this method is that the
value of `highlight` can be changed. It could be loaded from the door's
configuration, for example, allowing the sysop to select the colors used by
the door without requiring the format string to be changed.

## See also

[`od_disp_str()`](od_disp_str.md), [`od_set_color()`](od_set_color.md),
[`od_set_attrib()`](od_set_attrib.md), [`od_color_config()`](od_color_config.md),
[`od_putch()`](od_putch.md)
