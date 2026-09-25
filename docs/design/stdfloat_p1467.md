# P1467R9 extended floating-point types (`<stdfloat>`) — design note (#50)

Status: **plan only**, nothing implemented. Probed 2026-09-25 against HEAD `b1811f61aca4`, target
`x86_64-unknown-linux-gnu`. Primary source: https://wg21.link/P1467R9 (adopted C++23, still listed as
unimplemented by the fork's `docs/CXX26_GAPS.md`).

## What the paper requires

- Core: extended floating-point types are distinct types with a conversion *rank* (value-set
  inclusion) and *subrank*; `std::float16_t/32/64/128_t`, `std::bfloat16_t` are the IEEE/bfloat formats when the
  implementation supports them. Usual arithmetic conversions of operands whose ranks are unordered are
  ill-formed. Conditionally supported literal suffixes `f16 f32 f64 f128 bf16`.
  Predefined macros `__STDCPP_FLOAT16_T__`, `__STDCPP_FLOAT32_T__`, `__STDCPP_FLOAT64_T__`,
  `__STDCPP_FLOAT128_T__`, `__STDCPP_BFLOAT16_T__` (1 when supported).
- Library: `<stdfloat>` (aliases, and `__cpp_lib_stdfloat` — confirm the exact value in the current draft),
  `numeric_limits`, `is_floating_point`/`floating_point`, `<cmath>` overloads, `<complex>`, `<charconv>`,
  `<atomic>`/`atomic_ref`, `hash`, `<ostream>`/`<istream>` for rank <= `long double`, `<format>` (no wording
  change needed: it works through `is_arithmetic`-style dispatch, but the fork's formatter is keyed on the exact
  standard types, see below).

## Current behaviour (repros run on HEAD, `-std=c++26`)

Language: Clang already has `_Float16` (suffix `f16`, mangling `DF16_`) and `__bf16` (mangling `DF16b`).

| Probe | Result |
|---|---|
| `_Float32 x; _Float64 y; _Float128 z;` | unknown type name (all three) |
| `1.0bf16`, `1.0f32`, `1.0f64`, `1.0f128` | invalid suffix (`1.0f16` works) |
| `__STDCPP_FLOAT16_T__` etc. | none of the five macros defined |
| `int f(float); int f(double); f((_Float16)1)` | **ambiguous** (also for `__bf16`) |
| `(_Float16)1 + (__bf16)1` | accepted, type `_Float16`; the paper says unordered ranks are ill-formed |
| `__bf16 a,b; a+b` | type is `__bf16` (arithmetic works, stays in type) |

Library (built libc++, `-std=c++26`, `x = _Float16`):

| Probe | Result |
|---|---|
| `std::numeric_limits<_Float16>::is_specialized` | false |
| `std::is_floating_point_v<_Float16>`, `<__bf16>`, `std::floating_point<_Float16>` | false |
| `std::sqrt(x)`, `std::abs(x)`, `std::floor(x)` | ambiguous call |
| `std::to_chars(b, b+32, x)` | ambiguous call |
| `std::format("{}", x)` | `formatter<_Float16>` is implicitly deleted |
| `std::hash<_Float16>{}(x)` | deleted `__hash_impl` |
| `std::complex<_Float16>`, `std::atomic<_Float16>` | compile, but as the generic primary templates (no floating-point `fetch_add`, wrong `complex` semantics) |
| `#include <stdfloat>` | file does not exist; `import std;` has no `std::float16_t` |

## Touch points

Compiler (`clang/`)
1. New `BuiltinType` kinds for `_Float32`, `_Float64`, `_Float128` (`BuiltinTypes.def`, `ASTContext.cpp`
   float-semantic tables, `TargetInfo` hooks; `Float128Ty` and `__float128` already exist, so `_Float128` is mostly a
   spelling/identity question; `float32_t` must be a *distinct* type from `float`). Itanium mangling
   `DF32_`, `DF64_`, `DF128_` (`ItaniumMangle.cpp`), plus serialization and `TypeLoc`/`ASTNodeTraverser` boilerplate.
