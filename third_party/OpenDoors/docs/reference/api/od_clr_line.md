# `od_clr_line()`

Clears the rest of the current display line

## Synopsis

```c
void od_clr_line(void);
```

## Return value

N/A

## Description

This function clears the line that the cursor is on, from the cursor position
to the end of the line, subject to the plain ASCII limitation described below.
After the rest of the line is cleared, the cursor is automatically returned to
the position it was at prior to issuing the command. Hence, if the display line
the cursor was located on looked as follows, with the underscore (`_`)
character representing the cursor position:

```text
This is a_line of text!
```

With the cursor between the words "a" and "line", after the od_clr_line command is issued, the line would appear as follows:

```text
This is a_
```

With the cursor directly following the word "a". A space character is placed
at the cursor location and at each location cleared by the function.

When the door is running in plain ASCII mode, this command clears the line by
sending space characters for each remaining position before the final column,
followed by an equal number of backspace characters. The backspaces return the
cursor to its original position. The final column is deliberately not
overwritten in plain ASCII mode. Writing a character in that position may wrap
the cursor to the following line and, when the cursor is on the last display
line, may scroll the screen. Consequently, [`od_clr_line()`](od_clr_line.md) cannot guarantee
that the final character on the line has been erased in plain ASCII mode. If
the cursor is already in the final column, no characters are erased.

When ANSI, AVATAR or RIP modes are active, the corresponding ANSI or AVATAR
control sequence is sent to accomplish the line clear. These control sequences
do not require OpenDoors to write through the final column, and the line is
therefore cleared from the cursor position through the end of the line. Since
the graphics-mode sequences are much shorter than the sequence required to
clear the line manually, this function operates much more quickly when ANSI,
AVATAR or RIP mode is active.

Also note that in ANSI, AVATAR or RIP graphics modes, the line will be cleared
with the currently selected color attribute. Thus, if you wanted to place a
blue background on a particular line, you would use the
[`od_set_color()`](od_set_color.md) (or
[`od_set_attrib()`](od_set_attrib.md)) function, then use the
[`od_set_cursor()`](od_set_cursor.md) function to locate the cursor at the
beginning of the desired line, followed by the [`od_clr_line()`](od_clr_line.md) function. Just
such a procedure is demonstrated in the example below.

## Examples

Below, is an example of a function that clears an entire line with a specified color. Since this function performs operations that require ANSI, AVATAR or RIP graphics mode, it should only be used in a case where these modes are known to be available. For example, this function would be useful in a full-screen editor or viewer, or when performing ANSI animations. The function accepts three parameters: the line to be cleared (where 1 is the first line, 2 the second, and so on), the foreground color of this line, and the background color of this line.

This function differs from the [`od_clr_line()`](od_clr_line.md) function itself in several important manners. First of all, this function clears the entire line, whereas the [`od_clr_line()`](od_clr_line.md) function can be used to clear only the remaining characters of the line, after any particular location. Also, as mentioned before, this function selects a color to clear the line to, and moves the cursor to the line which is to be cleared - neither of which is done by the [`od_clr_line()`](od_clr_line.md) function.

```c
void clear_line(char line_number,char foreground,char
background)
{
   od_set_cursor(line_number,1);      /* move to correct line */
   od_set_color(foreground,background);          /* set color */
   od_clr_line();                        /* clear entire line */
}
```

## See also

[`od_clr_scr()`](od_clr_scr.md), [`od_scroll()`](od_scroll.md),
[`od_set_cursor()`](od_set_cursor.md)
