Title: clang-tidy crashes with libcpp-cpp-version-check or libcpp-internal-ftms enabled, preprocessing `#if defined(FOO) && __has_include(<stddef.h>)`

### Summary

A vanilla `clang-tidy` binary (built from an unmodified `llvmorg-22.1.8` checkout,
no third-party checks beyond libc++'s own in-tree `libcpp-*` checks) crashes while
preprocessing a three-line reproducer when either of two `PPCallbacks`-registering
checks is enabled: `libcpp-cpp-version-check` or `libcpp-internal-ftms`.

### Minimal reproducer

```cpp
#if defined(FOO) && __has_include(<stddef.h>)
#endif
```

```
clang-tidy -checks='-*,libcpp-cpp-version-check' repro.cpp -- -std=c++20
```//

Crashes identically with `-checks='-*,libcpp-internal-ftms'` instead. Both are the
only two of libc++'s nine in-tree `libcpp-*` checks that register `PPCallbacks`
(`proper_version_checks.cpp` / `internal_ftm_use.cpp` in
`libcxx/test/tools/clang_tidy_checks/`).

### Confirmation this is not a libc++-fork issue

This was found while running libc++'s own `clang_tidy.gen.py`/`*.sh.py` test suite
(145 crashing files, all going through one of these two checks) on a downstream
fork tracking `llvmorg-22.1.8`. Before reporting, we ruled out every
fork/environment-specific explanation:

1. **Plugin/host ABI mismatch** — rebuilt `libcxx-tidy.plugin` fresh against the
   exact `clang-tidy` binary being used (note: `ninja cxx-test-depends` does *not*
   rebuild this plugin; the target is `libcxx-tidy.plugin`). No change.
2. **Wrong Clang found by `find_package(Clang ...)`** — confirmed
   `libcxx/test/tools/clang_tidy_checks/CMakeLists.txt` can resolve to a
   system-installed Clang instead of the tree being tested. Ruled out as the cause:
   the crash reproduces identically with the 100% vanilla system `/usr/bin/clang-tidy`
   (matching ABI, matching version) loading the same plugin.
3. **Fork modification of the check sources** — confirmed byte-identical to
   upstream: `git diff llvmorg-22.1.8 -- libcxx/test/tools/clang_tidy_checks/{proper_version_checks,internal_ftm_use}.cpp libcxx/test/tools/clang_tidy_checks/CMakeLists.txt` is empty.

So: 100% vanilla `clang-tidy` binary, 100% vanilla check source, a plugin built the
normal way, crashing on ordinary system-header preprocessing
(`stddef.h`'s `__has_include_next` guard).

### Crash site

Varies by input (looks like corruption surfacing downstream of the real fault
rather than at it) — observed in `TokenLexer::PropagateLineStartLeadingSpaceInfo`
and `Preprocessor::LookupFile` across different repro shapes. Every function in
the observed call chain is unmodified from upstream
(`git diff llvmorg-22.1.8` empty for those files).

### Environment

- `llvmorg-22.1.8`, Release build, X86 target only, Ninja.
- Reproduced with both a `build-nyx`-built `clang-tidy` and the distro's
  `/usr/bin/clang-tidy`.

### Not yet done

Root cause inside `PPCallbacks` handling for `__has_include`/`__has_include_next`
during a `defined(...) && __has_include(...)` short-circuit was not traced to
completion — this write-up stops at "confirmed pure upstream, reproducible,
localized to two specific PPCallbacks-registering checks," not at the exact
faulting mechanism.

---
Drafted from `docs/LLVM22_SYNC.md`'s 2026-08-29/30 Milestone 8 session log entry
("Confirmed NOT a fork regression" section). Not yet filed — pending review before
posting publicly under the account's GitHub identity.
