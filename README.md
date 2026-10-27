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
[`cxx26-2026.10.27`](https://github.com/spwn02/clang-cxx26/releases/tag/cxx26-2026.10.27),
a Linux x86_64 toolchain with Clang, clangd, clang-tidy, lld, libc++, libc++abi,
libunwind, the compiler-rt runtimes and the libc++ `import std` module sources.

```bash
gh release download cxx26-2026.10.27 --repo spwn02/clang-cxx26
sha256sum -c clang-cxx26-2026.10.27-linux-x86_64.SHA256SUMS
zstd -dc clang-cxx26-2026.10.27-linux-x86_64.tar.zst | tar -x
source clang-cxx26-2026.10.27/share/clang-cxx26/activate.sh   # sets CC, CXX, PATH and the library path
$CXX -std=c++26 hello.cpp
```

For CMake use the toolchain file named by `$CXX26_CMAKE_TOOLCHAIN_FILE`. The snapshot
format and the release process are described in `cxx26/toolchain/README.md`.

## Deviations from the standard

The deviations are deliberate and documented in `clang/docs/ReleaseNotes.rst`. The one
that affects ordinary code: function types with C and C++ language linkage are distinct
types ([dcl.link]), as the standard requires, but converting between pointers to such
functions is diagnosed with `-Wfunction-type-language-linkage`, an error by default that
`-Wno-error=function-type-language-linkage` downgrades, and a use inside a system header
does not tell the linkages apart, so the headers of the host C++ library keep compiling.

This fork preserves upstream LLVM history and the history of its Bloomberg-originated
reflection implementation. That provenance does not imply Bloomberg endorsement
of this fork.

# The LLVM Compiler Infrastructure

[![OpenSSF Scorecard](https://api.securityscorecards.dev/projects/github.com/llvm/llvm-project/badge)](https://securityscorecards.dev/viewer/?uri=github.com/llvm/llvm-project)
[![OpenSSF Best Practices](https://www.bestpractices.dev/projects/8273/badge)](https://www.bestpractices.dev/projects/8273)
[![libc++](https://github.com/llvm/llvm-project/actions/workflows/libcxx-build-and-test.yaml/badge.svg?branch=main&event=schedule)](https://github.com/llvm/llvm-project/actions/workflows/libcxx-build-and-test.yaml?query=event%3Aschedule)

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

Join the [LLVM Discourse forums](https://discourse.llvm.org/), [Discord
chat](https://discord.gg/xS7Z362),
[LLVM Office Hours](https://llvm.org/docs/GettingInvolved.html#office-hours) or
[Regular sync-ups](https://llvm.org/docs/GettingInvolved.html#online-sync-ups).

The LLVM project has adopted a [code of conduct](https://llvm.org/docs/CodeOfConduct.html) for
participants to all modes of communication within the project.
