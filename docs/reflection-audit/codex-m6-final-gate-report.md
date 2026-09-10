# Reflection Closure Epic M6 Final-Gate Report

Date: 2026-09-10
Branch: `cxx26`
HEAD: `f8fd8305fe14`

## Verdict

**M6 BLOCKED.** The assertions-enabled clang toolchain rebuilt successfully and the complete
clang test tree ran to completion. It produced five failures, exactly the documented
consteval-escalation/Sema baseline. The documented 18 ASTUnit/libclang PCH manifestations did
not appear in this fresh consumer-tool rebuild.

The full libc++ gate did not complete. Its observed failures are exactly the seven documented
reflection-suite baseline failures, but the run stalled early in the non-reflection suite after
the filesystem-pressure retry. No additional completed libc++ failures were identified. M7
close-out is therefore not justified until full `libcxx/test` completes and its complete result
list is archived.

## Commands and environment

Ground Truth in `docs/REFLECTION_GAPS.md` was read in full before work began. The tree was clean.
`build-nyx` has assertions enabled. Initial available memory was 8.8 GiB; root filesystem had
8.0 GiB free. The configured ccache directory was read-only, so all build commands used the
writable task-local `CCACHE_DIR=/tmp/codex-m6-ccache`.

Commands run:

```text
ninja -C build-nyx clang -j4
ninja -C build-nyx check-clang -j1                         # ccache read-only; failed before tests
CCACHE_DIR=/tmp/codex-m6-ccache ninja -C build-nyx check-clang -j1
CCACHE_DIR=/tmp/codex-m6-ccache ninja -C build-nyx check-clang -j4
build-nyx/bin/llvm-lit -j1 -sv build-nyx/tools/clang/test # Python 3.14 forkserver blocked
python3.13 build-nyx/bin/llvm-lit -j1 -sv build-nyx/tools/clang/test # interrupted for throughput
python3.13 build-nyx/bin/llvm-lit -j8 -sv build-nyx/tools/clang/test # interrupted for throughput
python3.13 build-nyx/bin/llvm-lit -j22 -sv build-nyx/tools/clang/test
python3.13 build-nyx/bin/llvm-lit -j22 -sv build-nyx/tools/clang/test # final corrected run
ninja -C build-nyx check-cxx -j1                         # target does not exist
CCACHE_DIR=/tmp/codex-m6-ccache libcxx/utils/libcxx-lit build-libcxx -sv -j22 libcxx/test
CCACHE_DIR=/tmp/codex-m6-ccache libcxx/utils/libcxx-lit build-libcxx -sv -j4 libcxx/test
```

Python 3.14's default `forkserver` cannot bind its listener in this sandbox. Generated build
artifacts were temporarily pointed at `/usr/bin/python3.13`, whose default start method is
`fork`; no repository source was changed.

## check-clang result

Final corrected run: 49,850 discovered; 44,643 passed; 5,171 unsupported; 6 skipped; 25
expectedly failed; 5 failed. Testing time was 201.62 seconds.

Complete failed-test list:

```text
Clang :: SemaCXX/PR98671.cpp
Clang :: SemaCXX/builtin-is-within-lifetime.cpp
Clang :: SemaCXX/constant-expression-cxx11.cpp
Clang :: SemaCXX/cxx2a-constexpr-dynalloc.cpp
Clang :: SemaCXX/cxx2b-consteval-propagate.cpp
```

These are the five documented consteval-escalation baseline tests. The separate 18-test
ASTUnit/libclang PCH cluster documented in `codex-pch-bug-report.md` was not emitted by this
fresh rebuilt run; its pre-existing status remains established by the exact stash-isolated
reproducer in that report. The three Python helper failures seen in the first Python 3.13 run
were environment failures caused by nested hard-coded Python 3.14 invocations, not compiler
failures; the corrected final run removed them.

## check-cxx result

`ninja -C build-nyx check-cxx` is not configured in this two-tree checkout (`unknown target`).
The required libc++ wrapper discovered 11,834 tests. The first `-j22` run eventually exhausted
the root filesystem while generating module PCM files (`No space left on device`) after the
generated module-output directory grew to 25 GiB. That disposable directory was removed, freeing
25 GiB, and the full wrapper was retried at `-j4`.

Observed failed tests in the `-j4` retry (the run was stopped after several minutes without
progress beyond this early cluster):

```text
llvm-libc++-shared.cfg.in :: std/experimental/reflection/m5-p2996-p3096-batch12.verify.cpp
llvm-libc++-shared.cfg.in :: std/experimental/reflection/entity-proxy-member-queries.pass.cpp
llvm-libc++-shared.cfg.in :: std/experimental/reflection/attributed-function-type-queries.pass.cpp
llvm-libc++-shared.cfg.in :: std/experimental/reflection/entity-proxies.pass.cpp
llvm-libc++-shared.cfg.in :: std/experimental/reflection/namespace-reflection-equality-reopened.pass.cpp
llvm-libc++-shared.cfg.in :: std/experimental/reflection/m5-p3491-batch15.verify.cpp
llvm-libc++-shared.cfg.in :: std/experimental/reflection/m5-p2996-batch13.verify.cpp
```

These are exactly the seven failures listed in Ground Truth. Because the full tree did not
complete, no claim is made about the remaining 11,827 tests.

## Isolation and regression assessment

No failure beyond the documented baselines completed in either gate, so no reflection-epic
commit was bisected and no source fix was attempted. The five clang failures match the already
documented consteval cluster. The 18-test PCH cluster's pre-existing classification is supported
by the earlier exact isolation report, including the pre-epic checkout `501497f1ec18`. The seven
libc++ reflection failures were previously stash-isolated on an unmodified checkout and match
that baseline here.

## Required follow-up

Rerun the full `libcxx/test` wrapper after ensuring adequate disk headroom and diagnosing the
early non-reflection stall. Preserve the complete failure list and compare non-reflection tests
against zero. Only then can M6 be marked complete.
