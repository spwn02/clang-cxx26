# M4 audit report: issues #188, #220, #237

Date: 2026-09-10

## Method

Read the exact tracker rows first, then reconstructed the issue bodies from
`docs/reflection-audit/upstream-issues-snapshot.json` (the live `gh issue view` fallback was
unavailable because GitHub API access failed). Probes used the staged libc++ test-suite headers
and `build-nyx/bin/clang++`, with `-std=c++26 -freflection-latest`; the #220 probe was wrapped
in `timeout 20`. Temporary probe files were removed after execution.

## #220 — undeduced return type hangs

Exact reproducer:

```cpp
#include <meta>
using namespace std::meta;
struct s_undeducible { auto operator()(); };
consteval {
  static_assert(false,
                display_string_of(type_of(^^s_undeducible::operator())));
}
```

Result: still exceeds 20 seconds without producing a diagnostic. The failure is genuine and
reproducible. Source tracing gives a specific cycle: `type_of` handles the reflected
`FunctionDecl` by desugaring `VD->getType()` even when it contains an undeduced `AutoType`;
the resulting function type enters the printer's function-type renderer; that renderer queries
`return_type_of`, which returns the same undeduced placeholder type, and rendering that type
re-enters the same machinery.

P2996R13's `has-type` wording excludes a function whose type contains an undeduced placeholder,
and `type_of` is constant only when `has-type` is true. A guard in the `type_of` declaration
case, before `makeReflection(QT)`, is the likely design direction. No patch was made: this is
shared AST/evaluator machinery, and the machine had only about 1.9 GiB immediately free after
the probe, below the documented safe rebuild condition. Therefore there is no claimed fix or
verification gate.

## #237 — closure type alias

Exact probe:

```cpp
#include <meta>
using namespace std::meta;
consteval {
  constexpr auto closure = []() {};
  constexpr auto cr = ^^decltype(closure);
  using ct = typename[:cr:];
  static_assert(is_type_alias(^^ct));
}
```

Result: current compiler emits `'auto' not allowed in type alias` at the splice, while the
ordinary named-type controls in the upstream probe work. This is not Not-Applicable. The
adopted splice-type wording says a `typename` splice-specifier shall designate a type, class
template, or alias template and designates the same entity. The wording also gives a type-only
context example where `using alias = [:^^S::type:]` is valid. It does not exclude closure types.
The defect is therefore closure/invented-type reconstruction in the alias-declaration path,
consistent with the existing “deeper alias/type work” note. No narrow safe fix was identified.

## #188 — `display_string_of(dealias(...))`

Reconstructed the full `std::ranges::max_element` reproducer. Current HEAD rejects exactly the
five reported calls: `1.2`, `2.2`, `2.3`, `3.3`, and `4.3`. Direct displays (`1.1`, `2.1`,
`3.1`, `4.1`) and the single-dealias controls (`3.2`, `4.2`) compile. The diagnostic bottoms
out at the library printer's `pretty_printer::print` call to `reflect_invoke(^^tprint, ...)`;
there is no more specific evaluator diagnostic.

The implementation path is `dealias`/`underlying_entity_of` → canonicalized type reflection →
`tprint_impl::render`. Template-heavy iterator specializations cause further constant-evaluated
queries during `is_function_type`, template-argument rendering, and recursive `render` calls.
The evidence localizes the problem to evaluator support for this printer/metafunction sequence,
not to ranges or overload resolution. No source change was made because a narrow fix could not
be identified and this printer area has produced multiple false gap claims in this epic.

## Disposition

All three remain Confirmed-Open and are marked deferred in `docs/REFLECTION_GAPS.md`. No fixes
were attempted, so the required post-fix full Clang and libc++ reflection suites were not run.
