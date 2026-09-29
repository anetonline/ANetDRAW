# Custom door-information file formats

The configuration component can describe line-oriented drop-file formats which are not built into OpenDoors.

## Detailed reference

As is mentioned in the previous section, the OpenDoors configuration file system provides two settings which allow the sysop to define a custom door information file format. This permits OpenDoors doors to operate directly on any BBS system that produces a door information file format not directly supported by OpenDoors. A custom door information file format is defined using the "CustomFileName" option, followed by one or more lines beginning with the "CustomFileLine" option.

The "CustomFileName" option specifies the filename used to distinguish this file format from other file formats. This filename should not include a path. To specify the path where the door information file is located, the sysop should use the BBSDir configuration file setting. If the filename of the custom format is the same as that of one of the built-in formats, the custom format will override the built-in format.

The actual format of the custom file is specified using a number of lines that begin with the keyword "CustomFileLine". Each of these lines will correspond to a single physical line in the door information file, with the option following the "CustomFileLine" keyword specifying the information that can be found on that line. LF and CRLF line endings are removed before the contents are interpreted. An empty line is still consumed by its corresponding `CustomFileLine`; it supplies an empty string rather than being skipped. This can be one of the following keywords:

Ignore - Causes the next line in the door information file to be ignored. Use on lines for which none of the options below apply.

COMPORT - COM? port the modem is connected to (0 indicates local mode)

FOSSILPORT - Fossil port number the modem is connected to

MODEMBPS - BPS rate at which to communicate with modem (0 or non-numerical value indicates local mode)

LOCALMODE - 1, T or Y if door is operating in local mode

USERNAME - Full name of the user

USERFIRSTNAME - First name(s) of the user

USERLASTNAME - Last name of the user

ALIAS - The user's pseudonym / handle

HOURSLEFT - Hours user has left online

MINUTESLEFT - Minutes user has left online, or time left online in format hh:mm

SECONDSLEFT - Seconds user has left online, or time left online in format hh:mm:ss or format mm:ss (If more than one of the above time options are used, the user time left is taken to be the total of all of these values.)

ANSI - 1, T, Y or G for ANSI graphics mode

AVATAR - 1, T or Y for AVATAR graphics mode

PAGEPAUSING - 1, T or Y if user wishes a pause at end of screen

SCREENLENGTH - Number of lines on user's screen

SCREENCLEARING - 1, T or Y if screen clearing mode is on

SECURITY - The user's security level / access level

CITY - City the user is calling from

NODE - Node number user is connected to

SYSOPNAME - Full name of the sysop

SYSOPFIRSTNAME - The sysop's first name(s)

SYSOPLASTNAME - The sysop's last name

SYSTEMNAME - Name of the BBS

As an example of how to define custom door information file formats, consider the following imaginary file format, which we will name DROPINFO.TXT:

Brian Pirie         <-- User name 0                   <-- Local mode COM1:               <-- Serial port to use 9600                <-- BPS rate 22:30:15 05-08-95   <-- File creation time 35                  <-- Time remaining (in minutes) 1                   <-- ANSI mode Ottawa, Canada      <-- Location

This format would be defined in an OpenDoors configuration file as follows:

CustomFileName DROPINFO.TXT CustomFileLine USERNAME CustomFileLine LOCALMODE CustomFileLine COMPORT CustomFileLine MODEMBPS CustomFileLine IGNORE CustomFileLine MINUTESLEFT CustomFileLine ANSI CustomFileLine CITY

Notice that the first "CustomFileLine" keyword in the configuration file corresponds to the first line in our DROPINFO.TXT file, the second "CustomFileLine" to the second line, and so on. Also notice that the keyword "IGNORE" is used for the line that contains the file creation time, since there is no CustomFileLine keyword that allows you to read this information.
