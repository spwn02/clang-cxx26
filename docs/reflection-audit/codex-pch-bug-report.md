# c-index-test PCH loading audit

Date: 2026-09-09

## Result

The failure is pre-existing, not a regression from the Reflection Closure Epic. The exact
reproducer fails with current HEAD, while the PCH is accepted by the compiler's `-include-pch`
path. No commit since pre-epic baseline `501497f1ec18` changes `ASTUnit`, `ASTReader`,
`ModuleManager`, `c-index-test`, libclang, or PCH serialization. Today's AST/Sema changes are
limited to reflection implementation files.

## Diagnostic path

`clang/tools/c-index-test/core_main.cpp:284-291` calls `ASTUnit::LoadFromASTFile`. The generic
message comes from `clang/lib/Frontend/ASTUnit.cpp`: both initial
`ASTReader::readASTFileControlBlock` failure and later `ASTReader::ReadAST` failure report
`diag::err_fe_unable_to_load_ast_file`.

Temporary instrumentation at both return points showed that this reproducer fails in the first
path: `readASTFileControlBlock` returns `true`, so `ReadAST` is never reached. That helper has
multiple intentional error-swallowing paths while scanning options/control and extension blocks;
its `ReadOptionsBlock` and final `readUnhashedControlBlockImpl` results are collapsed to a
boolean. `c-index-test` passes `CaptureDiagsKind::None`, so no stored ASTUnit diagnostic is
available. The temporary instrumentation was removed.

Therefore “unable to load precompiled file” is not evidence of a stale PCH here. It is the generic
ASTUnit wrapper for a control-block read/validation failure whose underlying reason is discarded
by the existing ASTReader API.

## Reproduction

With current HEAD and freshly rebuilt consumer tools:

```text
clang -cc1 ... -emit-pch clang/test/Index/Core/index-pch.c -o /tmp/repro.pch
# exit 0
clang -cc1 ... -include-pch /tmp/repro.pch -fsyntax-only /tmp/main.c
# exit 0
c-index-test core -print-source-symbols -module-file /tmp/repro.pch
# error: unable to load precompiled file
# failed to create TU for: /tmp/repro.pch
```

The auxiliary binaries were newer than `clang-22` during the check and were rebuilt; the failure
remained. `LIBCLANG_DISABLE_PCH_VALIDATION=1` also did not change the failure, confirming that it
happens before ASTReader's normal module validation path.

## Historical classification

The requested pre-epic checkout was `501497f1ec18`, the last commit before 2026-09-09 reflection
work. Source history shows no PCH-consumer or serialization changes between that commit and HEAD.
The bug is consequently part of the pre-existing baseline and must not be counted as a Reflection
Closure regression. The approximately 18 Index/Core, ClangScanDeps, Interpreter, Tooling, and
Analysis failures are repeated manifestations of this one ASTUnit/libclang consumer issue.

No compiler fix is included. A future standalone fix should preserve/report the `llvm::Error` or
diagnostic category returned by control-block scanning, then add a focused ASTUnit/c-index-test
regression test.
