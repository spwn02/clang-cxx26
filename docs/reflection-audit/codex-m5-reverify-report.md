# M5 Reflection Gap Re-verification

Date: 2026-09-10

All library probes below used the staged libc++ headers at
`build-libcxx/libcxx/test-suite-install/include/c++/v1`. The direct command used
`-nostdinc++ -I...` so the staged libc++ headers were selected without the
system C++ headers interfering with libc++'s `include_next` setup. This is the
same staged path requested for the audit. Language-only NEW-2 used the direct
`-cc1` command with `-nostdsysteminc`.

## NEW-4 — row 2996-19

Claim: `has_inaccessible_nonstatic_data_members(r, ctx)` should be ill-formed
when `r` represents a closure type.

Repro (`/tmp/reflection-new4.cpp`):

```cpp
#include <meta>

constexpr auto r = ^^decltype([] {});
constexpr auto c = std::meta::access_context::current();
constexpr bool result = std::meta::has_inaccessible_nonstatic_data_members(r, c);

static_assert(result || !result);
```

Command:

```text
build-nyx/bin/clang++ -nostdinc++ -Ibuild-libcxx/libcxx/test-suite-install/include/c++/v1 -std=c++26 -freflection-latest -fsyntax-only /tmp/reflection-new4.cpp
```

Exact output:

```text
exit 0; no output
```

Verdict: **CONFIRMED**. The call is a required constant evaluation and the
closure reflection is accepted without a diagnostic. The repro uses exactly
the requested `^^decltype([] {})` form.

## NEW-6 — row 2996-48

Claim: splicing constructor and destructor reflections should be ill-formed,
but both were reported as accepted.

Constructor repro (`/tmp/reflection-new6-constructor.cpp`):

```cpp
#include <meta>

struct S {
  S();
  ~S();
};

auto constructor_splice() {
  [:^^S::S:] value;
  return value;
}
```

Exact output:

```text
exit 0; no output
```

Destructor repro (`/tmp/reflection-new6-destructor.cpp`):

```cpp
#include <meta>

struct S {
  S();
  ~S();
};

auto destructor_splice() {
  [:^^S::~S:] value;
  return value;
}
```

Exact output:

```text
/tmp/reflection-new6-destructor.cpp:9:3: error: reflection not usable in a splice type
    9 |   [:^^S::~S:] value;
      |   ^
1 error generated.
```

Verdict: **REFUTED as a combined claim; constructor subclaim CONFIRMED and
destructor subclaim REFUTED**. The constructor reflection is accepted in the
splice-type position, but the destructor reflection is rejected. Thus the
original evidence overgeneralized from a combined probe: there is a real
constructor-splice gap, but not a destructor-splice gap in this direct repro.

## NEW-7 — row 2996-49

Claim: a dependent splice-specifier in the forbidden CTAD-like position should
be ill-formed.

Repro (`/tmp/reflection-new7.cpp`):

```cpp
#include <meta>

struct T {
  constexpr T(int) {}
};

template <std::meta::info R>
consteval auto dependent_ctad() {
  [:R:] value = {1};
  return value;
}

using Dependent = decltype(dependent_ctad<^^T>());
```

Command:

```text
build-nyx/bin/clang++ -nostdinc++ -Ibuild-libcxx/libcxx/test-suite-install/include/c++/v1 -std=c++26 -freflection-latest -fsyntax-only /tmp/reflection-new7.cpp
```

Exact output:

```text
exit 0; no output
```

Verdict: **CONFIRMED**. This is the tracker scenario verbatim: dependent
`[:R:] value = {1}` and a type with a converting `T(int)` constructor. The
instantiation is accepted.

## NEW-8 — row 3096-06

Claim: parameter-only `identifier_of`, `u8identifier_of`, and `has_identifier`
should reject a non-parameter reflection.

Repro (`/tmp/reflection-new8.cpp`):

