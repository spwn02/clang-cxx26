// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %s

// __builtin_get_awaiter(c) and __builtin_get_awaiter(c, p) are the awaiter of the operand c of an await-expression in
// a coroutine whose promise object is p ([expr.await]): p.await_transform(c) if the promise type has an await_transform
// member, then the operator co_await chosen by overload resolution among the member and the non-member candidates.

namespace std {
template <class T> T &&declval() noexcept;
template <class A, class B> inline constexpr bool is_same_v = false;
template <class A> inline constexpr bool is_same_v<A, A> = true;
} // namespace std

struct Awaiter {};
struct Other {};
struct None {};

// no promise: no await_transform; the operand without operator co_await is its own awaiter
None make();
static_assert(std::is_same_v<decltype(__builtin_get_awaiter(make())), None>);
static_assert(std::is_same_v<decltype((__builtin_get_awaiter(std::declval<None &>()))), None &>);
static_assert(std::is_same_v<decltype((__builtin_get_awaiter(std::declval<None &&>()))), None &&>);
static_assert(std::is_same_v<decltype(__builtin_get_awaiter(1)), int>);

struct Member { Awaiter operator co_await(); };
struct Free {};
Other operator co_await(Free);
static_assert(std::is_same_v<decltype(__builtin_get_awaiter(std::declval<Member>())), Awaiter>);
static_assert(std::is_same_v<decltype(__builtin_get_awaiter(std::declval<Free>())), Other>);

// both viable and unordered: ambiguous; the call is ill-formed
struct Both { Awaiter operator co_await(); };
Other operator co_await(Both);
template <class T> concept HasAwaiter = requires(T &&t) { __builtin_get_awaiter(static_cast<T &&>(t)); };
static_assert(!HasAwaiter<Both>);
static_assert(HasAwaiter<Member>);
static_assert(HasAwaiter<Free>);
void ambiguous() { __builtin_get_awaiter(std::declval<Both>()); } // expected-error {{use of overloaded operator 'co_await' is ambiguous}}
// expected-note@* 2 {{candidate function}}

// overload resolution ranks the implicit object parameter: a &&-qualified member is better than a non-member taking
// const X& for an rvalue, and the const& non-member is the only candidate for an lvalue.
struct RefQual { Awaiter operator co_await() &&; };
Other operator co_await(const RefQual &);
static_assert(std::is_same_v<decltype(__builtin_get_awaiter(std::declval<RefQual>())), Awaiter>);
static_assert(std::is_same_v<decltype(__builtin_get_awaiter(std::declval<RefQual &>())), Other>);

// await_transform of the promise: the operand is its result, with no fallback to the operand
struct P1 { Free await_transform(int); };
static_assert(std::is_same_v<decltype(__builtin_get_awaiter(1, std::declval<P1 &>())), Other>);
template <class C, class P> concept HasAwaiterFor = requires(C &&c, P &p) { __builtin_get_awaiter(static_cast<C &&>(c), p); };
static_assert(!HasAwaiterFor<None, P1>);
static_assert(HasAwaiterFor<int, P1>);
struct P2 {}; // none: the operand
static_assert(std::is_same_v<decltype(__builtin_get_awaiter(std::declval<Member>(), std::declval<P2 &>())), Awaiter>);

// a final promise type has await_transform members that are found
struct P3 final { Member await_transform(None); };
static_assert(std::is_same_v<decltype(__builtin_get_awaiter(None{}, std::declval<P3 &>())), Awaiter>);
struct P4 final {};
static_assert(std::is_same_v<decltype(__builtin_get_awaiter(None{}, std::declval<P4 &>())), None>);
// a name that is declared in two bases is found (and ambiguous), whatever it declares
struct B1 { int await_transform; };
struct B2 { void await_transform(); };
struct P5 : B1, B2 {};
static_assert(!HasAwaiterFor<None, P5>);
// the member can be a data member that is not callable
static_assert(!HasAwaiterFor<None, B1>);

// an incomplete promise type cannot be searched: the call is ill-formed (not a crash)
struct Incomplete;
static_assert(!HasAwaiterFor<None, Incomplete>);

// the non-member candidates are found by argument-dependent lookup, not from where the call is instantiated: a
// declaration that is only visible at the use does not make the result depend on where the template is first used
namespace NS { struct X {}; Awaiter operator co_await(X); }
namespace NS2 { struct GlobalOnly {}; }
using NS2::GlobalOnly;
Other operator co_await(GlobalOnly); // global namespace: not an associated namespace of NS2::GlobalOnly
template <class C> using Adl_t = decltype(__builtin_get_awaiter(std::declval<C>()));
static_assert(std::is_same_v<Adl_t<NS::X>, Awaiter>);
namespace Hide {
struct Y {};
void operator co_await(Y);
static_assert(std::is_same_v<Adl_t<NS::X>, Awaiter>);
static_assert(std::is_same_v<Adl_t<GlobalOnly>, GlobalOnly &&>); // not visible by ADL: the operand itself
} // namespace Hide
static_assert(std::is_same_v<Adl_t<GlobalOnly>, GlobalOnly &&>);

// a global operator co_await that ADL does not reach does not make the call ill-formed (no search of the scope of the
// instantiation for a candidate "declared too late"), wherever the concept is first satisfied
namespace Base2 { struct B {}; }
namespace N2 { struct D : Base2::B {}; }
int operator co_await(N2::D);
static_assert(HasAwaiter<N2::D>);
namespace Hide2 {
struct Y {};
void operator co_await(Y);
static_assert(HasAwaiter<N2::D>);
} // namespace Hide2

// a dependent use
template <class C, class P> using Awaiter_t = decltype(__builtin_get_awaiter(std::declval<C>(), std::declval<P &>()));
static_assert(std::is_same_v<Awaiter_t<int, P1>, Other>);
static_assert(std::is_same_v<Awaiter_t<Member, P2>, Awaiter>);

void bad() {
  __builtin_get_awaiter(); // expected-error {{too few arguments to function call, expected 1, have 0}}
  __builtin_get_awaiter(1, 2, 3); // expected-error {{too many arguments to function call, expected at most 2, have 3}}
}
