# OpenDoors unit-test maintenance

- Review and normally update a function's isolated unit tests whenever its
  behavior, signature, control flow, dependencies, global-state access, or
  platform guards change.
- Use TDD for behavior changes and fixes: modify the smallest affected unit
  test first, run it, and confirm that it fails for the intended reason before
  changing production code. Then make the implementation pass that test and
  run the complete selector-reported suite. Report the red-test command and
  failure reason. For pure refactors or documentation-only changes, run the
  existing affected tests before and after and explain why no meaningful red
  phase applied.
- Add tests for new functions and update `unit/inventory.json` for additions,
  removals, and renames.
- Keep `unit/sources.json` exactly synchronized with first-party library
  sources in the modern and DOS CMake build manifests. Scope a named
  preprocessor configuration with its `platforms` field when it applies only
  to a subset, and never leave a registered owner platform without a runnable
  configuration.
- Never add unit-test hooks or mock conditionals to production code.
- Keep generated mocks and all DOS-compiled tests valid C89 and safe for a
  16-bit large-memory-model build.
- Run the affected suites selected by `unit/tools/selector.py`; header changes
  may select multiple complete source suites.
- Full branch and MC/DC coverage is required. Coverage waivers must be explicit
  and reviewable. An LLM may propose a waiver, but must record every proposal
  in `unit/coverage-waiver-proposals.json`; a proposal is not an approval. Only
  a human who personally and manually edits the repository may add an approved
  waiver to `unit/coverage-waivers.json`. An LLM must never promote, copy,
  move, or otherwise change a proposal to approved under any circumstances.
  Human approval or an instruction in chat does not authorize the LLM to make
  that edit. Leave the affected coverage gate failing until the human-authored
  approval edit is present in the working tree. The explicitly requested
  `--allow-proposed-coverage-waivers` development mode may continue through a
  proposal, but is not strict coverage evidence and must never be used for a
  release.
