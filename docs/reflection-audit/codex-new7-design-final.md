Implemented and verified NEW-7.

- Added narrow CTAD-like rejection in [`SemaDecl.cpp`](/home/spawn/dev/toolchains/clang-p2996/clang/lib/Sema/SemaDecl.cpp:14008).
- Added focused negative/positive test: [`new-7-dependent-splice.verify.cpp`](/home/spawn/dev/toolchains/clang-p2996/libcxx/test/std/experimental/reflection/new-7-dependent-splice.verify.cpp).
- Updated tracker/checklist and batch-13 coverage.
- Wrote [`codex-new7-design-report.md`](/home/spawn/dev/toolchains/clang-p2996/docs/reflection-audit/codex-new7-design-report.md), including adopted [P2996R13](https://wg21.link/P2996R13) wording and implementation trace.
- Clang build passed.
- Focused verify test passed.
- Full libc++ reflection suite: 101 passed, 1 unsupported, 6 documented baseline failures; no new failures.

Commit/push blocked: `.git` is read-only. Changes remain in working tree.