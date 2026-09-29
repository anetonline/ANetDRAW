# Isolated unit tests

This tree contains the OpenDoors white-box unit-test system. It is separate
from the public acceptance tests in `tests/` and does not require or permit
test hooks in production sources.

## Ownership and isolation

`sources.json` is the authoritative list of first-party library source files.
`inventory.json` is the generated conservative inventory of C function
definitions and assembler procedures, including definitions in inactive
preprocessor branches. `tests.json` assigns a case file and applicable
platforms to each unique source/function pair.

The generator constructs one translation unit for one function. The selected
body is retained and renamed `utt_<name>`. Other bodies in the source file are
removed. Every function dependency is renamed `utm_<name>` and receives a
typed default mock which fails on an unexpected call. A case overrides a mock
by defining `UT_CUSTOM_MOCK_<name>` and supplying the correspondingly named
function. Referenced external objects receive private test storage.

The original source files and headers are never rewritten. Generated files
belong under an ignored build directory.

A function whose implementation changes with preprocessor state may declare
multiple named `configurations` in `tests.json`. Each configuration is an
independent ownership and coverage obligation. `defines` and `undefines`
select the source variant for both Clang analysis and the selected compiler;
`native_flags` supplies warnings or other options only to the modern native
compiler and is not passed to Open Watcom. Configuration names must be unique
and safe for use in generated filenames. An optional configuration-level
`platforms` list limits that variant to a subset of the function's registered
platforms; this is useful for build options such as Windows diagnostics. The
manifest validator rejects an empty configuration set, malformed macro
operations, a macro both defined and undefined by the same configuration,
configuration platforms outside the function's platform set, and any function
platform left without a runnable configuration.

A Windows configuration may name a test-only C source in `windows_fixture`.
The runner builds it as a DLL beside the isolated executable with undecorated
stdcall exports; `ODComOpen()` uses this to validate the real Door32 loader and
export ABI on x86 and x64. A Turbo configuration may name a real-mode assembly
fixture in `turbo_tsr_fixture`; the Turbo runner assembles, links, and installs
the TSR before executing that unit. `turbo_late_declarations` supplies C89
prototypes needed by aliases inserted after the original compiler headers.
These fixtures remain in the test tree and do not alter production sources or
public headers.

## Local use

Regenerate and validate the inventory:

```
PYTHONDONTWRITEBYTECODE=1 python3 unit/tools/inventory.py --write
PYTHONDONTWRITEBYTECODE=1 python3 unit/tools/check.py
```

Run one function, one source, or all registered UNIX cases:

```
PYTHONDONTWRITEBYTECODE=1 python3 unit/tools/run.py --coverage \
    --function od_autodetect
PYTHONDONTWRITEBYTECODE=1 python3 unit/tools/run.py --coverage \
    --source src/ODAuto.c
PYTHONDONTWRITEBYTECODE=1 python3 unit/tools/run.py --coverage
```

CI executes the Unix suite on Ubuntu and macOS. The macOS jobs use
the current Homebrew LLVM toolchain so native execution and LLVM line/MC/DC
reporting are both available.

On a non-Windows host with working 32- and 64-bit Wine installations, MinGW
cases can be compiled and executed locally by naming the appropriate Wine
launcher. Wine requires an X display even for these console tests, so use the
`tools/unit-xvfb` wrapper with an explicit private display. For example:

```
tools/unit-xvfb --display :91 run --platform windows \
    --windows-architecture x86 --cc i686-w64-mingw32-gcc \
    --wine /usr/local/bin/wine
tools/unit-xvfb --display :92 run --platform windows \
    --windows-architecture x64 --cc x86_64-w64-mingw32-gcc \
    --wine /usr/local/bin/wine64
```

`tools/unit-xvfb` supplies a narrow, approvable wrapper around the two unit
runners. The private display is a required
parameter, preventing a run from accidentally inheriting or assuming the
interactive display. The wrapper explicitly refuses `:0` and `:0.0`:

```
tools/unit-xvfb --display :91 run --platform windows \
    --cc x86_64-w64-mingw32-gcc --wine /usr/local/bin/wine64
tools/unit-xvfb --display :91 --watcom /opt/watcom turbo \
    --generated build/unit-turbo-generated \
    --stage build/unit-turbo --dosbox /usr/local/bin/dosbox \
    --dosbox-config tools/turboc/dosbox.conf
```

The optional `--watcom ROOT` argument exports `WATCOM` and prepends its
`binl` directory before dispatch. If it is omitted, the wrapper honors an
existing `WATCOM` value or uses `$HOME/watcom` when present.

