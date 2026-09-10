Completed design investigation; no source fix attempted.

Findings:

- PR #261 (`1fdd67b76362`) proposes broad deferred expansion-body instantiation and remains applicable in concept.
- It does not fix #181’s independent tuple/reference-binding defect.
- #180’s published reproducer is reflected `substitute` plus `static_assert(false)`, not literally an expansion-body assertion.
- P1306R5 requires independent per-iteration instantiation and preservation of reference/value category.

Updated:

- [codex-m4-180-181-report.md](/home/spawn/dev/toolchains/clang-p2996/docs/reflection-audit/codex-m4-180-181-report.md)
- [REFLECTION_GAPS.md](/home/spawn/dev/toolchains/clang-p2996/docs/REFLECTION_GAPS.md)

No tests/build run because no implementation changed. Commit/push blocked: `.git` is read-only. PR references: [#261](https://github.com/bloomberg/clang-p2996/pull/261), [#180](https://github.com/bloomberg/clang-p2996/issues/180), [#181](https://github.com/bloomberg/clang-p2996/issues/181), [P1306R5](https://wg21.link/P1306R5).