```cpp
#include <meta>

struct S {};

constexpr auto type_reflection = ^^S;
constexpr auto identifier = std::meta::identifier_of(type_reflection);
constexpr auto u8identifier = std::meta::u8identifier_of(type_reflection);
constexpr bool has = std::meta::has_identifier(type_reflection);
```

Command:

```text
build-nyx/bin/clang++ -nostdinc++ -Ibuild-libcxx/libcxx/test-suite-install/include/c++/v1 -std=c++26 -freflection-latest -fsyntax-only /tmp/reflection-new8.cpp
```

Exact output:

```text
exit 0; no output
```

For completeness, the row also mentions `type_of`. Its separate repro was:

```cpp
#include <meta>

struct S {};

constexpr auto type_reflection = ^^S;
constexpr auto type = std::meta::type_of(type_reflection);
```

Exact `type_of` output (exit 1):

```text
/tmp/reflection-new8-type.cpp:6:16: error: constexpr variable 'type' must be initialized by a constant expression
    6 | constexpr auto type = std::meta::type_of(type_reflection);
      |                ^      ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
build-libcxx/libcxx/test-suite-install/include/c++/v1/meta:998:10: note: subexpression not valid in a constant expression
  998 |   return __metafunction(detail::__metafn_type_of, r);
      |          ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/tmp/reflection-new8-type.cpp:6:23: note: in call to 'type_of(^^(type))'
    6 | constexpr auto type = std::meta::type_of(type_reflection);
      |                       ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/tmp/reflection-new8-type.cpp:6:23: note: a type has no type
    6 | constexpr auto type = std::meta::type_of(type_reflection);
      |                       ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
1 error generated.
```

Verdict: **CONFIRMED for the three named parameter-only queries; REFUTED for
the row's `type_of` subcase**. The three requested functions accept a type
reflection with no output. `type_of(^^S)` already diagnoses because a type has
no type. The original NEW-8 entry correctly identifies a partial enforcement
gap, but its wording should not imply that `type_of` is also accepted.

## NEW-2 — row 3394-03

Claim: an annotation on an empty-declaration is accepted.

Grammar check: an empty-declaration is the standalone `;` production. P3394R4
explicitly gives `[[=123]];` as the error example for an annotation in an
empty-declaration's attribute-specifier-seq.

Repro (`/tmp/reflection-new2.cpp`):

```cpp
[[=1]];
```

Command:

```text
build-nyx/bin/clang -cc1 -internal-isystem build-nyx/lib/clang/22/include -nostdsysteminc -std=c++26 -freflection-latest -fannotation-attributes -fsyntax-only /tmp/reflection-new2.cpp
```

Exact output:

```text
exit 0; no output
```

Verdict: **CONFIRMED**. The exact adopted example is accepted. The earlier
different diagnostic involving mixed attributes is not this case; this probe
contains only the annotation and the empty-declaration semicolon.

## NEW-3 — row 3394-05

Claim: invalid arguments to `annotations_of_with_type(item, type)` should be
ill-formed, but are accepted.

P3394R4 requires `annotations_of(item)` to be a constant subexpression and
`dealias(type)` to represent a complete type. `^^void` is a type reflection but
does not represent a complete type, so it is a minimal invalid second
argument. The fork's function name is the adopted `annotations_of_with_type`.

Repro (`/tmp/reflection-new3.cpp`):

```cpp
#include <meta>

constexpr auto item = ^^int;
constexpr auto type = ^^void;
constexpr auto result = std::meta::annotations_of_with_type(item, type);
```

Command:

```text
build-nyx/bin/clang++ -nostdinc++ -Ibuild-libcxx/libcxx/test-suite-install/include/c++/v1 -std=c++26 -freflection-latest -fsyntax-only /tmp/reflection-new3.cpp
```

Exact output:

```text
exit 0; no output
```

Verdict: **CONFIRMED**. The invalid incomplete type argument is accepted and
produces an empty-result value without a diagnostic. This directly reproduces
the claimed missing precondition enforcement.
