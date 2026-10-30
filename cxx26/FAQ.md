# FAQ

## What is this?

A fork of [`llvm/llvm-project`](https://github.com/llvm/llvm-project) that implements C++26 as specified by the
working draft N5050 plus the applied defect reports, in Clang (the language) and libc++ (the standard library).
Static reflection ([P2996](https://wg21.link/p2996) and its companion papers), expansion statements, annotations and
contracts ([P2900](https://wg21.link/p2900)) are part of `-std=c++26`; they have no experimental flags.

## How do I install it?

See the [README](../README.md#release). A release is a Linux x86_64 archive: unpack it and `source` its
`share/clang-cxx26/activate.sh`. Nothing is installed system-wide.

## Which platforms are supported?

The release is a Linux x86_64 archive, and Linux x86_64 is the platform where the whole test suite runs. The host needs
a C library and the GCC runtime (on Debian and Ubuntu, `build-essential`).

The compiler and libc++ also build from source on two more platforms, checked on GitHub-hosted runners: macOS arm64
(Apple Silicon, Xcode 15.4) and Windows x64 with the MSVC ABI (`clang-cl`, Visual Studio 2022). On both, eight small programs
(hello world, reflection, expansion statements, contracts, `std::execution`, `<simd>`, a constexpr exception, `std::text_encoding`) compile and run, and on macOS some of the new
libc++ tests do. There are no binary releases for them, and their test suites have not been run. Constant evaluation of the transcendental functions of `<cmath>`
(`constexpr double s = std::sin(0.5);`, P1383R2) needs GMP and MPFR when the compiler is built: the Linux archive is built with
them, and a build from source finds them if they are installed (otherwise those functions are not usable in a constant
expression, and everything else works; the Windows runner has neither, so that check is skipped there). On Windows the modes C++11 to C++26 are checked; C++03 is not one, because the headers of Visual Studio themselves need `noexcept`. Snapshots up to 10.27 had two problems there, fixed since cxx26-2026.10.30: Apple's `ld` crashed when linking against the libc++
of the toolchain ([#350](https://github.com/spwn02/clang-cxx26/issues/350)), and `std::exception_ptr` was not constexpr
with the MSVC ABI ([#351](https://github.com/spwn02/clang-cxx26/issues/351)).

## Is this the same as Bloomberg's `clang-p2996`?

No. This fork preserves the history of Bloomberg's reflection implementation and builds on it, but it follows the
C++26 draft as adopted rather than the P2996 proposal as first written, and carries a lot of other work (the rest of
C++26, defect reports, libc++). That history does not imply Bloomberg endorses this fork.

## Is it part of LLVM?

No. It is an independent fork, not an LLVM release, and the LLVM project does not support it. The license is the one
of LLVM: Apache-2.0 with the LLVM exception (`LICENSE.TXT`).

## Is the libc++ of the toolchain ABI-compatible with the stock libc++?

No. The library keeps the inline namespace and the names, but some classes differ in layout (the comparison with the ABI
list of stock libc++ shows larger vtables for `basic_streambuf`, `basic_filebuf`, `basic_stringbuf`, `strstreambuf` and
`__assoc_sub_state`). Use the toolchain's libc++ for programs that are built with the toolchain, and do not mix it with
objects or libraries that were built against another libc++ (`activate.sh` puts the toolchain's library first on
`LD_LIBRARY_PATH`).

## What does "C++26" mean here?

The N5050 working draft with the defect reports applied (core and library issues the committee has adopted). Where the
fork is stricter than stock Clang, because it implements a rule of the standard that other compilers do not, the change
is listed in "Potentially Breaking Changes" in `clang/docs/ReleaseNotes.rst`; the README summarizes the one that affects
ordinary code (function types with C and C++ language linkage are distinct types, [dcl.link]). The only leniency
beyond the standard is that a system header does not tell the two linkages apart.

## Will my existing code build?

Usually yes. Code that used the old experimental spellings or relied on pre-C++26 leniencies needs small changes; they
are collected, with before/after examples, in [`cxx26/MIGRATING.md`](MIGRATING.md).

## Are older standards supported?

Yes: `-std=c++03` through `-std=c++23` still work. Use `-std=c++26` for the new features.

## Are releases stable?

A published snapshot (`cxx26-YYYY.MM.DD[.N]`) is immutable: its tag and assets never change, and a fix is a new
snapshot. The `cxx26` branch is a moving development branch. See `cxx26/toolchain/README.md` for the snapshot format
and the manifest each release carries.

## Which parts of the standard library are there?

All of the C++26 library, as libc++ tracks it: every row in `libcxx/docs/Status/Cxx2cPapers.csv` and
`Cxx2cIssues.csv` is Complete or Nothing To Do. That includes `std::execution` (senders and receivers, the parallel
scheduler, `async_scope`, `task`), `<simd>`, `<linalg>`, `<mdspan>`, `<inplace_vector>` and the execution-policy
overloads of the range algorithms (`std::ranges::sort(std::execution::par, r)`). The README lists the papers and has
examples. Status tables record that a paper's wording is implemented; they say nothing about how fast it runs.

## Does it have everything in the C++26 draft?

That is the aim. A fork-wide conformance audit against the draft (issue #116, closed) classified every defect-report row
and every library clause it checked; it does not claim the absence of bugs. If you find a mismatch with the draft, that
is a bug: please report it.

## How do I report a bug?

Open an issue on the [issue tracker](https://github.com/spwn02/clang-cxx26/issues) with the snapshot tag
(`$CXX --version` prints the source revision), a small reproducer, and the command line. For the language or library
specification itself, ask the committee; this project implements the draft rather than defining it.
