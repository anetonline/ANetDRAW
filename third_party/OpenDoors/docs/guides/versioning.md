# Versioning and compatibility

OpenDoors retains the version notation used by the original toolkit while
also presenting releases in the three-component form expected by current
build and package tools.

## Reading an OpenDoors version

The traditional version `6.30` and the release version `6.3.0` identify the
same source. The traditional form omits the dot between the minor and patch
digits:

| Meaning | Release form | Traditional form | [`OD_VERSION`](../reference/constants/general.md) |
| --- | --- | --- | --- |
| OpenDoors 6, ABI generation 3, maintenance release 0 | `6.3.0` | `6.30` | `0x630` |

Each component is represented by one hexadecimal digit in
[`OD_VERSION`](../reference/constants/general.md). A future version that
cannot be represented this way requires an explicit revision of the version
scheme rather than silently changing the macro's interpretation.

## Compatibility promises

The three components have the following meanings:

- The **major** number identifies the public source/API generation. A change
  that requires existing door source to be rewritten increments this number.
- The **minor** number identifies the binary ABI generation. A change such as
  altering the layout of [`tODControl`](../reference/control/index.md)
  increments this number even when existing source still compiles.
- The **patch** number identifies a binary-compatible maintenance release.
  Fixes and additive interfaces may increment this number when existing
  programs and libraries remain compatible.

Windows DLL names contain the major and minor digits, such as
`ODoors63.dll`. Unix and macOS shared libraries use the same `6.3` ABI line
in their SONAME or compatibility version. Patch releases within that line do
not change the ABI name.

## Tags and package versions

Git tags, GitHub Releases, CMake package files, and release asset names use
the dotted form, for example `v6.3.0`. OpenDoors' status line, sign-on text,
and other historically visible strings continue to use `6.30`.

The root `VERSION` file is the canonical dotted version. CI checks the other
representations against it before building a release.
