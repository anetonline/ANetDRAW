# `ODScrnSetAttribute()`

Selects the DOS local-screen text attribute.

## Synopsis

```c
void ODScrnSetAttribute(BYTE btAttribute);
```

The low nibble selects the foreground and the high nibble selects the
background and blink/intensity bits in the usual DOS text-mode form. The
attribute applies to subsequent local personality output.
