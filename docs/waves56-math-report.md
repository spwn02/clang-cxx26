# Wave 56 math implementation report

## Scope and sources

Issue #118 implements the remaining P1383R2 constexpr transcendental support
tracked in `docs/CXX26_GAPS.md`. The official paper is
https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p1383r2.pdf.

MPFR 4.2.2 is installed locally (`pkg-config --modversion mpfr`). MPFR/GMP are
external build dependencies and are not bundled or vendored. The Clang AST
CMake directory explicitly discovers `mpfr.h` and `libmpfr`; a configured Clang
build must provide both, plus GMP headers and library because the evaluator calls
GMP symbols directly. MPFR is LGPL-3.0-or-later; GMP is available under LGPL-3.0-or-later
or GPL-2.0-or-later. These libraries remain external dynamic dependencies and
are not bundled or vendored. This changes no public ABI or LLVM provenance.

## Implementation

- Added APFloat to MPFR transfer through an integer significand and binary
  exponent, and reverse transfer via MPFR's exact integer/exponent pair. No
  host `double` or `long double` conversion is used.
- Added evaluator dispatch for all P1383 scalar additions: sqrt, cbrt, pow,
  exp/exp2/expm1, log/log10/log1p/log2, trig, inverse trig, hyperbolic,
  inverse hyperbolic, atan2, hypot, erf, erfc, lgamma, and tgamma.
- Added constexpr annotations to matching libc++ wrappers and constexpr-only
  builtin aliases. Runtime calls still invoke their existing builtins.
- Added compiler and libc++ regression inputs for ordinary values, exceptional
  results, signed zero, subnormals, large trig arguments, and invalid domains.
- `__cpp_lib_constexpr_cmath` remains disabled: P0533R9 also covers declarations
  outside this P1383 subset, and its full constexpr declaration set has not been
  demonstrated here.

## Validation status and limits

The root integration build and focused tests now pass. The adaptive directed
lower/upper MPFR bounds accept a result only when both endpoints round to the
same APFloat value under the active rounding mode; unresolved inputs are
rejected after eight precision doublings. Domain and pole checks reject invalid
real-domain inputs during constant evaluation while runtime libm calls remain
unchanged. The implementation transfers target values through integer
significands and binary exponents, covering float, double, long double,
_Float16, and __float128 builtin formats without host floating intermediates.

The P0533 feature macro remains disabled because the complete declaration set
has not been demonstrated. Target-specific fenv and errno behavior is outside
the constexpr evaluator tests. These limits mean #118 remains open.

## Tests and validation

MPFR 4.2.2 was confirmed. The focused MPFR, exact-math, feature-macro,
builtin-alias, and CodeGen tests pass (12/12). The full Clang archive recorded
44,707 PASS, 25 XFAIL, 5,171 UNSUPPORTED and four failures. Comparison with the
archived baseline found no new compiler failures; three new failures were
Python 3.14 lit helper processes denied forkserver sockets by the sandbox, and
the remaining failure was the known consteval propagation baseline.

The complete libc++ suite excluding executing benchmarks passed 11,497 tests,
with 26 XFAIL and 1,089 UNSUPPORTED. Its only three new failures compared with
the archive were sandbox-denied GDB ptrace and Unix socket binds. Focused
libc++ validation passed 133/133 tests, including the C++23 exact-math reference
and C++26 transcendental suite. The archived candidate is
`/tmp/waves56-candidate/results/check-cxx-20260929T153254Z-a7ca98e7709d-post-clean-clang-rebuild.json`.

`__cpp_lib_constexpr_cmath` remains disabled. No tracker status flip is
recorded until remaining declaration/format edge review and the issue's full
acceptance criteria are satisfied. #116 remains untouched.
