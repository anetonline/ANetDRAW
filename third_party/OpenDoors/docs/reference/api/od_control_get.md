# `od_control_get()`

Returns the address of the OpenDoors control structure.

## Synopsis

```c
tODControl *od_control_get(void);
```

## Return value

Before terminal shutdown, the return value is a pointer to the same
[`tODControl`](../control/index.md) object exported as the global
[`od_control`](../control/index.md) variable. After [`od_exit()`](od_exit.md)
completes it returns `NULL` and sets
[`od_control.od_error`](../control/runtime.md#od_error) to
[`ERR_GENERALFAILURE`](../constants/errors.md#err_generalfailure).

The pointed-to structure is the writable public [`od_control`](../control/index.md) object and belongs
to OpenDoors. The application must not free it or assume that the pointer refers
to a separately allocated session object. Individual fields may be read and
written under the same rules as the global variable. Replacing the entire
structure is not a supported initialization or reset operation because it also
overwrites live runtime state and library-owned pointers.

## Description

OpenDoors keeps the information and settings for the active door session in
one [`tODControl`](../control/index.md) structure. C and C++ applications
normally access that structure through the global [`od_control`](../control/index.md)
variable:

```c
od_control.od_prog_name = "Example Door";
od_control.od_prog_version = "1.0";
```

Some foreign-function interfaces and dynamic loaders can call an exported
function more easily than they can import an exported data symbol.
[`od_control_get()`](od_control_get.md) provides function-based access for those environments:

```c
tODControl *control = od_control_get();

control->od_prog_name[0] = '\0';
strncat(control->od_prog_name, "Example Door",
    sizeof(control->od_prog_name) - 1);
```

Dereferencing the returned pointer and accessing the global variable are
exactly equivalent. A change made through one form is immediately visible
through the other. An already obtained pointer remains valid while that
OpenDoors library instance is loaded. The host may directly read the object
after shutdown, but must not call OpenDoors again or use the retained pointer
to attempt another session.

Unlike most OpenDoors API functions, [`od_control_get()`](od_control_get.md) does not initialize
OpenDoors and does not run the OpenDoors kernel. It is therefore safe to call
while preparing settings that must be established before initialization. This
does not change the timing rules of the individual fields: a field documented
as an initialization setting must still be assigned before
[`od_init()`](od_init.md) or before another API function initializes OpenDoors
implicitly.

The function does not provide synchronization or create independent control
structures. OpenDoors has one active control structure per loaded library
instance. All API and ABI access must occur on the thread which calls
[`od_init()`](od_init.md); obtaining the pointer does not transfer ownership.

## Errors

After terminal shutdown, this function returns `NULL` and sets
[`od_control.od_error`](../control/runtime.md#od_error) to
[`ERR_GENERALFAILURE`](../constants/errors.md#err_generalfailure).

## Example

The following helper uses function-based access to select local mode before
initialization:

```c
static void configure_local_session(void)
{
    tODControl *control = od_control_get();

    control->od_force_local = TRUE;
    control->od_silent_mode = TRUE;
}
```

## See also

[`od_control`](../control/index.md), [`tODControl`](../control/index.md),
[`od_init()`](od_init.md)