2. Literal suffixes in `LiteralSupport.cpp`/`Lexer` (`bf16`, `f32`, `f64`, `f128`), gated on availability.
3. Conversion rank/subrank: `ASTContext::getFloatingTypeOrder` (and `getFloatingTypeSemanticOrder`) return a total
   order today; needs an "unordered" outcome, with `Sema::UsualArithmeticConversions` diagnosing it
   (`_Float16` vs `__bf16`). `SemaOverload.cpp` `compareStandardConversionSequences` needs the paper's tie-break so
   `f(float)` vs `f(double)` with a `float16_t` argument is decided per [over.ics.rank] (check wording; probe above shows the current
   ambiguity).
4. Predefined macros in `InitPreprocessor.cpp`, keyed on target support (`TargetInfo::hasFloat16Type()`,
   `hasBFloat16Type()`, `hasFloat128Type()`; x86-64 has all of them, with `_Float16` needing SSE2 in software mode).
5. Codegen/ABI: `_Float16` on x86 is currently promoted through `half`; confirm the calling convention
   choice (GCC passes `_Float16` in XMM registers since GCC 12 on x86-64 psABI) before exposing it as `std::float16_t`.
6. Optional: `-Wpre-c++23-compat`-style extension warnings for the suffixes.

Library (`libcxx/`)
1. New `libcxx/include/stdfloat` (aliases guarded on the macros), `libcxx/include/module.modulemap.in` entry,
   `libcxx/modules/std/stdfloat.inc` + `std.compat` as needed, `version` FTM via
   `libcxx/utils/generate_feature_test_macro_components.py`, `libcxx/docs/Status/Cxx23Papers.csv` row, and the
   standing "real `import std;` test" for the new exports.
2. `__type_traits/is_floating_point.h`, `numeric_limits` specializations (`__cpp_lib_stdfloat`),
   `__math/`-based `<cmath>` overloads (the `is_floating_point`-constrained templates + the `common_type`
   promotion path for mixed arguments), `<complex>` (paper removes the explicit specializations in favour of a
   constrained primary template: this is a wide edit and an ABI question for existing `complex<float>` users),
   `<charconv>` (`to_chars`/`from_chars`; needs a conversion through `float` with exact round-trip proof or a
   dedicated shortest-representation path), `formatter`/`__format` type dispatch, `hash`, `atomic`/`atomic_ref`
   floating-point specializations, `ostream`/`istream` inserters.

## Staged milestones

1. **Language, types and macros** (compiler only): `_Float32/_Float64/_Float128`, suffixes, macros, mangling,
   serialization, tests in `clang/test/{SemaCXX,CodeGenCXX,PCH}`; no library dependency.
2. **Conversion rules**: unordered ranks, subrank, overload tie-break; fixes the `_Float16` ambiguity above.
3. **Minimal library**: `<stdfloat>`, `is_floating_point`, `numeric_limits`, `hash`; modulemap/`import std` exports.
4. **Math/charconv/format/complex/atomic/iostream** in that order of dependence; one paper-row per sub-facility in the CSV.

## Test plan

Paper-canonical: `static_assert(!is_same_v<float32_t, float>)`; rank ordering table; `sizeof`/`numeric_limits` per type; unordered mixed
arithmetic must be rejected; `sqrt(float16_t)` returns `float16_t`; `to_chars` round trips; `format("{}", 1.5f16)`;
`import std;` test naming each alias. Assertion build for clang tests, then the full libcxx lit run (the `check-cxx`
baseline) because `is_floating_point` and `<cmath>` overloads are include-order sensitive.

## Risks and unknowns

- ABI: mangling and calling convention for `_Float16` on each supported target; `float128_t` must not collide with
  `__float128`/`long double` on ppc64 (long double is IBM double-double).
- `<complex>` restructuring is the single largest library edit and needs an ABI review.
- Target availability drives macro values; every branch must be testable on the CI target set, not just x86-64 Linux.
- Overload-resolution change touches every `<cmath>` call; expect regressions in the transitive-includes and
  libc++ `check-cxx` baselines.

Effort: milestone 1-2 is a substantial compiler change (about the size of the `_Float16` bring-up); milestone 3-4 is
mostly mechanical but wide. Roughly four to six focused sessions in total.
