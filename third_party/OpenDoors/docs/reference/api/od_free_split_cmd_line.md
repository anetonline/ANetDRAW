# `od_free_split_cmd_line()`

Releases an argument array created by [`od_split_cmd_line()`](od_split_cmd_line.md).

## Synopsis

```c
void od_free_split_cmd_line(char **papszArguments);
```

## Parameters

`papszArguments`
: The exact pointer returned by a successful call to
  [`od_split_cmd_line()`](od_split_cmd_line.md).

## Return value

This function does not return a value.

## Description

[`od_split_cmd_line()`](od_split_cmd_line.md) allocates both an array of
argument pointers and the storage containing the argument strings.
[`od_free_split_cmd_line()`](od_free_split_cmd_line.md) releases the complete result. Applications must use
this function instead of attempting to free the individual pointers
themselves; the strings do not all have independent allocations.

Pass the returned pointer unchanged:

```c
INT argc;
char **argv = od_split_cmd_line(command_line, &argc);

if(argv != NULL)
{
    /* Use argv[0] through argv[argc - 1]. */
    od_free_split_cmd_line(argv);
}
```

The function must be called exactly once for each successful split. After it
returns, the array, every string in the array, and every pointer previously
obtained from those strings are invalid. The function does not clear the
caller's pointer; assigning `NULL` after freeing is the caller's
responsibility.

Do not pass a conventional `argv` received by `main()`, a manually constructed
array, an interior pointer such as `papszArguments + 1`, or an array already
freed by an earlier call. Those values do not have the allocation layout that
this function expects and result in undefined behavior.

It is valid to call [`od_free_split_cmd_line()`](od_free_split_cmd_line.md) before OpenDoors has been
initialized. It neither initializes OpenDoors nor runs the kernel.

## Errors

If `papszArguments` is `NULL`, no memory is released and
[`od_control.od_error`](../control/runtime.md#od_error) is set to
[`ERR_PARAMETER`](../constants/errors.md#err_parameter). No other error is
reported.

## Example

This example prints each word in a command line and then releases the result:

```c
INT count;
INT index;
char **arguments = od_split_cmd_line("-local -node 2", &count);

if(arguments == NULL)
{
    od_printf("Unable to split command line.\n\r");
}
else
{
    for(index = 0; index < count; ++index)
        od_printf("Argument %d: %s\n\r", index, arguments[index]);

    od_free_split_cmd_line(arguments);
    arguments = NULL;
}
```

## See also

[`od_split_cmd_line()`](od_split_cmd_line.md),
[`od_parse_cmd_line()`](od_parse_cmd_line.md)
