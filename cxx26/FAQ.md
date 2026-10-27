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

Only Linux x86_64 is built and tested today. The host needs a C library and the GCC runtime (on Debian and Ubuntu,
`build-essential`). The fork is not inherently tied to Linux or x86, but no other platform is validated.

## Is this the same as Bloomberg's `clang-p2996`?

No. This fork preserves the history of Bloomberg's reflection implementation and builds on it, but it follows the
C++26 draft as adopted rather than the P2996 proposal as first written, and carries a lot of other work (the rest of
C++26, defect reports, libc++). That history does not imply Bloomberg endorses this fork.

## Is it part of LLVM?

No. It is an independent fork, not an LLVM release, and the LLVM project does not support it. The license is the one
of LLVM: Apache-2.0 with the LLVM exception (`LICENSE.TXT`).

## What does "C++26" mean here?

The N5050 working draft with the defect reports applied (core and library issues the committee has adopted). Where the
fork deliberately differs from the draft the difference is listed in "Potentially Breaking Changes" in
`clang/docs/ReleaseNotes.rst`; the README summarizes the one that affects ordinary code (function types and language
linkage in system headers).

## Will my existing code build?

Usually yes. Code that used the old experimental spellings or relied on pre-C++26 leniencies needs small changes; they
are collected, with before/after examples, in [`cxx26/MIGRATING.md`](MIGRATING.md).

## Are older standards supported?

Yes: `-std=c++03` through `-std=c++23` still work. Use `-std=c++26` for the new features.

## Are releases stable?

A published snapshot (`cxx26-YYYY.MM.DD[.N]`) is immutable: its tag and assets never change, and a fix is a new
snapshot. The `cxx26` branch is a moving development branch. See `cxx26/toolchain/README.md` for the snapshot format
and the manifest each release carries.

## Does it have everything in the C++26 draft?

That is the aim. A fork-wide conformance audit against the draft (issue #116, closed) classified every defect-report row
and every library clause it checked; it does not claim the absence of bugs. If you find a mismatch with the draft, that
is a bug: please report it.

## How do I report a bug?

Open an issue on the [issue tracker](https://github.com/spwn02/clang-cxx26/issues) with the snapshot tag
(`$CXX --version` prints the source revision), a small reproducer, and the command line. For the language or library
specification itself, ask the committee; this project implements the draft rather than defining it.
