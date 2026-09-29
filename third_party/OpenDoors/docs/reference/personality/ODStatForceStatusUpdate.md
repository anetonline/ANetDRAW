# `ODStatForceStatusUpdate()`

Requests an immediate incremental update of the active personality display.

## Synopsis

```c
void ODStatForceStatusUpdate(void);
```

OpenDoors invokes the current
[`PEROP_UPDATE*`](../constants/components.md#personality-procedure-operations)
operation as soon as the DOS kernel can process it. A request made from inside
a personality callback is completed later in the active kernel pass.

Use [`od_control.od_update_status_now`](../control/runtime.md) when a complete
redraw is required because part of the status display may have been
overwritten.
