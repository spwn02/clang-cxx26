# Waves 5–6 freestanding implementation report (#23)

Implementation and focused audit are ready for the root integration gates. No
shared build tree was modified, no files were staged or committed, and no full
libc++ build/lit gate was run. Issue closure is not claimed here.

The audit used adopted paper wording from the official WG21 archive:
[P2198R7](https://open-std.org/JTC1/SC22/WG21/docs/papers/2023/p2198r7.html),
[P2338R4](https://open-std.org/JTC1/SC22/WG21/docs/papers/2023/p2338r4.html),
[P2013R5](https://open-std.org/JTC1/SC22/WG21/docs/papers/2023/p2013r5.html),
[P2407R5](https://open-std.org/JTC1/SC22/WG21/docs/papers/2023/p2407r5.html),
[P2937R0](https://open-std.org/JTC1/SC22/WG21/docs/papers/2023/p2937r0.html),
[P2833R2](https://open-std.org/JTC1/SC22/WG21/docs/papers/2023/p2833r2.html),
and [P2976R1](https://open-std.org/JTC1/SC22/WG21/docs/papers/2024/p2976r1.html).

## Implementation and declaration audit

- **P2198R7:** Registered required freestanding feature-test macros in the
generator and regenerated `<version>`, `FeatureTestMacroTable.rst`, and
per-header macro tests. Added `__cpp_lib_freestanding_operator_new` with the
required 0/202306L values. CMake sets 202306L only when libc++ or in-tree
libc++abi provides its replaceable allocation definitions, or when the
integrator explicitly confirms the selected external runtime with
`LIBCXX_FREESTANDING_OPERATOR_NEW`. Unknown runtimes conservatively report
zero. The `__config_site.in` substitution carries the configured value into
installed headers; unconfigured header-only use falls back to zero.
- **P2338R4:** Audited required declarations and added a no-exceptions compile
test for every integral `charconv` overload type; `char_traits` for
char/wchar_t/char8_t/char16_t/char32_t; required narrow and wide C string
functions; `bsearch`/`qsort`; integral `abs`, `labs`/`llabs`, `div`/`ldiv`/
`lldiv` and result types; `size_t`, `NULL`, exit status macros, required errno
macros, and `errc`. Hosted floating overloads remain available with hosted
semantics; no incomplete freestanding macro is advertised.
- **P2013R5:** Default libc++ and in-tree libc++abi replaceable allocation
definitions already use malloc/free-backed allocation and matching delete
functions; no ABI/runtime implementation change is needed in that
configuration. Added target-library configuration detection and an explicit
opt-in for externally configured runtimes. The regression test covers
scalar/array, sized, nothrow, and aligned allocation/deallocation forms when
the configured macro is 202306L. The other branch advertises zero.
- **P2407R5:** Preserved prior work deleting all eight freestanding
`variant::get` declarations while retaining `get_if`, `visit`, and supported
members. Positive and negative tests passed in this worktree.
- **P2937R0:** Preserved the freestanding `<cstring>` exclusion for `strtok`;
its negative compile test passed. Hosted extensions remain unaffected.
- **P2833R2:** Preserved the existing `span::at` and `expected::value`
deletions, `mdspan` surface, and `out_ptr`/`inout_ptr` implementation. The
unique/shared pointer adaptor tests passed in no-exceptions/no-threads mode;
hosted-only shared-pointer Mandates remain conditional.
- **P2976R1:** Bumped algorithm, memory, and numeric FTMs to 202502L and added
execution/random at 202502L. Execution policy tags and traits are available
from C++17 without requiring optional PSTL. The algorithm and numeric PSTL
implementation headers are suppressed in freestanding mode; separate helpers
declare every standard execution-policy overload freestanding-deleted (79
algorithm and 16 numeric declarations), and memory has the 12 required deleted
declarations. The three allocating classic algorithms and their ranges CPOs
are hosted-only. The required random subset includes integer engines/adaptors,
`uniform_int_distribution`, and `uniform_random_bit_generator`; `seed_seq`,
`random_device`, floating distributions, `shuffle_order_engine`/`knuth_b`,
`default_random_engine`, `generate_canonical`, and `ranges::generate_random`
remain unavailable. Permitted streaming declarations and hosted header
extensions remain intact. Module exports follow the same boundaries.

## Bugs found and fixed

1. CMake computed the operator-new macro, but `__config_site.in` did not
substitute it. Installed headers therefore always used fallback zero. Added
the site-config template entry and verified generated output for both 0L and
202306L.
2. P2976's three allocating algorithms had gated classic overloads, but ranges
implementations/includes/module exports remained visible in freestanding mode.
Gated implementation headers, public includes, and module exports; added one
negative test per ranges CPO.
3. `__numeric/pstl.h` did not suppress policy implementations for a
freestanding build with experimental PSTL enabled. Added the same
freestanding gate used by the algorithm PSTL header so deleted declarations
remain selected in that configuration.
4. A test draft assumed `errc` implicitly converts to `error_code`; P2338 only
requires the `errc` enum. Removed that unrelated assertion.
5. Adding the build-tree libc++ include path after source headers caused
`include_next` to pick stale staged headers and fail the C system-header
`size_t` path. Focused checks used the existing no-threads `__config_site`
overlay plus source headers; configured include order remains for root gates.

## Focused validation run

Compiler: `/home/spawn/dev/toolchains/clang-p2996/build-nyx/bin/clang++`.
No-threads config overlay: `/tmp/waves56-libcxx-no-threads`. Syntax checks used
`-ffreestanding -fno-exceptions`; no Ninja command or shared build tree was
used.

Final positive/negative focused matrix:

```sh
set -e
clang=/home/spawn/dev/toolchains/clang-p2996/build-nyx/bin/clang++
flags=(-std=c++2c -nostdinc++ -I/tmp/waves56-libcxx-no-threads -isystem libcxx/include -Ilibcxx/test/support -ffreestanding -fno-exceptions)
for t in libcxx/test/libcxx/freestanding/p2338r4-primitives.compile.pass.cpp libcxx/test/libcxx/freestanding/p2013r5-global-operator-new.compile.pass.cpp libcxx/test/libcxx/freestanding/p2976r1-algorithm.compile.pass.cpp libcxx/test/libcxx/freestanding/p2976r1-numeric.compile.pass.cpp libcxx/test/libcxx/freestanding/p2976r1-random.compile.pass.cpp libcxx/test/libcxx/freestanding/p2976r1-execution.compile.pass.cpp libcxx/test/libcxx/freestanding/variant.compile.pass.cpp libcxx/test/libcxx/freestanding/out_ptr.compile.pass.cpp libcxx/test/libcxx/freestanding/out_ptr_shared_ptr_mandate.compile.pass.cpp libcxx/test/libcxx/freestanding/partial-classes.compile.pass.cpp; do "$clang" $flags -fsyntax-only "$t"; done
"$clang" $flags -D_LIBCPP_FREESTANDING_OPERATOR_NEW_VALUE=202306L -fsyntax-only libcxx/test/libcxx/freestanding/p2013r5-global-operator-new.compile.pass.cpp
for t in libcxx/test/libcxx/freestanding/p2976r1-algorithm-parallel.compile.fail.cpp libcxx/test/libcxx/freestanding/p2976r1-numeric-parallel.compile.fail.cpp libcxx/test/libcxx/freestanding/p2976r1-memory-parallel.compile.fail.cpp libcxx/test/libcxx/freestanding/p2976r1-stable_sort.compile.fail.cpp libcxx/test/libcxx/freestanding/p2976r1-stable_partition.compile.fail.cpp libcxx/test/libcxx/freestanding/p2976r1-inplace_merge.compile.fail.cpp libcxx/test/libcxx/freestanding/p2976r1-ranges-stable_sort.compile.fail.cpp libcxx/test/libcxx/freestanding/p2976r1-ranges-stable_partition.compile.fail.cpp libcxx/test/libcxx/freestanding/p2976r1-ranges-inplace_merge.compile.fail.cpp libcxx/test/libcxx/freestanding/p2976r1-random-not-default-engine.compile.fail.cpp libcxx/test/libcxx/freestanding/p2976r1-random-not-floating-distribution.compile.fail.cpp libcxx/test/libcxx/freestanding/p2976r1-random-not-generate-canonical.compile.fail.cpp libcxx/test/libcxx/freestanding/p2976r1-random-not-shuffle-order.compile.fail.cpp libcxx/test/libcxx/freestanding/p2976r1-random-not-uniform-real.compile.fail.cpp libcxx/test/libcxx/freestanding/random-not-seed_seq.compile.fail.cpp libcxx/test/libcxx/freestanding/random-not-random_device.compile.fail.cpp libcxx/test/libcxx/freestanding/variant-get.compile.fail.cpp libcxx/test/libcxx/freestanding/strtok.compile.fail.cpp; do if "$clang" $flags -fsyntax-only "$t" >/tmp/waves56-negative.out 2>&1; then echo "UNEXPECTED_PASS $t"; exit 1; fi; done
```

All 10 positives and the configured-allocator positive passed. All 18 expected
failures failed compilation as intended. The execution-policy positive and the
algorithm/numeric/memory policy negatives also passed their boundary checks in
C++17 mode.

Generated macro tests were syntax checked with the same compiler and flags for
`algorithm`, `cerrno`, `charconv`, `cmath`, `cstdlib`, `cstring`, `cwchar`,
`execution`, `memory`, `new`, `numeric`, `random`, `string`, `system_error`,
`variant`, and `version`; all passed with:

```sh
for t in $(rg --files libcxx/test/std/language.support/support.limits/support.limits.general | rg '(algorithm|cerrno|charconv|cmath|cstdlib|cstring|cwchar|execution|memory|new|numeric|random|string|system_error|variant|version)\.version\.compile\.pass\.cpp$' | sort); do "$clang" $flags -fsyntax-only "$t" || exit; done
```

C++17 execution-policy availability and algorithm/numeric/memory deleted
overloads passed with:

```sh
clang=/home/spawn/dev/toolchains/clang-p2996/build-nyx/bin/clang++
flags=(-std=c++17 -nostdinc++ -I/tmp/waves56-libcxx-no-threads -isystem libcxx/include -Ilibcxx/test/support -ffreestanding -fno-exceptions)
"$clang" $flags -fsyntax-only libcxx/test/libcxx/freestanding/p2976r1-execution.compile.pass.cpp
for t in libcxx/test/libcxx/freestanding/p2976r1-algorithm-parallel.compile.fail.cpp libcxx/test/libcxx/freestanding/p2976r1-numeric-parallel.compile.fail.cpp libcxx/test/libcxx/freestanding/p2976r1-memory-parallel.compile.fail.cpp; do if "$clang" $flags -fsyntax-only "$t" >/tmp/waves56-negative.out 2>&1; then echo "UNEXPECTED_PASS $t"; exit 1; fi; done
```

Module source parse, freestanding and hosted mode:

```sh
/home/spawn/dev/toolchains/clang-p2996/build-nyx/bin/clang++ -std=c++2c -nostdinc++ -I/tmp/waves56-libcxx-no-threads -isystem libcxx/include -fno-exceptions -ffreestanding -x c++-module -fsyntax-only /tmp/waves56-freestanding-std.cppm
/home/spawn/dev/toolchains/clang-p2996/build-nyx/bin/clang++ -std=c++2c -nostdinc++ -I/tmp/waves56-libcxx-no-threads -isystem libcxx/include -fno-exceptions -x c++-module -fsyntax-only /tmp/waves56-freestanding-std.cppm
```

Both parsed successfully; Clang emitted only its reserved-module-name warning
for the temporary `std` module probe. Site-config substitution was verified
for each configured value using this temporary script and commands:

```sh
cat >/tmp/waves56-config-template.cmake <<'CMAKE'
set(_LIBCPP_ABI_VERSION 1)
set(_LIBCPP_ABI_NAMESPACE "__1")
set(_LIBCPP_FREESTANDING_OPERATOR_NEW_VALUE "${VALUE}")
configure_file("/home/spawn/dev/toolchains/clang-p2996/build-waves56-worktrees/freestanding/libcxx/include/__config_site.in" "${OUT}" @ONLY)
CMAKE
cmake -DVALUE=0L -DOUT=/tmp/waves56-config-site-off.h -P /tmp/waves56-config-template.cmake
cmake -DVALUE=202306L -DOUT=/tmp/waves56-config-site-on.h -P /tmp/waves56-config-template.cmake
rg -n '^#define _LIBCPP_FREESTANDING_OPERATOR_NEW_VALUE (0L|202306L)$' /tmp/waves56-config-site-off.h /tmp/waves56-config-site-on.h
```

Regeneration and source hygiene commands, all successful:

```sh
python3 libcxx/utils/generate_feature_test_macro_components.py
python3 libcxx/utils/generate_libcxx_cppm_in.py std
git diff --check
```

## Root integration handoff

The root integration gates are recorded below. Issue #116 remains deferred.

## Root integration gate update (2026-09-29)

The root build regenerated and tested the integrated module/header outputs.
The focused libc++ feature suite passed 133/133. The clean-rebuilt full libc++
archive passed 11,497 tests (26 XFAIL, 1,089 UNSUPPORTED); comparison with the
baseline found only three sandbox-only new failures: GDB ptrace denial and two
filesystem Unix-socket bind denials. The full Clang archive comparison found
no new compiler failures; three added Python 3.14 lit helper failures were
forkserver socket denials. These gates found no freestanding feature regressions.
The umbrella issue remains open pending final review and publication.
