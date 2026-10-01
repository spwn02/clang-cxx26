// RUN: %clang_cc1 -std=c++26 -freflection -verify %s
// RUN: %clang_cc1 -std=c++26 -freflection -DERRORS -verify=errors %s
// RUN: %clang_cc1 -std=c++26 -freflection -emit-llvm -o %t.ll %s
// RUN: %clang_cc1 -std=c++26 -freflection -emit-pch -o %t.pch %s
// RUN: %clang_cc1 -std=c++26 -freflection -include-pch %t.pch -emit-llvm -o %t.pch.ll %s
// RUN: %clang_cc1 -std=c++26 -freflection -include-pch %t.pch -DPCH_USE -verify=pch %s
// expected-no-diagnostics

#ifndef CONSTEVAL_ONLY_VALUES
#define CONSTEVAL_ONLY_VALUES
using info = decltype(^^int);
consteval int plus1(int x) { return x + 1; }
constexpr auto b = plus1;
template <auto F> struct Function {};
auto c = Function<plus1>();

info null;
struct Pointer { const info *p; };
auto pointer = Pointer{.p = nullptr};
struct Aggregate { info r; int n; };
Aggregate aggregate{};
union Union { info r; int n; };
Union active_int{.n = 42};

void local_null() { info i; (void)i; }
void local_constexpr() {
  constexpr info r = ^^int;
  constexpr auto p = plus1;
}
info return_null() { return {}; }
constexpr info reflection = ^^int;
constexpr info reflection_array[] = {info{}, ^^int};
constexpr const void *erased = &reflection;
constexpr auto past = reflection_array + 2;
struct Ref { const info &r; };
constexpr Ref reference{reflection};
struct Member { consteval int f() { return 1; } };
constexpr auto member = &Member::f;
Function<&Member::f> member_argument;
struct Cycle { const Cycle *self; info r; };
constexpr Cycle ordinary_cycle{&ordinary_cycle, {}};
auto ordinary_cycle_copy = ordinary_cycle;

#ifdef ERRORS
constexpr Cycle immediate_cycle{&immediate_cycle, ^^int};
auto immediate_cycle_copy = immediate_cycle; // errors-error {{is not associated with a constexpr variable}}
struct FunctionPointer { int (*p)(int); };
auto aggregate_function = FunctionPointer{plus1}; // errors-error {{is not associated with a constexpr variable}}
consteval void static_escape() { static auto r = ^^int; } // errors-error {{is not associated with a constexpr variable}}
auto a = plus1; // errors-error {{immediate object associated with variable 'a' is not associated with a constexpr variable}}
int (*arr[])(int) = {plus1}; // errors-error {{immediate object associated with variable 'arr' is not associated with a constexpr variable}}
auto nonnull = ^^int; // errors-error {{immediate object associated with variable 'nonnull' is not associated with a constexpr variable}}
auto immediate_pointer = &reflection; // errors-error {{immediate object associated with variable 'immediate_pointer' is not associated with a constexpr variable}}
auto erased_pointer = erased; // errors-error {{immediate object associated with variable 'erased_pointer' is not associated with a constexpr variable}}
auto one_past = past; // errors-error {{immediate object associated with variable 'one_past' is not associated with a constexpr variable}}
Ref runtime_reference{reflection}; // errors-error {{immediate object associated with variable 'runtime_reference' is not associated with a constexpr variable}}
auto member_pointer = &Member::f; // errors-error {{immediate object associated with variable 'member_pointer' is not associated with a constexpr variable}}
info runtime_return() { return ^^int; } // errors-error {{consteval-only value is only allowed in an immediate function context}}
const void *runtime_pointer_return() { return &reflection; } // errors-error {{consteval-only value is only allowed in an immediate function context}}
void assign(info &r) { r = ^^int; } // errors-error {{consteval-only value is only allowed in an immediate function context}}
consteval const void *static_pointer() {
  static constexpr info r = ^^int;
  return &r;
}
constexpr const void *static_erased = static_pointer();
const void *return_static_pointer() { return static_erased; } // errors-error {{consteval-only value is only allowed}}
void take_pointer(const void *);
void pass_static_pointer() { take_pointer(static_erased); } // errors-error {{consteval-only value is only allowed}}
void store_static_pointer(const void *&p) { p = static_erased; } // errors-error {{consteval-only value is only allowed}}
void take_reflection(info);
void pass_reflection() { take_reflection(reflection); } // errors-error {{consteval-only value is only allowed}}
void assign_pointer(const info *&p) { p = &reflection; } // errors-error {{consteval-only value is only allowed in an immediate function context}}
#endif
#endif

#ifdef PCH_USE
auto loaded_pointer = &reflection; // pch-error {{is not associated with a constexpr variable}}
auto loaded_member = member; // pch-error {{is not associated with a constexpr variable}}
#endif
