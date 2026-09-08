Root cause: rebuild/invocation artifact, not `ItaniumMangle.cpp`.

Evidence from [failure log](/home/spawn/dev/toolchains/clang-p2996/docs/reflection-audit/check-clang-post-mangling-fix.log):

- 308 failures were caused by missing/stale tools:
  - `clang-check`, `clang-extdef-mapping`, `clang-import-test`
  - `clang-scan-deps`, `opt`, LTO tools, etc.
- `clang-extdef-mapping` showed an undefined Clang ABI symbol.
- Manual invocation lacked `build-nyx/bin` on `PATH`, breaking bare tool substitutions.
- Rebuilt binaries initially had mismatched embedded repository revisions, invalidating PCH/PCM files.

Actions:

- Queried `tools/clang/test/check-clang` dependencies.
- Rebuilt missing tool targets with `-j2`.
- Rebuilt `clang` with `-j2`.
- Rebuilt stale `c-index-test` with `-j2`.
- Removed only generated `build-nyx/tools/clang/test/**/Output` directories.
- Reran full suite with `PATH=build-nyx/bin:$PATH`, `CLANG=.../bin/clang`, and `-j4`.

Final result:

- 49,818 tests discovered
- 44,613 passed
- 5 failed, exactly the known baseline:
  - `PR98671.cpp`
  - `builtin-is-within-lifetime.cpp`
  - `constant-expression-cxx11.cpp`
  - `cxx2a-constexpr-dynalloc.cpp`
  - `cxx2b-consteval-propagate.cpp`
- Zero `Tooling/*` failures
- Zero other new failures
- No compiler source files modified by this debugging work.