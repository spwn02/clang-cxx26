Completed and pushed commit `9a364e1f6768`.

- P2996R13: `display_string_of(null)` is a genuine gap because result must be non-empty; recorded as `NEW-1`. `source_location_of(null)` is conformant fallback behavior and marked Not-Applicable. ([paper wording](https://wg21.link/P2996R13))
- Added P1306R5 batch covering rows 1306-01 through 1306-05.
- Focused lit test: 1/1 passed with all diagnostics matched.
- Counts: 28 Covered, 62 Needs-New-Test, 18 Blocked, 1 Not-Applicable.
- Report: [codex-m5-batch4-report.md](/home/spawn/dev/toolchains/clang-p2996/docs/reflection-audit/codex-m5-batch4-report.md)
- Working tree clean; `origin/cxx26..HEAD` is empty.