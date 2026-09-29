# Colors

OpenDoors uses the traditional IBM-PC four-bit color numbers. A text attribute
contains a foreground nibble in bits 0–3 and a background value in bits 4–6;
bit 7 is the traditional blink bit. The same numerical constants therefore
have slightly different names and effects depending on whether they are passed
as a foreground or background.

[`od_set_color()`](../api/od_set_color.md) accepts separate foreground and
background values and constructs the attribute. [`od_set_attrib()`](../api/od_set_attrib.md)
accepts an already combined attribute byte. The color fields in
[`od_control`](../control/customization.md#color-customization) also store
combined attributes unless their individual reference says otherwise.

## Dark foreground colors

The `D_*` names are values 0 through 7. They select normal-intensity
foregrounds. When used as a background, the same values select the eight
ordinary background colors.

### `D_BLACK`

Value 0. As a foreground it selects black; against a black background this is
invisible. As a background it selects black. A default monochrome-style
attribute often combines `D_GREY` with `D_BLACK`.

### `D_BLUE`

Value 1. Selects dark blue as a foreground or blue as a background.

### `D_GREEN`

Value 2. Selects dark green as a foreground or green as a background.

### `D_CYAN`

Value 3. Selects dark cyan as a foreground or cyan as a background.

### `D_RED`

Value 4. Selects dark red as a foreground or red as a background.

### `D_MAGENTA`

Value 5. Selects dark magenta as a foreground or magenta as a background.

### `D_BROWN`

Value 6. Selects brown as a foreground. The same bit pattern selects the
brown/yellow member of the ordinary background palette.

### `D_GREY`

Value 7. Selects light grey—historically called dark white—as a foreground,
or grey/white as a background. This is the conventional default text
foreground.

## Light foreground colors

The `L_*` names are values 8 through 15. Bit 3 raises the foreground intensity.
All eight are valid foreground choices. They do not select an additional set
of bright backgrounds in the established OpenDoors attribute model.

### `L_BLACK`

Value 8. Selects high-intensity black, conventionally displayed as dark grey.

### `L_BLUE`

Value 9. Selects bright blue.

### `L_GREEN`

Value 10. Selects bright green.

### `L_CYAN`

Value 11. Selects bright cyan.

### `L_RED`

Value 12. Selects bright red.

### `L_MAGENTA`

Value 13. Selects bright magenta.

### `L_YELLOW`

Value 14. Selects yellow. It is the high-intensity form of `D_BROWN`.

### `L_WHITE`

Value 15. Selects bright white. It is the high-intensity form of `D_GREY`.

## Blinking background names

When values 8 through 15 are supplied as the background argument to
[`od_set_color()`](../api/od_set_color.md), their low three bits select the
background and their high bit requests blink. The `B_*` aliases make that use
clear. Their numerical values are identical to the corresponding `L_*`
foreground values.

### `B_BLACK`

Alias of `L_BLACK`, value 8. Selects a black background with the blink bit set.

### `B_BLUE`

Alias of `L_BLUE`, value 9. Selects a blue background with blink.

### `B_GREEN`

Alias of `L_GREEN`, value 10. Selects a green background with blink.

### `B_CYAN`

Alias of `L_CYAN`, value 11. Selects a cyan background with blink.

### `B_RED`

Alias of `L_RED`, value 12. Selects a red background with blink.

### `B_MAGENTA`

Alias of `L_MAGENTA`, value 13. Selects a magenta background with blink.

### `B_BROWN`

Alias of `L_YELLOW`, value 14. Selects a brown background with blink. The name
describes the three-bit background color; it is not a bright yellow background.

### `B_GREY`

Alias of `L_WHITE`, value 15. Selects a grey background with blink.

[`B_YELLOW`](../compatibility.md#status-and-color-aliases) and
[`B_WHITE`](../compatibility.md#status-and-color-aliases) are compatibility
spellings for values 14 and 15. They do not introduce bright-background
semantics.

## Combining an attribute

To combine a foreground and background explicitly, keep the foreground's low
four bits and move the background into the upper nibble:

```c
BYTE attribute = (BYTE)(L_YELLOW | (D_BLUE << 4));
od_set_attrib(attribute);
```

The equivalent high-level call is:

```c
od_set_color(L_YELLOW, D_BLUE);
```

Use the high-level call when choosing colors by name. Use a combined byte when
storing a complete attribute or passing it to an API which explicitly accepts
one.

## Terminal representation

These values describe the logical IBM-PC attribute even on Windows, macOS, and
Unix-like systems. OpenDoors translates that attribute into the remote
terminal's ANSI/AVATAR output and into the local virtual-screen representation.
The actual RGB color, intensity, or blink effect is chosen by the terminal.

Many modern terminals treat blink as a bright-background request or disable
blinking entirely. OpenDoors preserves the established bit and wire-level
intent; it cannot require a terminal emulator to animate it. Doors which need
the broadest readability should prefer normal backgrounds and use foreground
intensity for emphasis.

Color selection does not change character encoding. CP437-to-UTF-8 conversion,
terminal emulation, and display attributes are independent stages of output.