The runner gives MinGW outputs their required `.exe` suffix. After compiling
the selected cases, it runs every isolated executable synchronously from one
`cmd.exe` process and retains a separate output, failure marker, and portable
coverage report for each case. Omit `--wine` on a native Windows runner.
LLVM line and MC/DC coverage is executed for Windows targets only on a native
Windows host with the target profiling runtime installed. Targets without that
runtime, and cross-host MinGW/Wine runs, retain the portable coverage oracle.
`--windows-architecture` also applies to test-only DLL fixtures, so an x86
isolated executable never accidentally loads an x64 `DOOR32.DLL` or vice
versa. Cross-host MinGW uses the default GNU Windows ABI; native Windows Clang
uses `--windows-abi msvc` so analysis uses the same SDK and calling convention
as execution.

Long cross-host runs may be resumed after an interruption with
`--start-at SOURCE:FUNCTION`. The named manifest entry is included and every
following entry applicable to the selected platform is run. This is an
explicit manifest boundary, not an artifact cache: the runner neither assumes
that an existing executable passed nor treats an old coverage report as
current.

## Test-driven changes

For a behavior change or defect fix, change the smallest affected case first
and run that function's test. Confirm that it fails because the requested
behavior is not yet implemented; a compile failure or unrelated assertion is
not a valid red phase. Preserve the red-test command and concise failure
reason for the handoff. Change production code only after that observation,
then rerun the same command to green and run every suite selected for the
complete diff.

Pure refactors and documentation-only changes may not admit a meaningful new
failing test. In that case, run the existing affected tests before and after
the change and explicitly identify why the red phase was not applicable.

Open Watcom compile checks use the installed toolchain without placing object
files in the source tree:

```
WATCOM=$HOME/watcom PATH=$HOME/watcom/binl:$PATH \
  python3 unit/tools/run.py --platform dos16 --toolchain watcom16 \
    --compile-only
WATCOM=$HOME/watcom PATH=$HOME/watcom/binl:$PATH \
  python3 unit/tools/run.py --platform dos32 --toolchain watcom32r \
    --compile-only
```

Cases which require a resident real-mode service may name a test-only assembly
source in `dos_tsr_fixture`. The DOS runner assembles and links each distinct
fixture with Open Watcom, installs the resulting `.COM` program before the
selected cases, and keeps the fixture entirely outside production sources.
The analogous `turbo_tsr_fixture` setting provides the same facility to the
exact Turbo C runner.

For runtime coverage, run the same command without `--compile-only`, supply
`--dosbox`, and give DOSBox a private Xvfb display through `tools/unit-xvfb`.
The runner gives every case
deterministic 8.3 filenames, compiles the selected cases, executes all of them
in one DOSBox batch, and imports each portable coverage record. DOS/32 runs
copy the Open Watcom DOS/4GW runtime into the build directory automatically:

```
WATCOM=$HOME/watcom PATH=$HOME/watcom/binl:$PATH \
  tools/unit-xvfb --display :93 run \
    --platform dos16 --toolchain watcom16 \
    --dosbox "$(command -v dosbox)"
WATCOM=$HOME/watcom PATH=$HOME/watcom/binl:$PATH \
  tools/unit-xvfb --display :94 run \
    --platform dos32 --toolchain watcom32r \
    --dosbox "$(command -v dosbox)"
```

Open Watcom must not inherit `DISPLAY`; its tools assign a different historic
meaning to that variable. The runner removes it from compiler subprocesses
while leaving it available to DOSBox.

`unit/tools/turbo_generate.py` analyzes the exact Turbo C 2.01 preprocessor
path while using target-only body isolation to keep unrelated Borland assembly
out of Clang's parser. `unit/tools/turbo.py` stages those self-contained,
8.3-named sources beside Turbo C. A first DOSBox pass compiles each source to
assembly and applies the small Turbo-assembly normalizer. Host Open Watcom
`wasm` then creates OMF objects, avoiding DOS-hosted assembler termination
defects. A second DOSBox pass links with Turbo C, executes the tests, and
applies the same portable coverage gate. Manifest configuration defines and
undefines are applied to both analysis and compilation. The development
workflow obtains Turbo C through the pinned, license-conscious cache procedure
used by the regular Turbo build; neither compiler nor assembler is committed
to this repository. Case-local headers are embedded in generated units so no
host path is exposed to the DOS compiler.

When a complete isolated translation unit exceeds Turbo C's compiler memory,
its manifest entry may specify `turbo_shards` with a value from 2 through 15.
The case must use `UT_TURBO_SHARD` to divide only its test scenarios; a value
of zero must continue to select every scenario for all other compilers. The
Turbo runner compiles and executes every shard, combines their observations
against the single unchanged production-function model, and only then applies
the full portable branch and MC/DC gate. Sharding is not a coverage waiver and
must not exclude any scenario from the combined report.

## Test cases and mocks

Cases must be valid C89. Avoid variadic macros, declarations after statements,
designated initializers, compound literals, constructor-based registration,
and large automatic objects. Tests run with an empty mock-call log and fresh
failure state.

