# Investigation: the "SFINAE-class" compiler gaps behind `dependent_sender` and `tag_of_t`

Status: **closed, confirmed permanently blocked at the standard-wording
level**. Not fixable by a Clang patch in this fork. Preserved here so a
future session doesn't re-chase it without first reading this.

## Background

Two separately-documented deviations in this fork were suspected of sharing
one root cause, both described (at the time) as "Clang hard-errors instead
of gracefully SFINAE-ing":

1. `<__execution/get_completion_signatures.h>`'s `dependent_sender_error`
   comment, blocking P3557R3 (and, transitively, P3481R5/P3570R2/P3887R1 —
   see issue #11, closed 2026-09-17) — attributed to "this Clang does not
   yet support throwing an exception from within a consteval function
   ([P3068])."
2. `<__execution/domain.h>`'s `default_domain::transform_sender` comment,
   permanently disabling domain-based sender customization fork-wide —
   attributed to "`tag_of_t`'s auto-return-type body instantiation is not
   in the 'immediate context' of substitution."

Both prior attributions turned out to be **imprecise** (not wrong that
something hard-errors — wrong about *why*). This investigation re-derived
the actual cause empirically, since two previously-closed papers (#11) and
one large piece of unfinished parallel_scheduler work (real domain-based
`bulk()` customization) all hinge on whether either gap is actually
fixable.

## Method: four progressively-narrowed repro programs

All four compiled/run against this fork's own `build-nyx/bin/clang++`
(`clang version 22.1.8`, the exact compiler libc++ is built/tested
against in this session) via `clang++ -std=c++26 -fsyntax-only <file>`.

### 1. Does throw/catch work at all inside a single `consteval` function?

```cpp
struct E {};
consteval bool f() {
  try { throw E{}; } catch (E&) { return true; }
  return false;
}
static_assert(f());
int main() { return 0; }
```

**Result: compiles cleanly (exit 0).** This directly falsifies the old
`get_completion_signatures.h` comment's specific claim ("this Clang does
not yet support" throwing from consteval at all) — a throw *caught within*
a consteval function's own body works fine.

### 2. Does an *uncaught* throw escaping into a `requires{}` nested-requirement
gracefully evaluate to "not satisfied" (the actual mechanism P3557R3's
`is-dependent-sender-helper` needs), or hard-error?

```cpp
struct E {};
template <bool Throw>
consteval bool maybe_throw() {
  if constexpr (Throw) { throw E{}; }
  return true;
}
template <bool Throw>
concept satisfied_unless_throws = requires {
  requires maybe_throw<Throw>();
};
static_assert(satisfied_unless_throws<false>);
static_assert(!satisfied_unless_throws<true>);
```

**Result: hard error** — `error: substitution into constraint expression
resulted in a non-constant expression`, with a note "exception thrown here
was not caught within the constant expression." This is the precise shape
P3557R3's mechanism needs, and it fails.

### 3. Is this specifically about `tag_of_t`'s auto-return-type body
instantiation (the `domain.h` comment's claim), or something more general?
Tested by reproducing the *identical* structured-binding decomposition
failure, then again with an explicit (non-`auto`) return type via
reflection, to isolate whether return-type deduction is the actual cause.

```cpp
// (a) structured-binding version, matching sender.h's __sender_tag_of exactly:
template <class T>
constexpr auto get_tag(T&& t) {
  auto&& [tag, data] = t;
  return tag;
}
template <class T>
concept has_tag = requires (T&& t) { get_tag(t); };
static_assert(!has_tag<struct NotDecomposable_with_1_member>); // hard-errors

// (b) reflection version, EXPLICIT size_t return type (no auto at all):
template <class T>
consteval std::size_t member_count() {
  return nonstatic_data_members_of(^^T, access_context::current()).size();
}
template <class T>
concept has_two_members = requires {
  { member_count<T>() } -> std::same_as<std::size_t>;
} && member_count<T>() >= 2;
static_assert(!has_two_members<int>); // reflecting a non-class type
```

**Result: both hard-error identically**, with the reflection version's
error tracing through `nonstatic_data_members_of` → `__make_exception` →
"invalid reflection operand" — an *uncaught exception thrown by library
code*, not a return-type-deduction issue at all. This falsifies the old
`domain.h` comment's specific attribution: an explicit, non-`auto` return
type reproduces the exact same failure. The real cause is the same as (2):
an evaluation failure escaping into atomic-constraint checking.

### 4. Discriminating test: is this Clang's *general* policy for any
non-constant-expression atomic constraint (not specific to exceptions —
i.e., not something a future "P3068-successor" paper would even address),
or does it special-case exceptions specifically?

```cpp
template <bool B>
consteval int div_by_zero() {
  int x = 1;
  int y = B ? 0 : 1;
  return x / y;
}
template <bool B>
concept DivC = div_by_zero<B>() >= 0;
static_assert(DivC<false>);
static_assert(!DivC<true>);
```

**Result: hard-errors identically** to the throwing cases (same diagnostic
text, "division by zero" instead of "exception thrown here"). Exceptions
get no special treatment — this is Clang's uniform policy for *any* reason
an atomic constraint's expression fails to be a constant expression.

## Cross-check against Clang's own pre-existing test suite

`grep -rln "substitution into constraint expression" clang/test/` finds
four files, none touched by this fork, all asserting this exact diagnostic
as *intended*, tested behavior for shapes matching test 4 above (a
non-constexpr function call, a runtime function parameter used where a
constant expression is required):

- `clang/test/CXX/expr/expr.prim/expr.prim.id/p3.cpp` —
  `concept C4 = U::add(1, 2) == 3;` where `add` isn't `constexpr`.
- `clang/test/CXX/expr/expr.prim/expr.prim.req/nested-requirement.cpp` —
  `requires a == 0;` where `a` is a runtime function parameter.
- `clang/test/SemaCXX/cxx23-assume.cpp` — `concept C = f4<T>();`.
- `clang/test/SemaCXX/requires-nested-non-constant.cpp` —
  `requires (x > 0) && (x < 10);` where `x` is a runtime function
  parameter.

This settles it: hard-erroring on a non-constant-expression atomic
constraint is **deliberate, long-tested upstream Clang behavior**, not an
unimplemented rule or a fork-introduced regression. [temp.constr.atomic]'s
graceful "not satisfied" treatment applies to *substitution* failures
(forming an invalid type/expression by plugging in template arguments —
classic SFINAE); it does not extend to an otherwise well-formed expression
that merely fails to *evaluate* as a constant expression, whether via an
escaping exception, division by zero, or anything else. P3557R3's
`is-dependent-sender-helper` mechanism (and, transitively, `tag_of_t`'s
"if well-formed" branch as an equivalent technique) requires the former
kind of failure to be gracefully absorbed; what actually happens is the
latter kind, on every Clang, per Clang's own settled test suite — not
something specific to this fork's build.

## Conclusion

**Both "SFINAE-class" gaps are the same one root cause, and that root
cause is conforming (or at least deliberate, tested, unlikely-to-change)
compiler behavior, not a bug.** Making either work would require new
standard machinery — something like a real "P3068-successor" paper that
gives programmers a way to make an escaping exception from an otherwise
fully-resolvable `consteval` evaluation distinguishable, during atomic
constraint checking, from ordinary non-constant-expression failure. That
doesn't exist in the adopted standard today. There is nothing here for a
future session to patch in this compiler — this is a WG21-level gap, not a
Clang-level one.

This confirms (rather than overturns) the original diagnosis recorded on
issues #10/#11 before this investigation started. The value of this pass
was replacing "a prior session's note" with "four reproducible programs
plus four cross-referenced upstream Clang tests" as the evidence — worth
having if a future compiler version or standard revision changes the
premise, but nothing to act on today.

**Redirect:** the actual highest-leverage currently-open compiler-side
work is #101 and #103 (both already root-caused, bounded scope, no design
work needed) — not this cluster.
