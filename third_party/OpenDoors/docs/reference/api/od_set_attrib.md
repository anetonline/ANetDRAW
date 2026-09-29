# `od_set_attrib()`

Selects a complete IBM-PC text attribute

## Synopsis

```c
void od_set_attrib(INT nColor);
```

## Return value

N/A

## Description

[`od_set_attrib()`](od_set_attrib.md) selects the attribute used for subsequently displayed text
and for cells cleared by OpenDoors. `nColor` is normally a value from 0 through
255 containing the traditional IBM-PC text attribute:

```text
bit 7       blink
bits 6..4   background color (0..7)
bit 3       foreground intensity
bits 2..0   foreground color (0..7)
```

Thus `0x1e` selects an intense yellow foreground on a blue background, while
`0x9e` requests the same colors with blink. The complete foreground,
background, and blink names are listed under [Colors](../constants/colors.md).
[`od_set_color()`](od_set_color.md) constructs the same byte from separate
foreground and background arguments.

A value of `-1` is a special request to leave the current attribute unchanged.
Other values outside the range of an attribute byte are not part of the public
contract.

In AVATAR mode OpenDoors sends the set-attribute command when the requested
value differs from the value recorded in
[`od_control.od_cur_attrib`](../control/runtime.md#od_cur_attrib), or whenever
[`od_control.od_full_color`](../control/customization.md#od_full_color) is
enabled. In ANSI mode it emits only the reset, intensity, blink, foreground,
and background parameters needed to reach the new value. Enabling
[`od_full_color`](../control/customization.md#od_full_color) causes a complete ANSI attribute sequence to be sent each
time. The virtual and local screen attributes are updated to the same byte.

When neither ANSI nor AVATAR operation is available, no attribute is changed
and [`ERR_NOGRAPHICS`](../constants/errors.md#err_nographics) is placed in
[`od_control.od_error`](../control/runtime.md#od_error). Plain-ASCII terminals
do not carry per-character color attributes. The function returns no value.

## Example

This example displays all 256 attribute values and accepts a decimal
selection:

```c
unsigned char choose_color(void)
{
   unsigned int counter;
   char string[4];

   od_set_attrib(0x07);
   od_disp_str("Available colors:\n\r\n\r");

   for(counter = 0; counter <= 255; ++counter)
   {
      od_set_attrib((INT)counter);
      od_printf("%03u", counter);
      if((counter % 16) == 15)
      {
         od_set_attrib(0x07);
         od_disp_str("\n\r");
      }
   }

   od_set_attrib(0x07);
   od_disp_str("Which color do you prefer: ");
   od_input_str(string, 3, '0', '9');
   return (unsigned char)atoi(string);
}
```

## See also

[`od_set_color()`](od_set_color.md), [`od_color_config()`](od_color_config.md),
[`od_printf()`](od_printf.md), [Colors](../constants/colors.md)
