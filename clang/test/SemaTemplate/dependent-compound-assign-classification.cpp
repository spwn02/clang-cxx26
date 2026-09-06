// RUN: %clang_cc1 -fsyntax-only -verify -std=c++26 %s
// expected-no-diagnostics

// Regression test: a type-dependent CompoundAssignOperator's value classification
// must agree with how Sema::CreateOverloadedBinOp constructs the dependent
// placeholder node, or Expr::ClassifyImpl's assert(isPRValue()) fires when the
// classifier is invoked pre-instantiation (e.g. via an `auto` NTTP argument's
// DeduceAutoType call). Previously, only the 10 compound-assignment operators
// crashed here; plain assignment and increment/decrement were unaffected.

template <auto X>
struct Constant {};

template <class T, class R>
auto add_assign(T, R) -> Constant<(T::value += R::value)>;
template <class T, class R>
auto sub_assign(T, R) -> Constant<(T::value -= R::value)>;
template <class T, class R>
auto mul_assign(T, R) -> Constant<(T::value *= R::value)>;
template <class T, class R>
auto div_assign(T, R) -> Constant<(T::value /= R::value)>;
template <class T, class R>
auto mod_assign(T, R) -> Constant<(T::value %= R::value)>;
template <class T, class R>
auto and_assign(T, R) -> Constant<(T::value &= R::value)>;
template <class T, class R>
auto or_assign(T, R) -> Constant<(T::value |= R::value)>;
template <class T, class R>
auto xor_assign(T, R) -> Constant<(T::value ^= R::value)>;
template <class T, class R>
auto shl_assign(T, R) -> Constant<(T::value <<= R::value)>;
template <class T, class R>
auto shr_assign(T, R) -> Constant<(T::value >>= R::value)>;

// Negative controls: must keep compiling.
template <class T, class R>
auto plain_assign(T, R) -> Constant<(T::value = R::value)>;
template <class T>
auto pre_inc(T) -> Constant<(++T::value)>;
template <class T>
auto pre_dec(T) -> Constant<(--T::value)>;
template <class T, class R>
auto plain_plus(T, R) -> Constant<(T::value + R::value)>;
