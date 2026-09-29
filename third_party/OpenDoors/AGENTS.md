# OpenDoors development instructions

OpenDoors supports current systems as well as Turbo C 2.01 and 16-bit DOS.
Preserve the public API and ABI unless the task explicitly authorizes a
compatibility break. Do not add test hooks, unit-test conditionals, or mock
support to production sources or public headers.

When changing a function, review that function's entry in `unit/tests.json`
and its case beneath `unit/cases/`. A change to behavior, parameters, return
values, branches, called functions, global-state use, or platform guards will
normally require corresponding test and mock-expectation changes. Do not
blindly update an assertion merely to accept new output; verify the intended
contract first.

Use test-driven development for behavior changes and defect fixes. Add or
update the smallest affected unit test before editing production code, run it,
and verify that it fails for the intended missing or incorrect behavior. Only
then change the production implementation and rerun that same test to verify
it passes, followed by the complete selector-reported suite. Do not combine
the red-test and production changes before observing the expected failure.
Record the red-test command and failure reason in the change handoff. A pure
refactor or documentation-only change may have no meaningful new red test;
run the existing affected tests before and after it and state why the red
phase did not apply.

New functions require isolated unit tests on every applicable platform.
Renamed or removed functions require regenerating `unit/inventory.json` and
updating test ownership. Adding or removing a first-party library source in
either CMake build manifest also requires updating `unit/sources.json`; the
strict checker requires the manifests to agree exactly. Header changes require
running every source suite reported by the unit selector. If a preprocessor
variant applies only to some registered platforms, scope its named
configuration with the `platforms` field while leaving at least one runnable
configuration for every owner platform. Test and mock code compiled by a DOS
toolchain must remain C89-compatible and respect 16-bit segment and stack
limits.

Before submitting a production change, run the selector against the working
tree and execute the suites it reports. At minimum, also run:

```
PYTHONDONTWRITEBYTECODE=1 python3 unit/tools/inventory.py --check
PYTHONDONTWRITEBYTECODE=1 python3 unit/tools/check.py
```

See `unit/README.md` for the complete workflow and coverage requirements.

An LLM may propose a coverage waiver, but every such proposal must be recorded
in `unit/coverage-waiver-proposals.json`. A proposal is not an approval. Only a
human who personally and manually edits the repository may approve a waiver by
adding it to `unit/coverage-waivers.json`. An LLM must never add a proposal to
that file, move or copy one there, or otherwise change a waiver from proposed
to approved under any circumstances. A human statement or instruction in chat
does not authorize the LLM to make the approval edit; the human must make that
repository change directly. Until the human-authored edit exists in the
working tree, the affected coverage gate must remain failing; do not weaken,
bypass, or silently reinterpret it. The explicitly requested
`--allow-proposed-coverage-waivers` development run may continue through such
a gap, but is not strict coverage evidence and must never be used by a release
job.
