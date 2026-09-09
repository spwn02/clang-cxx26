# Reflection final `check-clang` gate

Date: 2026-09-09  
HEAD: `501497f1ec181dfe88426e7cec25ad9d0fb42f58`

## Result

The authoritative clean run completed with no OOM interruption and found exactly
five failing tests. All five are the declared pre-existing baseline. There are
no failures beyond baseline and no regression attributable to this session's
`reflection:` commits.

The lit terminal summary reported 49,820 discovered tests. The JSON result
contained 49,816 result records:

| Result | Count |
|---|---:|
| Passed | 44,615 |
| Failed | 5 |
| Expectedly failed | 25 |
| Unsupported | 5,171 |
| Unresolved | 0 |

## Exact failures

1. `Clang :: SemaCXX/PR98671.cpp`
2. `Clang :: SemaCXX/builtin-is-within-lifetime.cpp`
3. `Clang :: SemaCXX/constant-expression-cxx11.cpp`
4. `Clang :: SemaCXX/cxx2a-constexpr-dynalloc.cpp`
5. `Clang :: SemaCXX/cxx2b-consteval-propagate.cpp`

These match the documented consteval-escalation / PR98671 baseline and were
not changed by this session.

## Verification details

- Removed all `build-nyx/tools/clang/test/**/Output` scratch directories using
  `command find` to bypass the shell's wrapped `find`.
- Rebuilt stale test consumers individually, including `c-index-test`,
  `clang-extdef-mapping`, `clang-scan-deps`, `clang-import-test`, `clang-repl`,
  and `clang-check`.
- Discovered a further stale primary driver: `clang-22` embedded
  `7728197f...` while consumers had current-HEAD metadata. Rebuilt `clang`;
  all tested binaries then embedded `501497f1...`.
- The final run was:
  `build-nyx/bin/llvm-lit -j4 -q -o /tmp/clang-gate-final.json clang/test`
- No compiler or library source was modified. Only build artifacts and this
  audit/tracker documentation changed.
