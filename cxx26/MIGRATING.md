# Migrating to `cxx26-2026.10.30`

This page lists what existing C++ code has to change to build with the `cxx26-2026.10.30`
toolchain. Each item is a deliberate change: it follows the C++26 draft (N5050 plus the applied
defect reports) or removes a fork-specific spelling. The complete list is "Potentially Breaking
Changes" in `clang/docs/ReleaseNotes.rst`.

The items marked **seen in practice** were hit while moving two real code bases (a module-based
library and a library that imports it) to this toolchain; the examples come from that work.

Install and activate the toolchain as described in the [README](../README.md#release). The
`-std=c++26` mode enables everything below; there is nothing else to switch on.

## Compiler flags that no longer exist

Reflection, expansion statements, annotations and contracts are part of `-std=c++26`. These
flags were removed, and passing one is `error: unknown argument`:

| Removed | Replacement |
| --- | --- |
| `-freflection`, `-freflection-latest` | nothing; use `-std=c++26` |
| `-fcontracts`, `-fcontract-group-*`, and the other `-fcontract-*` options | nothing; `-fcontract-evaluation-semantic=` stays (it selects the evaluation semantic) |
| `-fexperimental-library` | nothing; see [libc++](#libc) |
| `-f[no-]erroneous-initialization` | nothing; erroneous behavior for uninitialized reads (P2795R5) is the default |

**Seen in practice:** a CMake target carried `target_compile_options(... -fexperimental-library)`.
CMake copied it onto the synthetic `import std` target, so the failure appeared as
`clang-scan-deps: unknown argument: '-fexperimental-library'` before any project source was
compiled. Remove the option from the target and from any presets or CI scripts.

Feature-test code: the `__has_feature` names `reflection`, `parameter_reflection`,
`expansion_statements`, `annotation_attributes` and `experimental_library` are gone. Use
`__cpp_impl_reflection` and `__cplusplus`. The contract extensions the draft does not have
(`[[clang::contract_semantic]]`, `[[clang::contract_message]]`, `[[clang::contract_emission]]`)
were removed.

## libc++

- The experimental library is gone. The parallel algorithms and execution policies, the time zone
  database (`std::chrono::tzdb`, `zoned_time`), `<syncstream>` and `std::optional`'s range support
  are in the library in the language modes that have them, with no flag and no
  `-lc++experimental`. There is no `libc++experimental.a`; drop it from link lines.
- The `<experimental/*>` Technical Specification headers still require
  `_LIBCPP_ENABLE_EXPERIMENTAL`.
- Hardening: a hardened precondition violation under the `enforce` semantic is a contract
  violation and goes through the replaceable contract-violation handler. The `ignore` and
  `observe` assertion semantics were removed (P3878R1).
- `std::contracts::assertion_kind` is `pre`, `post`, `assert` and `detection_mode` is
  `predicate_false`, `evaluation_exception`, as in the draft.

**Seen in practice:** code that switched over `assertion_kind` listed `manual` and `cassert`, and
code that switched over `detection_mode` listed `unspecified`. Those enumerators do not exist:
delete the `case` labels. A `case` label at the end of a `switch` needs a statement, so give the
remaining `assert` label its `return`.

## Modules

`export` cannot name an explicit specialization, an explicit instantiation or a partial
specialization ([module.interface]). They do not need it: a partial specialization declared in a
module interface unit is reachable from the importers.

```cpp
// before: error: an export declaration cannot declare a partial specialization
export template <class T>
inline constexpr bool std::ranges::enable_borrowed_range<my::View<T>> = true;

// after
template <class T>
inline constexpr bool std::ranges::enable_borrowed_range<my::View<T>> = true;
```

**Seen in practice:** `std::formatter`, `std::ranges::enable_view` and
`std::ranges::enable_borrowed_range` specializations for a library's own types (seven
declarations in one code base).

`import` and `module` directives follow [cpp.pre] (P1857R3): they begin a directive only as the
first token of a logical line and end at the first new-line. A mid-line `import` is an ordinary
identifier, `import .x;` is no longer a directive, and `module;export module M;` on one line is
rejected.

## Language linkage of function types

Function types with C language linkage are distinct from function types with C++ language linkage
([dcl.link], CWG13, CWG168). A pointer to a C++ function can no longer be passed where a pointer to
an `extern "C"` function is expected, or the reverse: `atexit`, `qsort`, `bsearch`, `signal`,
`sigaction`'s `sa_sigaction`, `pthread_create`. The conversion is
`-Wfunction-type-language-linkage`, an error by default.

Define the function inside a linkage-specification so that its type has C language linkage:

```cpp
// before: error: converting 'auto (*)(int, siginfo_t *, void *) noexcept -> void'
//         to 'extern "C" void (*)(int, siginfo_t *, void *)' changes the language linkage
namespace {
auto faultHandler(int, siginfo_t *, void *) noexcept -> void { /* ... */ }
}
action.sa_sigaction = &faultHandler;

// after
namespace {
extern "C" {
auto faultHandler(int, siginfo_t *, void *) noexcept -> void { /* ... */ }
}
}
action.sa_sigaction = &faultHandler;
```

An unnamed namespace gives the function internal linkage even inside the linkage-specification
(CWG1113, CWG4), so this stays private to the translation unit. Other ways out: an
`extern "C" typedef` for the type, or `-Wno-error=function-type-language-linkage` (or
`-Wno-function-type-language-linkage`) to downgrade or silence the diagnostic while migrating.

Code in a system header is exempt, so the headers of the host C++ library keep compiling.

**Seen in practice:** a POSIX signal handler installed with `sigaction`.

## Other changes in this release

- An array of an abstract class type is a valid type (CWG1640); only defining an object of such an
  array type is an error. A non-static data member found in several base class subobjects of the
  same type is not ambiguous by itself, so `sizeof(D::n)` is valid.
- The nested-name-specifier of a friend function declarator is declarative: a `decltype` or
  pack-indexing specifier in it is diagnosed (CWG2858).
- Explicit specialization and instantiation declarators no longer need an enclosing namespace
  (P1787R6). `-Wfriend-specialization-specifier` diagnoses `inline`, `constexpr` or `consteval` on a
  friend declaration of a function template specialization (CWG2379).
- A replaceable global allocation function in a module purview is attached to the global module
  (CWG2597).
- Clang warns when the auto-detected GCC installation has no libstdc++ headers although another one
  does; select one with `--gcc-install-dir` or silence it with `-Wno-gcc-install-dir-libstdcxx`.

## Checking a build

1. Activate the toolchain and compile with `$CXX -std=c++26`; nothing else is needed for
   reflection, contracts or expansion statements.
2. Remove the flags listed above from CMake targets, presets, `compile_flags.txt` and CI.
3. Fix what the diagnostics name. The removed flags, the `export` rule, the contracts enumerators and the
   language-linkage conversions are all compile-time errors that point at the construct.
   The hardening semantic and erroneous behavior are runtime changes: re-run your tests.
4. If you use CMake `import std`, include the toolchain file named by `$CXX26_CMAKE_TOOLCHAIN_FILE`;
   it sets the experimental `import std` gate CMake 4.4 still requires.
