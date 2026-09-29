# `od_color_config()`

Parses a color configuration line from the configuration file, generating a color attribute value.

## Synopsis

```c
BYTE od_color_config(char *pszColorDesc);
```

## Return value

Color attribute value

## Description

This function will be of use if you are using the configuration file system of OpenDoors, and wish to allow the sysop to specify text colors to be used in your door. While OpenDoors automatically recognizes color configuration settings for things such as sysop chat mode and FILES.BBS listings, you may wish to add additional color configuration options. In this case, you could call the [`od_color_config()`](od_color_config.md) function from your custom line function. For more information on the custom line function, see the section on the OpenDoors configuration file system.

To use this function, pass the configuration-file text to be parsed in
`pszColorDesc`. The function returns an IBM-PC color attribute in the same
format accepted by [`od_set_attrib()`](od_set_attrib.md). A color description
will normally be written in the following form:

{Flashing} {Bright} [foreground] on [background]

Here, `Flashing` is an optional keyword indicating that the text should flash,
and `Bright` is an optional keyword indicating a high-intensity foreground.
`foreground` and `background` are color names. Case is not significant.

The words in the example are provided for readability; the parser does not
require the word `on` or any other punctuation. It separates the description at
spaces and tabs and compares each word with the names in
[`od_control.od_color_names`](../control/customization.md#od_color_names).
Unrecognized words are ignored. The first recognized color name sets the
foreground. Each later recognized color name sets the background, so if more
than two colors are supplied, the last of them becomes the background.

The recognized `BRIGHT` and `FLASHING` names set the intensity and blink bits
respectively. They do not consume the foreground or background position and
may appear anywhere in the description. If a foreground or background is not
specified, that part of the attribute retains its initial value. The initial
attribute is `0x07`, corresponding to grey on black; thus an entirely
unrecognized or empty description returns `0x07`.

The default recognized names are `BLACK`, `BLUE`, `GREEN`, `CYAN`, `RED`,
`MAGENTA`, `YELLOW`, `WHITE`, `BROWN`, `GREY`, `BRIGHT`, and `FLASHING`.
`BROWN` selects the same three-bit color value as `YELLOW`, and `GREY` selects
the same value as `WHITE`; the separate names allow the low-intensity forms to
be described naturally. The application may replace any default name before
initialization. Names supplied by the application must be uppercase because
the input token, but not the configured name, is converted to uppercase before
comparison.

When this function is called by [`od_printf()`](od_printf.md), parsing stops at
the active closing color delimiter. A direct call normally parses through the
terminating nul character. The function retains the point at which parsing
stopped for use by the [`od_printf()`](od_printf.md) parser; that retained address is not part
of the public interface. `pszColorDesc` must not be `NULL`.

Individual words are compared using at most their first 39 characters. If a
word is longer, the remaining characters are skipped before parsing continues
with the following word.

## Examples

See the example accompanying in the section on the OpenDoors configuration file system.

## See also

[`od_set_attrib()`](od_set_attrib.md), [`od_set_color()`](od_set_color.md),
[Colors](../constants/colors.md)