Writable file-scope objects referenced by the selected function are restored
before each case. Function-local static objects are registered when their
declarations are first reached and are restored before later cases. This keeps
production storage semantics within a case while preventing state leakage
between cases.

Default mocks fail immediately. Custom mocks should validate every meaningful
argument, record their stable test-local mock identifier with
`ut_mock_called()`, and make their result explicit. A custom mock may implement
a small deterministic substitute for a CRT operation, but the production
function must never reach the real dependency directly.

## Coverage contract

Every applicable configuration requires function execution, both outcomes of
every branch, and MC/DC for every compound decision. Host-compatible LLVM
builds additionally require every executable production source line. LLVM
19's coverage JSON is the independent C coverage oracle on Unix and native
Windows builds. That
LLVM copy retains the framework's function-local static-state
registration but leaves production Boolean expressions unmodified. If LLVM
declines to emit an MC/DC record for a supported source decision, the missing
record is itself a gated coverage result. Recompiling a Windows- or
DOS-modeled translation unit as host Unix would select different preprocessor
branches, so cross-target runs use
portable instrumentation executed by the actual target compiler instead.
Preprocessor directives, brace-only lines, and standalone `break;` statements
are not independent executable-line obligations; their controlling behavior
remains covered by the branch and MC/DC gates.
Modern Unix and Windows production code is compiled under the
repository's C99 contract; DOS analysis and every Turbo-compiled case remain
C89-compatible. MC/DC is gated on every target. Complete multiple-condition
coverage is also reported; short-circuited conditions remain marked as
unevaluated rather than being guessed.

A genuinely infeasible site may be listed in `coverage-waivers.json` only with
its exact function, platform, source range, range hash, kind, rationale, and
evidence. Waivers are visible in reports and become stale when the covered
source text changes. An LLM may propose a waiver, but every LLM proposal must
be recorded separately in `coverage-waiver-proposals.json`; proposals never
affect strict coverage results. Test authors and automation may add or improve
those proposal records. A proposal is not an approval. Only a human who
personally and manually edits the repository may approve a waiver by adding it to
`coverage-waivers.json`. An LLM must never promote, copy, move, or otherwise
change a proposal to approved under any circumstances. Human approval or an
instruction in conversation does not authorize the LLM to make that edit; the
human must make the repository change directly. The associated coverage gate
remains failing until that human-authored approval edit is present in the
repository. Validation rejects a proposal with any status other than
`proposed`, as well as a site present in both files.

For local development, `run.py` and `turbo.py` accept
`--allow-proposed-coverage-waivers`. This lets the rest of a selected suite
continue while leaving each accepted gap identified as a proposed waiver in
the coverage report; it does not turn the proposal into an approved waiver.
The manually dispatched development workflow exposes the same choice and
defaults it to off. Strict CI and release evidence must omit the option. The
release workflow runs a policy check which rejects any attempt to use it.

## Selection

`selector.py` selects individual functions only when old and new parses prove
that every changed hunk is wholly inside the same function body. Signature,
file-scope, preprocessor, added, deleted, or ambiguous changes select the whole
source. A header selects every source in whose current or previous transitive
local include closure it appears. Unit infrastructure and workflow changes
select the complete suite. Any analysis failure must broaden selection rather
than skip tests.

Changes to `unit/tests.json` select the union of changed old and new test
owners. Changes to a source record in `unit/sources.json` select that complete
current source. `unit/inventory.json` is generated from the production sources
and ownership manifests, so its line-number and signature churn does not by
itself select runtime tests; the mandatory inventory check still rejects a
stale or manually inconsistent file. Schema, runner, selector, framework, and
workflow changes remain complete-suite changes.

The selector writes exact `(source, function)` owners. Both `run.py` and
`turbo_generate.py` accept that document with `--selection`; they do not turn
its source and function sets into a cross-product. Shared support beneath one
`unit/cases/<source>/` directory selects every case owned by that production
source, while changing an individual case `.c` selects only its named owner.
A changed Windows DLL or DOS TSR fixture selects every configuration which
names that fixture. The resulting registered owner/platform pairs construct
the Unix and Watcom matrices, so a DOS16-only or DOS32-only owner does not
launch empty jobs for other platform variants.

`.github/workflows/unit-tests.yml` runs the selected owners on pushes and pull
requests. Manual dispatch defaults to the complete suite and may explicitly
enable proposed-waiver development mode to expose failures beyond known gaps.
The same workflow is reusable; the release workflow calls it with full
selection and without proposed-waiver mode. Inventory validation is strict in
both workflows. A release therefore requires every applicable case and every
branch/MC/DC gate on Ubuntu and macOS Unix, Windows, Watcom DOS16, both Watcom
DOS32 calling conventions, and exact Turbo C 2.01.
