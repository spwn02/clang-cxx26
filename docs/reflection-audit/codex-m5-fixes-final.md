Implemented and directly verified:

- NEW-4: closure-type rejection.
- NEW-6: constructor-splice rejection; destructor path unchanged.
- NEW-2: annotation on empty declaration rejected.
- NEW-3: incomplete annotation filter type rejected.

Deferred NEW-7 due lack of a safely scoped rejection point.

NEW-8 was reclassified: [P3096R12](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3096r12.pdf) does not restrict all three queries to parameters.

Verification:

- Full Clang gate: 23 failures, exactly documented baseline.
- libc++ gate: 6 executed tests passed; 11,826 skipped by configuration.

Report: [codex-m5-fixes-report.md](/home/spawn/dev/toolchains/clang-p2996/docs/reflection-audit/codex-m5-fixes-report.md)

Commits/pushes were blocked: `.git` is read-only (`Unable to create .git/index.lock`). Changes remain in the working tree.