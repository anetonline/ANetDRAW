# Configuration and command lines

OpenDoors can receive settings from the BBS door-information file, its standard
command line, a configuration file, and assignments made by the door itself. The
exact source is less important than timing: settings which control
initialization must be established before
[`od_init()`](../reference/api/od_init.md).

On Unix-like and DOS programs, pass `argc` and `argv` to
[`od_parse_cmd_line()`](../reference/api/od_parse_cmd_line.md). The Windows form
accepts the command-line string supplied to `WinMain`. Programs which need to
split such a string independently may use
[`od_split_cmd_line()`](../reference/api/od_split_cmd_line.md) and must later
release its result with
[`od_free_split_cmd_line()`](../reference/api/od_free_split_cmd_line.md).

The standard parser recognizes OpenDoors options and can pass application
options to callbacks installed in [`od_control`](../reference/control/index.md).
The configuration component is selected through
[`od_control.od_config_file`](../reference/control/customization.md);
[`INCLUDE_CONFIG_FILE`](../reference/constants/components.md) enables the built-in
component, while
[`NO_CONFIG_FILE`](../reference/constants/components.md) disables it. Set
[`od_control.od_config_filename`](../reference/control/customization.md) to
override the default name and use
[`od_control.od_config_function`](../reference/control/customization.md) for
program-specific keywords.

Door-information files supply the caller, system, connection, and time-limit
state used to populate [`od_control`](../reference/control/index.md). OpenDoors
recognizes several established BBS formats as well as Door32 and socket-oriented
operation. Programs should read the normalized
[`od_control`](../reference/control/index.md) fields rather than parse the
original file a second time.

## Detailed reference

One of the most useful OpenDoors features that you can optionally choose to include in your programs is the OpenDoors configuration file system. All that is required to enable the configuration file system is to include the following line before your first call to any OpenDoors function:

```c
od_control.od_config_file = INCLUDE_CONFIG_FILE;
```

OpenDoors will now search for and read an OpenDoors configuration file. If you do not specify the name of this file, the default name of `door.cfg` will be used. The filename is lowercase and must be written that way on case-sensitive file systems. Using this configuration file, the sysop can set a wide variety of options, such as modem and system configuration information, maximum time limits for the door, and even define custom door information (drop) file formats. The example `door.cfg` file included in your OpenDoors package shows the format and all options that are automatically supported by the configuration file system. This configuration file format is designed to be easy to use, and the example configuration file contains comments which provide a complete description of each option. Feel free to redistribute `door.cfg` or a modified version of this file with your door programs. In addition to the many configuration file settings already supported, you can add your own settings that are specific to your particular program.

To specify your own filename for the configuration file, use the od_config_filename control structure variable. A `NULL` pointer selects the default filename; an explicit empty string instead names a required file which cannot be opened and causes the normal missing-configuration-file error. For example, the following line:

