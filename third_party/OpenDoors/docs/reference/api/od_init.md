# `od_init()`

To initialize OpenDoors activities

## Synopsis

```c
void od_init(void);
```

## Return value

N/A

## Description

This function initializes OpenDoors. It must be called explicitly if the
application needs caller or system information before calling another function
which performs initialization. Otherwise, most OpenDoors functions call
[`od_init()`](od_init.md) automatically when first used. A later explicit call returns
without repeating initialization.

The settings which control initialization must be assigned first. In
particular, call [`od_parse_cmd_line()`](od_parse_cmd_line.md) before
initialization, and install DOS personalities with
[`od_add_personality()`](od_add_personality.md) before initialization. The
pure command-line allocation helpers
[`od_split_cmd_line()`](od_split_cmd_line.md) and
[`od_free_split_cmd_line()`](od_free_split_cmd_line.md) may also be used at
that time. [`od_control_get()`](od_control_get.md) merely returns the address of
[`od_control`](../control/index.md) and does not initialize OpenDoors.

[`od_init()`](od_init.md) supplies defaults for unset program strings, color names,
configuration keywords, prompts, colors and other customization fields. It
then runs the enabled configuration and personality components, reads the
available door-information file, establishes communications, allocates the
common input queue, creates the session screen and local presentation,
initializes the status personality, starts the OpenDoors kernel, and opens the
activity log when that component is enabled. The individual
[`od_control`](../control/index.md) field descriptions identify which defaults
are unconditional, which preserve a value supplied by the application, and
which depend upon a particular door-information format.

The [`od_init()`](od_init.md) function searches for door information at the location in
[`od_control.info_path`](../control/connection.md#info_path). The value may
identify a directory to search or a recognized door-information filename. If
it is empty, OpenDoors searches the current directory. A door which runs with a
different working directory should accept the information path from the BBS
launch command or configuration rather than assuming that the BBS directory is
current.

The bit flags in
[`od_control.od_disable`](../control/customization.md#od_disable) can suppress
selected initialization and runtime activities. For example,
[`DIS_INFOFILE`](../constants/session.md#dis_infofile) prevents the
door-information search. [`od_control.od_force_local`](../control/customization.md#od_force_local)
selects local operation without a drop file. The complete meaning and timing of
each flag is given in the [session constants](../constants/session.md).

The node number is selected, in order, from the `TASK` environment variable,
the `SBBSNNUM` environment variable, a command-line node setting, an existing
nonzero [`od_control.od_node`](../control/connection.md#od_node), or the default
value 1. Fatal initialization failures, such as inability to allocate required
storage or establish the selected communications method, display an
initialization error and terminate with the configured value in
[`od_control.od_errorlevel`](../control/customization.md#od_errorlevel).
For a Windows console-subsystem application with local presentation enabled,
initialization uses an inherited console or allocates one when necessary. An
inability to obtain a usable console is fatal in local mode; a remote session
may fall back to silent operation. Silent mode performs no console allocation
or console state management.

Initialization is not reentrant. If an OpenDoors callback is currently active,
[`od_init()`](od_init.md) returns without doing anything. Applications must not use such a
return as evidence that initialization completed.

## Examples

At times, you may wish to write a door program which will require a maintenance utility to be run on a regular basis. For example, a game door may have to have its system files updated on a daily basis, by having a utility program run in a system event each day at midnight. One way of accomplishing this would be to have your door package include two .EXE files, one being the actual door program, and the other being a utility program. However, another option would be to have both the door and maintenance functions to be accessible from a single .EXE file, in order to simplify use of the door for the sysop. In this case, you would want to test the command line to determine whether your program should run in door mode or maintenance mode. You would then only execute the [`od_init()`](od_init.md) function, along with the rest of your door code, if you program were running in "door mode".

The program below demonstrates one method of doing just this. In this case, the program would include two functions, door(), which would carry out all of the door-related activities, and maint(), which would carry out all of the maintenance-related activities. In this simple example, if the command line includes a "-M" or "/M", the program will run in maintenance mode, otherwise it will run in door mode. Also, if it is running in door mode, the program will take the first command-line parameter, if any, as a path to the location of the door information file.

```c
#include <stdlib.h>
#include <string.h>
#include "OpenDoor.h"

static void door(void);
static void maint(void);

int main(int argc, char *argv[])
{
   int counter;

   for(counter = 1; counter < argc; ++counter)
   {
      if(strcmp(argv[counter], "/M") == 0
         || strcmp(argv[counter], "-M") == 0)
      {
         maint();
         return(20);
      }
   }

   /* This example accepts a bare information path as its first argument. */
   if(argc > 1)
   {
      strncpy(od_control.info_path, argv[1],
         sizeof(od_control.info_path) - 1);
      od_control.info_path[sizeof(od_control.info_path) - 1] = '\0';
   }

   od_init();
   door();
   od_exit(30, FALSE);
   return(30);                 /* od_exit() normally terminates first. */
}

static void maint(void)
{
   /* Carry out maintenance activities without initializing OpenDoors. */
}

static void door(void)
{
   /* Carry out the interactive door activities. */
}
```

## See also

[`od_parse_cmd_line()`](od_parse_cmd_line.md), [`od_kernel()`](od_kernel.md),
[`od_exit()`](od_exit.md), [Session
lifecycle](../../guides/session-lifecycle.md)
