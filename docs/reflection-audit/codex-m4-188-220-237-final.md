Investigated all three; no fixes claimed.

- #220: reproduces hang beyond `timeout 20`; traced to undeduced `AutoType` recursion through `type_of` → printer → `return_type_of`.
- #237: reproduces `'auto' not allowed in type alias`; wording supports closure-type splicing, so it remains a real deeper alias/type gap. [splice wording](https://eel.is/c%2B%2Bdraft/dcl.type.splice)
- #188: exact reproducer rejects five cases: `1.2`, `2.2`, `2.3`, `3.3`, `4.3`; failure bottoms out at printer `reflect_invoke`.

Updated tracker and added [audit report](/home/spawn/dev/toolchains/clang-p2996/docs/reflection-audit/codex-m4-188-220-237-report.md). No full post-fix suites were run because no fix was made.

Commit/staging failed: `.git/index` is read-only. Changes remain unstaged.