[`od_control.od_config_filename`](../reference/control/customization.md#od_config_filename) = "MYDOOR.CFG"

causes OpenDoors to look for the configuration file MYDOOR.CFG instead of the default `door.cfg`.

OpenDoors will first search for the configuration file at the path specified in the od_config_filename variable. If a directory or drive was included and the file cannot be opened there, OpenDoors will retry the complete basename in the current directory. This fallback is attempted only when the basename fits in the 257-byte configuration-line buffer; an oversized basename is not truncated. If the configuration file system is unable to locate a configuration file, or if any settings are omitted from the file, the default values for these settings will be used automatically. This means that the configuration file is always optional, unless your program has custom settings that it requires in order to run.

The format for the configuration file is as follows. Blank lines and any text following the semi-colon (;) character are ignored. Configuration options are specified using a keyword, possibly followed by one or more options. The keywords are not case sensitive, but some of the options are. The order of options in the configuration file is not significant, with the exception of the "CustomFileLine" option. For more information on the "CustomFileLine" setting, see the section that begins on page 230. The built-in configuration options are as follow:

BBSDir - BBS System directory. Indicates where the door information file (drop file) can be found. The stored path is limited to 59 characters; longer values are truncated.

DoorDir - The door's working directory. This is where the door's system files are located. OpenDoors will automatically perform a chdir into this directory at initialization, and will return to the original directory on exit. An empty `DoorDir` is ignored and leaves the process working directory unchanged. OpenDoors also ignores the setting if it cannot save the original directory needed for restoration on exit. The stored path is limited to 79 characters; longer values are truncated before the directory change is attempted.

LogFileName - Specifies the filename (path optional) where the door should record log information. The stored filename is limited to 79 characters; longer values are truncated.

DisableLogging - Prevents door from writing to a log file.

Node - BBS node number that the door is running on. Only used if OpenDoors is unable to determine the node number by some other means.

???dayPagingHours - Specifies sysop paging hours. Sysop paging will be permitted beginning at the start time, up until, but not including, the end time. Times should be in the 24- hour format. To disable paging on a particular day, set the paging start and end times to the same time. ???day can be one of Sunday, Monday, Tuesday, Wednesday, Thursday, Friday or Saturday.

PageDuration - Duration of sysop page. Value indicates the number of beeps that compose the sysop page alarm.

MaximumDoorTime - Maximum length of time a user is permitted to access the door. If the user's total remaining time on the BBS is less than this value, the user will only be permitted to access the door for this shorter length of time. This option is disabled by commenting out the line.

InactivityTimeout - Specifies the maximum number of seconds that may elapse without the user pressing a key, before the user will automatically be disconnected. A value of 0 disables inactivity timeouts.

SysopName - Name of the sysop. OpenDoors can usually determine the sysop's name from the door information (drop) file. How3ever, some BBS packages do not supply this information. In such cases, if the sysop's name is required by the door, it may be supplied here.

SystemName - Like the sysop's name, this option can usually be determined from the door information file. If it is not available, the sysop my supply the information here.

ChatUserColor - Specifies the color of text typed by the user in sysop chat mode. The format of the color name is included in the description of the [`od_color_config()`](../reference/api/od_color_config.md) function.

ChatSysopColor - Specifies the color of test typed by the sysop in chat mode.

FileListTitleColor - Files.BBS listing colors. FileListNameColor FileListSizeColor FileListDescriptionColor FileListOfflineColor

SwappingDir - Directory where disk swapping will be done.

SwappingNoEMS - Disables swapping to EMS memory.

SwappingDisable - Disables swapping entirely.

LockedBPS -  BPS rate at which door should communicate with the modem.  Valid rates are 300, 600, 1200, 2400, 4800, 9600, 19200 and 38400. A value of 0 forces the door to always operate in local mode. This option is not normally needed, as the information is usually available from the door information file.

FossilPort - Specifies the FOSSIL driver port number that the modem is connected to. FOSSIL port 0 usually corresponds to COM1, port 1 to COM2, and so on. This option is not normally needed, as the information is usually available from the door information file.

PortAddress - Specifies the hexadecimal base I/O address of a serial port when using direct UART communications under DOS or DOS32. OpenDoors skips characters preceding the first hexadecimal digit and reads the value which follows as a 16-bit hexadecimal number. If the option contains no hexadecimal digits, the current port-address setting is left unchanged. This option is normally required only for ports above COM4 and has no effect when a FOSSIL driver is used.

CustomFileName - Specifies the filename used by the custom door information file format. Described in more detail below.

CustomFileLine - Specifies the contents of a particular line in the custom door information file format.

The last two configuration file options, "CustomFileName" and "CustomFileLine" allow you or the system operator using your program to define your own door information (drop) file formats. For more information on this topic, see the section which begins on page 230.

You can also extend OpenDoor's configuration file format to add your own options by supplying a callback function. OpenDoors calls this function for every nonblank configuration line, including lines which contain one of the built-in keywords. Built-in options are processed before the callback is called, so the callback can observe or separately use their settings as well as process options belonging only to the door. The prototype of this function should be as follows:

custom_line_function(char *keyword, char *options)

To cause OpenDoors to use your function, you would include the following line before your first call to any OpenDoors function:

```c
od_control.od_config_function = custom_line_function;
```

(You can use a different function name if you wish.) For each nonblank configuration line, OpenDoors calls your function after processing any matching built-in option. It passes a pointer to an upper case version of the keyword string in the first parameter, and a pointer to any options that follow the keyword in the second parameter. For instance, if the following line were encountered in the configuration file:

RegisteredTo    John Smith      ; Sysop's name

The parameters passed to your function would be:

char *keyword = "REGISTEREDTO" char *options = "John Smith"

If a keyword has no following option text, the `options` argument points to an
empty string. It is never `NULL`. This permits presence-only settings such as
`DisplayWinners` to be handled without a dummy value.

Your custom line function should be written in such a way that if OpenDoors passes a configuration option to your function that your function does not recognize, that option is simply ignored. This includes built-in options in which the door has no separate interest.

The example program below demonstrates how to use the custom line function to add your own configuration file options. This program looks for three custom configuration file options, "RegistrationKey", "DefaultColor" and "DisplayWinners". If the "RegistrationKey" option is present, the numerical value following this option is stored in the global variable "key". If the "DefaultColor" option is present, the color description (such as "Bright Red on Black") is translated to an `od_set_attr()` color code using [`od_color_config()`](../reference/api/od_color_config.md). This color setting is stored in the global variable default_color. Since this variable is initialized to 0x07 (the value for dark white on black), if this option is omitted, that color is used by default. If the "DisplayWinners" option is included in the configuration file, the global variable display_winners is set to TRUE, regardless of any options that may follow this keyword.

```c
#include "opendoor.h"                  /* Include opendooor.h */
                        /* Prototype for custom line function */
void custom_line_function(char *keyword, char *options);
```

unsigned long key=0L; /* Variables for our own config option */ unsigned char default_color=0x07; char display_winners=FALSE;

```c
main()                     /* Program's execution begins here */
{               /* Begin door operations, reading config file */
   od_control.od_config_file = INCLUDE_CONFIG_FILE;
                /* Tell OpenDoors to use custom line function */
   od_control.od_config_function = custom_line_function;
   od_init();
   /* Main program's operations go here */
   od_exit(10, FALSE);                        /* Exit program */
}
                             /* Code for custom line function */
void custom_line_function(char *keyword, char *options)
{                            /* If option is registration key */
   if(stricmp(keyword,"REGISTRATIONKEY")==0)
   {
      key=atol(options);             /* Store key in variable */
   }                               /* If option is text color */
   else if(stricmp(keyword,"DEFAULTCOLOR")==0)
   {               /* Get color value using od_color_config() */
      default_color=od_color_config(options);
   }         /* Example of option enabled by just the keyword */
   else if(stricmp(keyword,"DISPLAYWINNERS")==0)
   {                 /* If keyword is present, turn on option */
      display_winners=TRUE;
   }
}
```
