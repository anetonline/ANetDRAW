# `od_get_answer()`

Waits for the user to select one of a specified set of keys.

## Synopsis

```c
char od_get_answer(const char *pszOptions);
```

## Description

`pszOptions` points to a null-terminated string containing every acceptable
one-byte response. [`od_get_answer()`](od_get_answer.md) waits until the user
presses one of those keys and ignores all other input.

Matching is not case-sensitive, but the function returns the character in the
form in which it appears in `pszOptions`. For example:

```c
od_get_answer("YN");
```

returns `Y` for either `y` or `Y`, while:

```c
od_get_answer("yn");
```

returns `y` for either form. Spaces, digits, punctuation, carriage return, and
line feed can be included like any other byte. Thus the following accepts the
letters `A`, `B`, `C`, and `Z`, space, the digits 1 through 3, Enter, or line
feed:

```c
od_get_answer("ABC 123\n\rZ");
```

Input is obtained through [`od_get_key()`](od_get_key.md), so remote input and
an enabled local keyboard are both accepted, and OpenDoors continues its normal
kernel processing while waiting.

`pszOptions` must be non-null and must contain at least one character. The
legacy interface does not diagnose a null or empty option set; a null pointer
has undefined behavior and an empty string waits indefinitely without being
able to accept a key.

## Return value

The function returns the matching byte from `pszOptions`. It has no separate
cancel or error return.

## See also

[`od_get_key()`](od_get_key.md), [`od_hotkey_menu()`](od_hotkey_menu.md)
