// RUN: %clang_cc1 -std=c++2c -verify %s
// expected-no-diagnostics

template <typename T> concept Small = sizeof(T) <= 4;
template <typename T> constexpr auto Sz = sizeof(T);

template <template <typename> concept C> struct Holder {};
template <template <typename> auto V> struct VHolder {};

// Deduction of a concept / variable template template argument from a
// specialization of a class template.
template <template <typename> auto V, typename T>
constexpr auto ded_var(VHolder<V>, T) { return V<T>; }
static_assert(ded_var(VHolder<Sz>{}, 3.0) == 8);

template <template <typename> concept C>
constexpr bool ded_concept(Holder<C>) { return C<int>; }
static_assert(ded_concept(Holder<Small>{}));

// Overloads differing only by which concept is bound to the template
// template parameter are ordered by constraint subsumption.
template <typename T> concept Big = !Small<T>;
template <template <typename> concept C, typename T> requires C<T>
constexpr int pick(T) { return 0; }
static_assert(pick<Small>(1) == 0);
static_assert(pick<Big>(1LL) == 0);

// Partial specialization on a template template parameter of kind concept.
template <typename T, template <typename> concept C> struct Sel { static constexpr int v = 0; };
template <typename T> struct Sel<T, Small> { static constexpr int v = 1; };
static_assert(Sel<int, Small>::v == 1);
static_assert(Sel<int, Big>::v == 0);
