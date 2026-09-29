# `od_list_files()`

Lists files in a particular file area (using FILES.BBS)

## Synopsis

```c
BOOL od_list_files(char *pszFileSpec);
```

## Return value

[`TRUE`](../constants/general.md#true) when the listing was opened and display
completed or was stopped by the user; [`FALSE`](../constants/general.md#false)
when the directory or listing file could not be opened or an implementation
limit was exceeded.

## Description

This function displays a list of files available for download from a file
area. `pszFileSpec` may name either a directory containing `FILES.BBS` or a
particular `FILES.BBS`-compatible index file. `NULL` and the empty string select
`FILES.BBS` in the current directory. A directory may be specified with or
without its trailing directory separator. A nonempty file specification may
contain no more than 99 characters. Thus, to list the files available for
download in `C:\BBS\FILES\UPLOADS`, use:

```c
od_list_files("C:\\BBS\\FILES\\UPLOADS");
```

OpenDoors uses a third-generation FILES.BBS format, that is compatible with other FILES.BBS formats, but adds some additional features. Each line in the FILES.BBS file lists a filename, along with it's description. Thus, a typical FILES.BBS file might look as follows:

```text
PKZ110.EXE    PKZip file compressor, version 1.10
ODOORS60.ZIP  The newest version of OpenDoors
REC*.ZIP      A Record file
C:\BBS\*.*    All BBS files.
```

For each matching file, OpenDoors displays its name, size and description. If
no file matches an entry, the configurable
[`od_control.od_offline`](../control/customization.md#od_offline) string is
displayed in place of the size. A line whose first character is a space is
treated as a title or comment and displayed without filename processing. A
blank line is displayed as a blank title or comment line. Thus, the index
could contain:

```text
   NEWEST UPLOADS
   ~~~~~~~~~~~~~~
PKZ110.EXE    PKZip file compressor, version 1.10
ODOORS60.ZIP  The newest version of OpenDoors
REC*.ZIP      A Record file
C:\BBS\*.*    All BBS files.
```

In addition to the standard `FILES.BBS` format, OpenDoors permits wildcards in
the filename field, such as `FNEWS???.*`. Every matching file is displayed with
the description from that entry. Name matches include normal, archive, and
read-only files. A filename may include a full directory path, allowing one
index to list files from several directories. A relative filename is resolved
relative to the directory named by `pszFileSpec`, or relative to the directory
containing the explicitly named index file.

On DOS and Windows, a colon separates the drive prefix from the remaining
path. UNIX paths have no drive component, so a colon is treated as an ordinary
filename character and remains part of a relative or absolute path.

The display uses a 12-character filename column, a six-character decimal size
column, and up to 56 characters of description. Input lines are read in
511-character pieces. If a physical line exceeds that size, the remainder of
that line is discarded rather than interpreted as another entry.

The filename token at the beginning of an entry may contain no more than 79
characters. This limit applies to the complete token, including any drive,
directory, wildcard, and extension characters.

Within that token, the directory component through its final separator may
contain no more than 69 characters. An entry which exceeds this component
limit stops the listing, returns [`FALSE`](../constants/general.md#false), and
sets [`od_control.od_error`](../control/runtime.md#od_error) to
[`ERR_LIMIT`](../constants/errors.md#err_limit).

After a relative entry is combined with the index directory, the resolved path
may contain no more than 99 characters. The original index directory is used
independently for every entry. A resolved path which exceeds this limit also
stops the listing with [`ERR_LIMIT`](../constants/errors.md#err_limit).

You may alter the colors used for the different portions of the listing with
[`od_control.od_list_title_col`](../control/customization.md#od_list_title_col),
[`od_control.od_list_name_col`](../control/customization.md#od_list_name_col),
[`od_control.od_list_size_col`](../control/customization.md#od_list_size_col),
[`od_control.od_list_comment_col`](../control/customization.md#od_list_comment_col),
and [`od_control.od_list_offline_col`](../control/customization.md#od_list_offline_col).

Page pausing initially follows
[`od_control.od_page_pausing`](../control/runtime.md#od_page_pausing). When an
extended door-information record is active, bit `0x04` of
[`od_control.user_attribute`](../control/caller.md#user_attribute) supplies the
caller's pausing preference instead. The configured pause and stop keys are
honored while the listing is displayed. Stopping a listing is considered a
successful completion and returns [`TRUE`](../constants/general.md#true).

If the directory, index file, or initial directory entry cannot be opened,
OpenDoors returns [`FALSE`](../constants/general.md#false) and sets
[`od_control.od_error`](../control/runtime.md#od_error) to
[`ERR_FILEOPEN`](../constants/errors.md#err_fileopen). A later `fgets()` failure
is treated as end of file and is not reported separately.

If a nonempty `pszFileSpec` contains more than 99 characters, OpenDoors returns
[`FALSE`](../constants/general.md#false) and sets
[`od_control.od_error`](../control/runtime.md#od_error) to
[`ERR_LIMIT`](../constants/errors.md#err_limit).

An index entry whose filename token contains more than 79 characters produces
the same return value and error. Processing stops at that entry.

## See also

[`od_send_file()`](od_send_file.md), [The `od_control`
structure](../control/index.md)
