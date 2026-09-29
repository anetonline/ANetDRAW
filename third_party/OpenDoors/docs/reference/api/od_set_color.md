# `od_set_color()`

Selects foreground and background text colors

## Synopsis

```c
void od_set_color(INT nForeground, INT nBackground);
```

## Return value

N/A

## Description

[`od_set_color()`](od_set_color.md) selects the foreground and background used for subsequently
displayed text. It combines both arguments into an IBM-PC attribute and passes
the result to [`od_set_attrib()`](od_set_attrib.md). ANSI, AVATAR, or RIP
operation is required for the color change to take effect on the caller's
terminal.

The foreground may be any of the following values:

| Value | Color |
| --- | --- |
| [`D_BLACK`](../constants/colors.md#d_black) | Black |
| [`D_BLUE`](../constants/colors.md#d_blue) | Dark blue |
| [`D_GREEN`](../constants/colors.md#d_green) | Dark green |
| [`D_CYAN`](../constants/colors.md#d_cyan) | Dark cyan |
| [`D_RED`](../constants/colors.md#d_red) | Dark red |
| [`D_MAGENTA`](../constants/colors.md#d_magenta) | Dark magenta |
| [`D_BROWN`](../constants/colors.md#d_brown) | Brown |
| [`D_GREY`](../constants/colors.md#d_grey) | Light grey, historically called dark white |
| [`L_BLACK`](../constants/colors.md#l_black) | High-intensity black, normally displayed as dark grey |
| [`L_BLUE`](../constants/colors.md#l_blue) | Bright blue |
| [`L_GREEN`](../constants/colors.md#l_green) | Bright green |
| [`L_CYAN`](../constants/colors.md#l_cyan) | Bright cyan |
| [`L_RED`](../constants/colors.md#l_red) | Bright red |
| [`L_MAGENTA`](../constants/colors.md#l_magenta) | Bright magenta |
| [`L_YELLOW`](../constants/colors.md#l_yellow) | Yellow |
| [`L_WHITE`](../constants/colors.md#l_white) | Bright white |

An ordinary background uses one of the eight [`D_*`](../constants/colors.md#dark-foreground-colors) values. A blinking
background uses the corresponding [`B_*`](../constants/colors.md#blinking-background-names) name:

| Ordinary | Blinking |
| --- | --- |
| [`D_BLACK`](../constants/colors.md#d_black) | [`B_BLACK`](../constants/colors.md#b_black) |
| [`D_BLUE`](../constants/colors.md#d_blue) | [`B_BLUE`](../constants/colors.md#b_blue) |
| [`D_GREEN`](../constants/colors.md#d_green) | [`B_GREEN`](../constants/colors.md#b_green) |
| [`D_CYAN`](../constants/colors.md#d_cyan) | [`B_CYAN`](../constants/colors.md#b_cyan) |
| [`D_RED`](../constants/colors.md#d_red) | [`B_RED`](../constants/colors.md#b_red) |
| [`D_MAGENTA`](../constants/colors.md#d_magenta) | [`B_MAGENTA`](../constants/colors.md#b_magenta) |
| [`D_BROWN`](../constants/colors.md#d_brown) | [`B_BROWN`](../constants/colors.md#b_brown) |
| [`D_GREY`](../constants/colors.md#d_grey) | [`B_GREY`](../constants/colors.md#b_grey) |

The [`B_*`](../constants/colors.md#blinking-background-names) values set the traditional IBM-PC blink bit. A modern terminal may
display that bit as a bright background or may ignore blinking; OpenDoors
cannot control that terminal preference.

For example:

```c
od_set_color(L_WHITE, D_BLACK);
```

selects bright white on black. The equivalent combined-attribute call is:

```c
od_set_attrib(L_WHITE | (D_BLACK << 4));
```

Both arguments must be one of the documented color constants. The function
does not independently validate them. When graphics are unavailable,
[`od_set_attrib()`](od_set_attrib.md) leaves the current attribute unchanged
and records [`ERR_NOGRAPHICS`](../constants/errors.md#err_nographics).

## Example

The following helpers retain the unmodified half of a color while changing
the other half. They assume they are the only routines used by the application
to select colors:

```c
static INT current_foreground = D_GREY;
static INT current_background = D_BLACK;

static void set_foreground(INT foreground)
{
   current_foreground = foreground;
   od_set_color(current_foreground, current_background);
}

static void set_background(INT background)
{
   current_background = background;
   od_set_color(current_foreground, current_background);
}
```

They may then be called independently:

```c
set_foreground(L_YELLOW);
set_background(D_BLUE);
```

## See also

[`od_set_attrib()`](od_set_attrib.md), [`od_printf()`](od_printf.md),
[`od_color_config()`](od_color_config.md), [Colors](../constants/colors.md)
