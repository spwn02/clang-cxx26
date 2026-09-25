# P2795R5 erroneous behaviour for uninitialized reads — design note (#41)

Status: **plan only**, nothing implemented. Probed 2026-09-25 on HEAD `b1811f61aca4`.
Primary source: https://wg21.link/P2795R5 (adopted for C++26).

## What the paper changes

- New category *erroneous behavior*: well-defined behaviour the implementation is *recommended to diagnose*.
- Objects with automatic storage duration and no initializer are initialised to an implementation-defined
  fixed value (an *erroneous value*); reading one is erroneous behaviour rather than UB. The compiler may not assume
  it does not happen, but may diagnose or terminate.
- `[[indeterminate]]` attribute: opts a block-scope variable or a function parameter (first declaration only, no
  arguments) back into the pre-C++26 indeterminate value; reading is UB again. No opt-out for temporaries.
- Still UB: dereferencing an erroneous pointer, invalid representations of e.g. `bool`, accessing outside lifetime.
- Constant evaluation is unchanged in effect: reading an erroneous value is not a constant expression.

## Current behaviour (HEAD, `-std=c++26`)

| Probe | Result |
|---|---|
| `void f(){ int x [[indeterminate]]; }` | `warning: unknown attribute 'indeterminate' ignored` (also on parameters) |
| `int f(){ int x; return x; }` at `-O0` | `alloca` + `load` of the uninitialised slot (indeterminate) |
| same at `-O2` | `ret i32 undef`: the optimizer assumes the read never happens (UB semantics) |
| `-ftrivial-auto-var-init=zero` | works (the existing per-variable mechanism, `CGDecl.cpp` ~1259) |
| `[[clang::uninitialized]]` | exists (`Attr.td`), same intent as `[[indeterminate]]` for that flag |
| `__cpp_*` / FTM | none defined for this paper (the paper adds no FTM) |

So: no language-level erroneous behaviour, no `[[indeterminate]]`, and `-O2` treats reads as UB.

## Design

Two independent levels, matching how the paper can be met in stages:

1. **Attribute (front end only)**: add `[[indeterminate]]` as a standard C++26 attribute (`Attr.td`: `CXX11<"", "indeterminate", 202311>` style
   spelling in the standard-attribute table, `SubjectList` = `Var` (local, non-static) and `ParmVar`), Sema check that the first
   declaration of a parameter carries it and redeclarations agree (ill-formed otherwise), semantics identical to `[[clang::uninitialized]]`
   for codegen. Add `__has_cpp_attribute(indeterminate)`. Cheap and independently useful.
2. **Erroneous values (CodeGen and diagnostics)**: in C++26 mode, treat automatic variables without initializer as if
   `-ftrivial-auto-var-init=<zero|pattern>` applied unless `[[indeterminate]]`/`[[clang::uninitialized]]`. Choose the value
   (recommendation: `pattern`, matching the paper's spirit of making the bug visible; `zero` is the friendlier default), and a
   driver flag to switch it off (`-fno-erroneous-behavior`-style) for code that measures the perf hit. Temporaries and `new`ed
   objects without an initializer are not automatic variables and are out of the paper's scope for the value guarantee, but note the
   "no opt-out for temporaries" rule.
3. **Diagnostics** (recommended, not required): reuse `-Wuninitialized`/`-Wsometimes-uninitialized`/`-Wconditional-uninitialized`;
   optionally add a dynamic check mode via `-fsanitize=...` later.

Not needed: constant evaluator changes (already rejects), libc++ (no library facility).

## Touch points

`clang/include/clang/Basic/Attr.td` (+ `AttrDocs.td`), `clang/lib/Sema/SemaDeclAttr.cpp` (subject + redeclaration check),
`clang/lib/CodeGen/CGDecl.cpp` (`shouldSplitConstantStore`/`isTrivialInitializer` and the auto-var-init path),
`clang/lib/Frontend/CompilerInvocation.cpp` + `clang/include/clang/Driver/Options.td` (default and opt-out flag),
`clang/lib/Sema/AnalysisBasedWarnings.cpp` (wording), `clang/test/{SemaCXX,CodeGenCXX,Parser}`.

## Staged milestones

1. `[[indeterminate]]` attribute + tests + docs (small).
2. C++26 default erroneous initialisation + flag + codegen tests; measure code-size/perf on the fork's own build and
   on a libc++ lit run (hot spots: small arrays and `std::array<T,N>` locals, SSO buffers in `std::string`/`std::format`, which libc++
   must mark `[[indeterminate]]` where it deliberately leaves storage uninitialised).
3. libc++ audit: mark deliberate uninitialised buffers (`__compressed_pair` storage, `uninitialized_*` scratch, format
   buffers) so performance does not regress.

## Test plan

Paper examples: indeterminate vs erroneous read; `[[indeterminate]]` on a parameter and its first-declaration rule;
`char buf[N] [[indeterminate]]`; assertion that `-O2` output no longer contains `undef` for the erroneous read; PCH round trip of the
attribute; ODR/redeclaration diagnostics; `-std=c++23` unchanged.

## Risks and unknowns

- Performance regression in C++26 mode by default; this is what the paper accepts, but the fork's own libc++ needs the
  audit in milestone 3 before the default flips.
- Choice of erroneous value is implementation-defined but must be *fixed* per build; interacts with `-ftrivial-auto-var-init`
  users who already picked `pattern`.
- Sanitizer interplay (MSan treats these as initialised: needs to keep reporting, or explicitly not, to match the paper).

Effort: milestone 1 small; milestone 2 moderate; milestone 3 is the unknown (libc++ audit).
