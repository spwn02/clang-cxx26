# CXX26 Clang: a C++26 LLVM fork

CXX26 Clang is a fork of [`llvm/llvm-project`](https://github.com/llvm/llvm-project)
that implements C++26 as specified by the C++ working draft N5050 plus the applied
defect reports: the language in Clang and the standard library in libc++, including
static reflection ([P2996](https://wg21.link/p2996) and its companion papers),
expansion statements, annotations and contracts. Enable it with `-std=c++26`; these
features are part of that mode and have no experimental flags (see `AGENTS.md`).
Earlier standards (`-std=c++03` through `-std=c++23`) are supported.

## Release

The current release is
[`cxx26-2026.10.30`](https://github.com/spwn02/clang-cxx26/releases/tag/cxx26-2026.10.30),
a Linux x86_64 toolchain with Clang, clangd, clang-tidy, lld, libc++, libc++abi,
libunwind, the compiler-rt runtimes and the libc++ `import std` module sources.

```bash
gh release download cxx26-2026.10.30 --repo spwn02/clang-cxx26
sha256sum -c clang-cxx26-2026.10.30-linux-x86_64.SHA256SUMS
zstd -dc clang-cxx26-2026.10.30-linux-x86_64.tar.zst | tar -x
source clang-cxx26-2026.10.30/share/clang-cxx26/activate.sh   # sets CC, CXX, PATH and the library path
$CXX -std=c++26 hello.cpp
```

For CMake use the toolchain file named by `$CXX26_CMAKE_TOOLCHAIN_FILE`. The snapshot
format and the release process are described in `cxx26/toolchain/README.md`.

Code written for earlier snapshots of this fork or for stock Clang may need changes; they are
listed in [`cxx26/MIGRATING.md`](cxx26/MIGRATING.md).

Without `gh`, download the same files from the release page with `curl -LO`. The toolchain needs a
C library and the GCC runtime on the host (`build-essential` on Debian and Ubuntu, which provides the C
headers and `crtbeginS.o`/`libgcc`) and `zstd` to unpack the archive. CMake's `import std` additionally
needs CMake 4.4 or newer and Ninja 1.11 or newer.

Platform: the published toolchain is a Linux x86_64 archive. The compiler and libc++ also build from source on
macOS arm64 (Apple Silicon) and on Windows with the MSVC ABI (`clang-cl`), where eight small
programs (hello world, reflection, expansion statements, contracts, `std::execution`, `<simd>`, a constexpr exception and
`std::text_encoding`) compile and run; there are no binary releases for them yet. See the
[FAQ](cxx26/FAQ.md#which-platforms-are-supported) for what was checked and the known issues.

## Try it

```cpp
// reflect.cpp: print the members of a struct with static reflection (P2996)
#include <meta>
#include <print>

struct Point { int x; double y; };

int main() {
  template for (constexpr auto member :
                std::define_static_array(std::meta::nonstatic_data_members_of(
                    ^^Point, std::meta::access_context::unchecked()))) {
    std::println("{}: {}", std::meta::identifier_of(member),
                 std::meta::display_string_of(std::meta::type_of(member)));
  }
}
```

```cpp
// contracts.cpp: preconditions and postconditions (P2900)
int divide(const int a, const int b)
  pre(b != 0)
  post(r : r * b <= a)
{
  return a / b;
}
```

```bash
$CXX -std=c++26 reflect.cpp -o reflect && ./reflect     # x: int / y: double
```

A violated contract terminates by default (`-fcontract-evaluation-semantic=` selects the semantic).

More questions are answered in [`cxx26/FAQ.md`](cxx26/FAQ.md). Bugs and questions go to the
[issue tracker](https://github.com/spwn02/clang-cxx26/issues).

## Standard library

libc++ implements the C++26 library, including the parallel and numeric parts. Every paper and issue row in
libc++'s C++26 status tables ([`Cxx2cPapers.csv`](libcxx/docs/Status/Cxx2cPapers.csv),
[`Cxx2cIssues.csv`](libcxx/docs/Status/Cxx2cIssues.csv)) is either Complete or Nothing To Do (a paper that needs no
library change); none is left open.

| Facility | Header | Papers |
| --- | --- | --- |
| Senders and receivers, `std::execution` | `<execution>` | P2300R10, P3284R4, P3396R1, P3481R5 (`bulk`) |
| Parallel scheduler, `async_scope`, coroutine `task` | `<execution>` | P2079R10, P3149R11, P3552R3 |
| Parallel range algorithms (`std::ranges::sort(std::execution::par, r)` and the rest) | `<algorithm>`, `<numeric>` | P3179R9 |
| Data-parallel types | `<simd>` | P1928R15, P2876R3 |
| Linear algebra, BLAS-style, with execution-policy overloads | `<linalg>` | P1673R13, P3371R5 |
| `mdspan`, `submdspan`, `aligned_accessor` | `<mdspan>` | P0009R18, P2630R4, P2897R7 |
| `inplace_vector`, `optional<T&>` | `<inplace_vector>`, `<optional>` | P0843R14, P2988R12 |

The execution policies use libc++'s `std::thread` backend in the released toolchain, so `par` and `par_unseq` run
on several threads. Each example below was compiled with `-std=c++26 -Wall -Wextra -Wpedantic -Werror` and run:

```cpp
#include <algorithm>
#include <execution>
#include <linalg>
#include <mdspan>
#include <print>
#include <vector>

int main() {
  std::vector<int> v{5, 3, 4, 1, 2};
  std::ranges::sort(std::execution::par_unseq, v);              // parallel range algorithm (P3179)
  std::vector<double> a{1, 2, 3, 4}, b{5, 6, 7, 8}, c(4);
  std::mdspan A(a.data(), 2, 2), B(b.data(), 2, 2), C(c.data(), 2, 2);
  std::linalg::matrix_product(std::execution::par, A, B, C);    // C = A * B (P1673)
  std::println("{} | {} {} {} {}", v, C[0, 0], C[0, 1], C[1, 0], C[1, 1]);
}
```

```cpp
#include <execution>
#include <print>
#include <simd>
#include <vector>

namespace ex = std::execution;

int main() {
  std::vector<int> squares(8);
  auto work = ex::schedule(ex::get_parallel_scheduler())        // P2079
            | ex::bulk(ex::par, squares.size(),                 // P3481
                       [&](std::size_t i) { squares[i] = int(i * i); })
            | ex::then([&] { return squares.back(); });
  auto [last] = std::this_thread::sync_wait(std::move(work)).value();   // P2300
  std::simd::vec<int, 4> x{[](auto i) { return int(i) + 1; }};          // P1928
  std::println("{} {}", last, std::simd::reduce(x));
}
```

## Differences from stock Clang

The differences are deliberate and documented in `clang/docs/ReleaseNotes.rst`. The one
that affects ordinary code is the language linkage of function types. [dcl.link] says that
function types with C and C++ language linkage are distinct types, so a pointer to a C++
function does not convert to a pointer to an `extern "C"` function (`atexit`, `qsort`,
`sigaction`, `pthread_create` and the like). GCC, MSVC and stock Clang accept that code; this
fork follows the standard and diagnoses it with `-Wfunction-type-language-linkage`, an error by
default. `-Wno-error=function-type-language-linkage` downgrades it to a warning, which is still
a diagnostic. The one place where the fork accepts more than the standard does is a system
header: there the two linkages are not told apart, so that the headers of the host C++ library,
which assume the old behavior, keep compiling.

This fork preserves upstream LLVM history and the history of its Bloomberg-originated
reflection implementation. That provenance does not imply Bloomberg endorsement
of this fork.

# The LLVM Compiler Infrastructure

Welcome to the LLVM project!

This repository contains the source code for LLVM, a toolkit for the
construction of highly optimized compilers, optimizers, and run-time
environments.

The LLVM project has multiple components. The core of the project is
itself called "LLVM". This contains all of the tools, libraries, and header
files needed to process intermediate representations and convert them into
object files. Tools include an assembler, disassembler, bitcode analyzer, and
bitcode optimizer.

C-like languages use the [Clang](https://clang.llvm.org/) frontend. This
component compiles C, C++, Objective-C, and Objective-C++ code into LLVM bitcode
-- and from there into object files, using LLVM.

Other components include:
the [libc++ C++ standard library](https://libcxx.llvm.org),
the [LLD linker](https://lld.llvm.org), and more.

## Getting the Source Code and Building LLVM

Consult the
[Getting Started with LLVM](https://llvm.org/docs/GettingStarted.html#getting-the-source-code-and-building-llvm)
page for information on building and running LLVM.

For information on how to contribute to the LLVM project, please take a look at
the [Contributing to LLVM](https://llvm.org/docs/Contributing.html) guide.

## Getting in touch

Questions, bug reports and mismatches with the C++26 draft for this fork go to its
[issue tracker](https://github.com/spwn02/clang-cxx26/issues). The LLVM Discourse forums, Discord chat and
office hours are LLVM's channels and do not support this fork. Code in this repository that comes from LLVM is
still governed by the [LLVM code of conduct](https://llvm.org/docs/CodeOfConduct.html).
