# Item 10 — P3560R2 `3560-18` fix report

Date: 2026-09-11

## Result

Fixed. This was a general C++26 P3068 constexpr-exceptions bug, separate from
item 9's compiler-originated `meta::exception` construction problem.

`EvaluateVarDecl` creates addressable storage and registers its block cleanup
before evaluating a local initializer. If that initializer throws, the APValue
is absent because the local's lifetime never began. The cleanup remained on the
enclosing block's cleanup stack, so exception unwinding attempted destruction
and `HandleDestructionImpl` correctly rejected it with
`note_constexpr_destroy_out_of_lifetime`.

`EvalInfo::cancelCleanup` now removes the cleanup associated with an initializer
that failed. The operation is idempotent because some ordinary failed-initializer
paths have already unwound their cleanup before `EvaluateVarDecl` returns. This
does not skip destruction of constructed locals: their cleanups remain and run
during P3068 unwinding as before.

## Regression coverage

* `clang/test/SemaCXX/constexpr-p3068r6-throw.cpp` now contains the minimal,
  non-reflection `Widget` / throwing initializer / catch test.
* `libcxx/test/std/experimental/reflection/exception.pass.cpp` now catches the
  exception from `access_context::current().via(^^int)` and inspects `what()`,
  `from()`, and `where()`.

## Commands and results

* `ninja -C build-nyx -j$(nproc) clang` — passed after the source change.
* `./build-nyx/bin/llvm-lit -v clang/test/SemaCXX/constexpr-p3068r6-throw.cpp`
  — passed (1/1).
* `ninja -C build-libcxx -t clean cxx && ninja -C build-libcxx -j$(nproc) cxx`
  — initially hit the sandbox's read-only `ccache`; rerun with ccache write
  access completed successfully after the idempotence adjustment.
* `libcxx/utils/libcxx-lit build-libcxx -sv
  libcxx/test/std/experimental/reflection/exception.pass.cpp` — passed (1/1).
* `libcxx/utils/libcxx-lit build-libcxx -s
  libcxx/test/std/experimental/reflection/` — 114 tests; exactly 7 failures,
  all documented pre-existing baseline warning/verify mismatches:
  `attributed-function-type-queries.pass.cpp`, `entity-proxies.pass.cpp`,
  `entity-proxy-member-queries.pass.cpp`,
  `namespace-reflection-equality-reopened.pass.cpp`,
  `m5-p2996-batch13.verify.cpp`, `m5-p2996-p3096-batch12.verify.cpp`, and
  `m5-p3491-batch15.verify.cpp`.
* `./build-nyx/bin/llvm-lit -v clang/test/Reflection/ clang/test/Parser/
  clang/test/SemaCXX/ clang/test/SemaTemplate/ clang/test/AST/
  clang/test/CodeGenCXX/` — 3836 tests: 3784 passed, 7 expected failures,
  40 unsupported, and exactly the documented 5 SemaCXX baseline failures:
  `PR98671.cpp`, `builtin-is-within-lifetime.cpp`,
  `constant-expression-cxx11.cpp`, `cxx2a-constexpr-dynalloc.cpp`, and
  `cxx2b-consteval-propagate.cpp`.

No commit or push was performed.
