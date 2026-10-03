//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// Regression for #185: exceptions must be catchable during constant evaluation.
#include <meta>
#include <array>
#include <vector>
#include <tuple>
#include <variant>
using namespace std::meta;
template<class Probe> consteval bool throws_exception(Probe probe) {
  try { probe(); } catch (const exception&) { return true; }
  return false;
}
template<class Probe> consteval bool returns_normally(Probe probe) {
  try { probe(); } catch (const exception&) { return false; }
  return true;
}
#define THROWS(...) static_assert(throws_exception([] consteval { (void)(__VA_ARGS__); }))
#define NORMAL(...) static_assert(returns_normally([] consteval { (void)(__VA_ARGS__); }))
namespace NS {}
struct Incomplete;
struct C { int member; unsigned bits : 3; static int sm; void mf(); };
struct Other { int member; };
struct Op { bool operator==(const Op&) const; };
struct Abstract { virtual void f() = 0; };
struct VirtualAbstract : virtual Abstract {};
enum class E { a };
enum class Opaque : int;
int global;
int& ref = global;
constexpr int value = 42;
[[maybe_unused]] constexpr int array[] = {1, 2};
void fn([[maybe_unused]] int parameter) {}
void noargs() {}
template<class> struct Box {};
template<class T> requires (sizeof(T) == 1) struct Small {};
template<class T> auto undeduced();
constexpr auto unchecked = access_context::unchecked();
constexpr auto closure = [] {};
struct Ptr { const char* p; };

struct ThrowBegin {
  using value_type = int;
  constexpr int* begin() { if consteval { throw exception("range", ^^int); } else { return nullptr; } }
  constexpr int* end() { return nullptr; }
};
struct ThrowCopy {
  bool fail;
  constexpr ThrowCopy(bool b) : fail(b) {}
  constexpr ThrowCopy(const ThrowCopy& other) : fail(other.fail) {
    if consteval { if (fail) throw exception("copy", ^^int); }
  }
};

extern int& unknown_reference;
thread_local int thread_object;
auto pending();
[[maybe_unused]] constexpr const int* const_pointer = &value;
struct ConstMember { const int member; };
void noexcept_fn() noexcept {}

// Non-throwing controls for facilities without a Throws element.
NORMAL(display_string_of(info{}));
NORMAL(u8display_string_of(info{}));
NORMAL(source_location_of(info{}));
NORMAL(dealias(info{}));
NORMAL(current_namespace());
struct Generated;
consteval { (void)define_aggregate(^^Generated, {data_member_spec(^^int, {.name="m"})}); }
NORMAL(size_of(^^Generated));

// Clause 2:
// \throws
// \tcode{meta::exception} unless
// \tcode{r} represents an operator function or operator function template.
THROWS(operator_of(^^int));
NORMAL(operator_of(^^Op::operator==));

// Clause 3:
// \throws
// \tcode{meta::exception} unless
// the value of \tcode{op} corresponds to one of the enumerators in \tcode{operators}.
THROWS(symbol_of(static_cast<operators>(9999)));
NORMAL(symbol_of(operators::op_plus));
THROWS(u8symbol_of(static_cast<operators>(9999)));
NORMAL(u8symbol_of(operators::op_plus));

// Clause 4:
// \throws
// \tcode{meta::exception} unless
// \tcode{has_identifier(r)} is \tcode{true}
// and the identifier that would be returned (see above)
// is representable by $E$.
THROWS(identifier_of(info{}));
NORMAL(identifier_of(^^C));
THROWS(u8identifier_of(info{}));
NORMAL(u8identifier_of(^^C));
THROWS(identifier_of(reflect_constant(1)));
THROWS(u8identifier_of(reflect_constant(1)));
THROWS(identifier_of(data_member_spec(^^int, {.bit_width=1})));
THROWS(u8identifier_of(data_member_spec(^^int, {.bit_width=1})));

// Clause 5:
// \throws
// \tcode{meta::exception} unless
// \tcode{\exposid{has-type}(r)} is \tcode{true}.
THROWS(type_of(^^int));
NORMAL(type_of(^^global));
THROWS(type_of(^^pending));

// Clause 6:
// \throws
// \tcode{meta::exception} unless
// \tcode{r} is a reflection representing either
// \begin{itemize}
// \item
//   an object with static storage duration\iref{basic.stc.general}, or
// \item
//   a variable that either declares or refers to such an object,
//   and if that variable is a reference $R$, then either
//   \begin{itemize}
//   \item
//     $R$ is usable in constant expressions\iref{expr.const.init}, or
//   \item
//     the lifetime of $R$ began within the core constant expression
//     currently under evaluation.
//   \end{itemize}
// \end{itemize}
THROWS(object_of(^^int));
static_assert(throws_exception([] consteval { int local = 0; (void)object_of(^^local); }));
NORMAL(object_of(^^global));
THROWS(object_of(^^thread_object));
static_assert(returns_normally([] consteval { int& local = global; (void)object_of(^^local); }));

// Clause 7:
// \throws
// \tcode{meta::exception} unless
// either \tcode{r} represents an annotation or
// \tcode{[: $R$ :]} is a valid
// \grammarterm{splice-expression}\iref{expr.prim.splice}.
THROWS(constant_of(^^int));
NORMAL(constant_of(^^value));

// Clause 8:
// \throws
// \tcode{meta::exception} unless
// \tcode{has_parent(r)} is \tcode{true}.
THROWS(parent_of(^^int));
NORMAL(parent_of(^^C));

// Clause 9:
// \throws
// \tcode{meta::exception} unless
// \tcode{has_template_arguments(r)} is \tcode{true}.
THROWS(template_of(^^int));
NORMAL(template_of(^^Box<int>));

// Clause 10:
// \throws
// \tcode{meta::exception} unless
// \tcode{has_template_arguments(r)} is \tcode{true}.
THROWS(template_arguments_of(^^int));
NORMAL(template_arguments_of(^^Box<int>));

// Clause 11:
// \throws
// \tcode{meta::exception} unless
// \tcode{r} represents a function or a function type.
THROWS(parameters_of(^^int));
NORMAL(parameters_of(^^fn));
NORMAL(parameters_of(^^void(int)));

// Clause 12:
// \throws
// \tcode{meta::exception} unless
// \begin{itemize}
// \item
//   \tcode{r} represents a parameter of a function $F$ and
// \item
//   there is a point $P$ in the evaluation context
//   for which the innermost non-block scope enclosing $P$
//   is the function parameter scope\iref{basic.scope.param}
//   associated with $F$.
// \end{itemize}
THROWS(variable_of(^^int));
THROWS(variable_of(parameters_of(^^fn)[0]));
consteval bool parameter_control([[maybe_unused]] int p) { try { (void)variable_of(parameters_of(^^parameter_control)[0]); } catch (const exception&) { return false; } return true; }
static_assert(parameter_control(0));

// Clause 13:
// \throws
// \tcode{meta::exception} unless
// either \tcode{r} represents a function
// and \tcode{\exposid{has-type}(r)} is \tcode{true}
// or \tcode{r} represents a function type.
THROWS(return_type_of(^^int));
NORMAL(return_type_of(^^fn));
NORMAL(return_type_of(^^void(int)));

// Clause 14:
// \throws
// \tcode{meta::exception} unless $S$ represents a function.
consteval { bool caught = false; try { (void)current_function(); } catch (const exception&) { caught = true; } if (!caught) throw 0; }
NORMAL(current_function());

// Clause 15:
// \throws
// \tcode{meta::exception} unless $S$ represents
// either a class or a member function.
consteval bool no_current_class() { try { (void)current_class(); } catch (const exception&) { return true; } return false; }
static_assert(no_current_class());
struct ScopeControl { static consteval bool test() { try { (void)current_class(); } catch (const exception&) { return false; } return true; } };
static_assert(ScopeControl::test());

// Clause 16:
// \throws
// \tcode{meta::exception} unless
// \tcode{cls} is either the null reflection
// or a reflection of a complete class type.
THROWS(unchecked.via(^^int));
THROWS(unchecked.via(^^Incomplete));
NORMAL(unchecked.via(info{}));
NORMAL(unchecked.via(^^C));
THROWS(unchecked.via(^^E));

// Clause 17:
// \throws
// \tcode{meta::exception} if
//   \tcode{r} represents a class member
//   for which \tcode{\exposid{PARENT-CLS}(r)} is an incomplete class.
consteval bool inaccessible_incomplete(info r) { try { (void)is_accessible(r, unchecked); } catch (const exception&) { return true; } return false; }
struct InProgress { int member; static_assert(inaccessible_incomplete(^^InProgress::member)); };
NORMAL(is_accessible(^^C::member, unchecked));

// Clause 18:
// \throws
// \tcode{meta::exception} if
// \begin{itemize}
// \item
//   the evaluation of
//   \tcode{nonstatic_data_members_of(r, access_context::unchecked())}
//   would exit via an exception or
// \item
//   \tcode{r} represents a closure type.
// \end{itemize}
THROWS(has_inaccessible_nonstatic_data_members(^^int, unchecked));
THROWS(has_inaccessible_nonstatic_data_members(^^Incomplete, unchecked));
NORMAL(has_inaccessible_nonstatic_data_members(^^C, unchecked));
THROWS(has_inaccessible_nonstatic_data_members(^^decltype(closure), unchecked));
THROWS(has_inaccessible_subobjects(^^int, unchecked));
THROWS(has_inaccessible_subobjects(^^Incomplete, unchecked));
THROWS(has_inaccessible_subobjects(^^decltype(closure), unchecked));
NORMAL(has_inaccessible_subobjects(^^C, unchecked));

// Clause 19:
// \throws
// \tcode{meta::exception} if the evaluation of
// \tcode{bases_of(r, access_context::unchecked())}
// would exit via an exception.
THROWS(has_inaccessible_bases(^^int, unchecked));
THROWS(has_inaccessible_bases(^^Incomplete, unchecked));
NORMAL(has_inaccessible_bases(^^C, unchecked));

// Clause 20:
// \throws
// \tcode{meta::exception} unless
// \tcode{dealias(r)} is a reflection representing either
// a class type that is complete from some point in the evaluation context
// or a namespace.
THROWS(members_of(^^int, unchecked));
THROWS(members_of(^^Incomplete, unchecked));
NORMAL(members_of(^^C, unchecked));
NORMAL(members_of(^^NS, unchecked));

// Clause 21:
// \throws
// \tcode{meta::exception} unless
// \tcode{dealias(type)} represents a class type
// that is complete from some point in the evaluation context.
THROWS(bases_of(^^int, unchecked));
THROWS(bases_of(^^Incomplete, unchecked));
NORMAL(bases_of(^^C, unchecked));

// Clause 22:
// \throws
// \tcode{meta::exception} unless
// \tcode{dealias(type)} represents a class type
// that is complete from some point in the evaluation context.
THROWS(static_data_members_of(^^int, unchecked));
THROWS(static_data_members_of(^^Incomplete, unchecked));
NORMAL(static_data_members_of(^^C, unchecked));

// Clause 23:
// \throws
// \tcode{meta::exception} unless
// \tcode{dealias(type)} represents a class type
// that is complete from some point in the evaluation context.
THROWS(nonstatic_data_members_of(^^int, unchecked));
THROWS(nonstatic_data_members_of(^^Incomplete, unchecked));
NORMAL(nonstatic_data_members_of(^^C, unchecked));

// Clause 24:
// \throws
// \tcode{meta::exception} unless
// \tcode{dealias(type)} represents a class type
// that is complete from some point in the evaluation context.
THROWS(subobjects_of(^^int, unchecked));
THROWS(subobjects_of(^^Incomplete, unchecked));
NORMAL(subobjects_of(^^C, unchecked));

// Clause 25:
// \throws
// \tcode{meta::exception} unless
// \tcode{dealias(type_enum)} represents an enumeration type,
// and \tcode{is_enumerable_type(\brk{}type_enum)} is \tcode{true}.
THROWS(enumerators_of(^^int));
THROWS(enumerators_of(^^Opaque));
NORMAL(enumerators_of(^^E));

// Clause 26:
// \throws
// \tcode{meta::exception} unless
// \tcode{r} represents a non-static data member,
// unnamed bit-field, or
// direct base class relationship $(D, B)$
// for which either $B$ is not a virtual base class
// or $D$ is not an abstract class.
THROWS(offset_of(^^int));
THROWS(offset_of(bases_of(^^VirtualAbstract, unchecked)[0]));
NORMAL(offset_of(^^C::member));

// Clause 27:
// \throws
// \tcode{meta::exception} unless
// all of the following conditions are met:
// \begin{itemize}
// \item
// \tcode{dealias(r)} is a reflection of a
// type,
// object,
// value,
// variable of non-reference type,
// non-static data member that is not a bit-field,
// direct base class relationship, or
// data member description $(T, N, A, W, \mathit{NUA}, \mathit{ANN})$\iref{class.mem.general}
// where $W$ is $\bot$.
// \item
// If \tcode{dealias(r)} represents a type,
// then \tcode{is_complete_type(r)} is \tcode{true}.
// \end{itemize}
THROWS(size_of(^^NS));
THROWS(size_of(^^Incomplete));
THROWS(size_of(^^ref));
THROWS(size_of(^^C::bits));
THROWS(size_of(data_member_spec(^^int, {.name="b", .bit_width=3})));
THROWS(size_of(data_member_spec(^^int, {.bit_width=0})));
static_assert(size_of(data_member_spec(^^int, {.name="b"})) == sizeof(int));
NORMAL(size_of(^^int));
THROWS(size_of(^^void));

// Clause 28:
// \throws
// \tcode{meta::exception} unless
// all of the following conditions are met:
// \begin{itemize}
// \item
// \tcode{dealias(r)} is a reflection of a
// type,
// object,
// variable of non-reference type,
// non-static data member that is not a bit-field,
// direct base class relationship, or
// data member description
// $(T, N, A, W, \mathit{NUA}, \mathit{ANN})$\iref{class.mem.general}
// where $W$ is $\bot$.
// \item
// If \tcode{dealias(r)} represents a type,
// then \tcode{is_complete_type(r)} is \tcode{true}.
// \end{itemize}
THROWS(alignment_of(^^NS));
THROWS(alignment_of(^^Incomplete));
THROWS(alignment_of(^^ref));
THROWS(alignment_of(^^C::bits));
THROWS(alignment_of(data_member_spec(^^int, {.name="b", .bit_width=3})));
THROWS(alignment_of(data_member_spec(^^int, {.bit_width=0})));
static_assert(alignment_of(data_member_spec(^^int, {.name="b"})) == alignof(int));
NORMAL(alignment_of(^^int));
THROWS(alignment_of(reflect_constant(1)));
THROWS(alignment_of(^^void));

// Clause 29:
// \throws
// \tcode{meta::exception} unless
// all of the following conditions are met:
// \begin{itemize}
// \item
// \tcode{dealias(r)} is a reflection of a
// type,
// object,
// value,
// variable of non-reference type,
// non-static data member,
// unnamed bit-field,
// direct base class relationship, or
// data member description.
// \item
// If \tcode{dealias(r)} represents a type,
// then \tcode{is_complete_type(r)} is \tcode{true}.
// \end{itemize}
THROWS(bit_size_of(^^NS));
THROWS(bit_size_of(^^Incomplete));
THROWS(bit_size_of(^^ref));
NORMAL(bit_size_of(^^int));
THROWS(bit_size_of(^^void));
static_assert(is_bit_field(data_member_spec(^^int, {.name="b", .bit_width=3})));
static_assert(!is_bit_field(data_member_spec(^^int, {.name="b"})));
static_assert(bit_size_of(data_member_spec(^^int, {.name="b", .bit_width=3})) == 3);
static_assert(bit_size_of(data_member_spec(^^int, {.bit_width=0})) == 0);

// Clause 30:
// \throws
// \tcode{meta::exception} unless
// \tcode{item} represents a
// type,
// type alias,
// variable,
// function,
// function parameter,
// namespace,
// enumerator,
// direct base class relationship, or
// non-static data member.
THROWS(annotations_of(info{}));
NORMAL(annotations_of(^^C));

// Clause 31:
// \throws
// \tcode{meta::exception} unless
// \begin{itemize}
// \item
//   the evaluation of
//   \tcode{annotations_of(item)} would not exit via an exception and
// \item
//   \tcode{dealias(type)} represents a type and
//   \tcode{is_complete_type(type)} is \tcode{true}.
// \end{itemize}
THROWS(annotations_of_with_type(info{}, ^^int));
THROWS(annotations_of_with_type(^^C, info{}));
THROWS(annotations_of_with_type(^^C, ^^Incomplete));
NORMAL(annotations_of_with_type(^^C, ^^int));

// Clause 41:
// Every function and function template declared in this subclause
// throws an exception of type \tcode{meta::exception}
// unless the following conditions are met:
// \begin{itemize}
// \item
//   For every parameter \tcode{p} of type \tcode{info},
//   \tcode{is_type(p)} is \tcode{true}.
// \item
//   For every parameter \tcode{r}
//   whose type is constrained on \libconcept{reflection_range},
//   \tcode{ranges::\brk{}all_of(\brk{}r, is_type)} is \tcode{true}.
// \end{itemize}
THROWS(is_void_type(^^NS));
NORMAL(is_void_type(^^int));
THROWS(is_null_pointer_type(^^NS));
NORMAL(is_null_pointer_type(^^int));
THROWS(is_integral_type(^^NS));
NORMAL(is_integral_type(^^int));
THROWS(is_floating_point_type(^^NS));
NORMAL(is_floating_point_type(^^int));
THROWS(is_array_type(^^NS));
NORMAL(is_array_type(^^int));
THROWS(is_pointer_type(^^NS));
NORMAL(is_pointer_type(^^int));
THROWS(is_lvalue_reference_type(^^NS));
NORMAL(is_lvalue_reference_type(^^int));
THROWS(is_rvalue_reference_type(^^NS));
NORMAL(is_rvalue_reference_type(^^int));
THROWS(is_member_object_pointer_type(^^NS));
NORMAL(is_member_object_pointer_type(^^int));
THROWS(is_member_function_pointer_type(^^NS));
NORMAL(is_member_function_pointer_type(^^int));
THROWS(is_enum_type(^^NS));
NORMAL(is_enum_type(^^int));
THROWS(is_union_type(^^NS));
NORMAL(is_union_type(^^int));
THROWS(is_class_type(^^NS));
NORMAL(is_class_type(^^int));
THROWS(is_function_type(^^NS));
NORMAL(is_function_type(^^int));
THROWS(is_reflection_type(^^NS));
NORMAL(is_reflection_type(^^int));
THROWS(is_reference_type(^^NS));
NORMAL(is_reference_type(^^int));
THROWS(is_arithmetic_type(^^NS));
NORMAL(is_arithmetic_type(^^int));
THROWS(is_fundamental_type(^^NS));
NORMAL(is_fundamental_type(^^int));
THROWS(is_object_type(^^NS));
NORMAL(is_object_type(^^int));
THROWS(is_scalar_type(^^NS));
NORMAL(is_scalar_type(^^int));
THROWS(is_compound_type(^^NS));
NORMAL(is_compound_type(^^int));
THROWS(is_member_pointer_type(^^NS));
NORMAL(is_member_pointer_type(^^int));
THROWS(is_const_type(^^NS));
NORMAL(is_const_type(^^int));
THROWS(is_volatile_type(^^NS));
NORMAL(is_volatile_type(^^int));
THROWS(is_trivially_copyable_type(^^NS));
NORMAL(is_trivially_copyable_type(^^int));
THROWS(is_standard_layout_type(^^NS));
NORMAL(is_standard_layout_type(^^int));
THROWS(is_empty_type(^^NS));
NORMAL(is_empty_type(^^int));
THROWS(is_polymorphic_type(^^NS));
NORMAL(is_polymorphic_type(^^int));
THROWS(is_abstract_type(^^NS));
NORMAL(is_abstract_type(^^int));
THROWS(is_final_type(^^NS));
NORMAL(is_final_type(^^int));
THROWS(is_aggregate_type(^^NS));
NORMAL(is_aggregate_type(^^int));
THROWS(is_structural_type(^^NS));
NORMAL(is_structural_type(^^int));
THROWS(is_signed_type(^^NS));
NORMAL(is_signed_type(^^int));
THROWS(is_unsigned_type(^^NS));
NORMAL(is_unsigned_type(^^int));
THROWS(is_bounded_array_type(^^NS));
NORMAL(is_bounded_array_type(^^int));
THROWS(is_unbounded_array_type(^^NS));
NORMAL(is_unbounded_array_type(^^int));
THROWS(is_scoped_enum_type(^^NS));
NORMAL(is_scoped_enum_type(^^int));
THROWS(is_constructible_type(^^NS, {^^int}));
THROWS(is_constructible_type(^^int, {^^NS}));
NORMAL(is_constructible_type(^^int, {^^int}));
THROWS(is_default_constructible_type(^^NS));
NORMAL(is_default_constructible_type(^^int));
THROWS(is_copy_constructible_type(^^NS));
NORMAL(is_copy_constructible_type(^^int));
THROWS(is_move_constructible_type(^^NS));
NORMAL(is_move_constructible_type(^^int));
THROWS(is_assignable_type(^^NS, ^^int));
THROWS(is_assignable_type(^^int, ^^NS));
NORMAL(is_assignable_type(^^int, ^^int));
THROWS(is_copy_assignable_type(^^NS));
NORMAL(is_copy_assignable_type(^^int));
THROWS(is_move_assignable_type(^^NS));
NORMAL(is_move_assignable_type(^^int));
THROWS(is_swappable_with_type(^^NS, ^^int));
THROWS(is_swappable_with_type(^^int, ^^NS));
NORMAL(is_swappable_with_type(^^int, ^^int));
THROWS(is_swappable_type(^^NS));
NORMAL(is_swappable_type(^^int));
THROWS(is_destructible_type(^^NS));
NORMAL(is_destructible_type(^^int));
THROWS(is_trivially_constructible_type(^^NS, {^^int}));
THROWS(is_trivially_constructible_type(^^int, {^^NS}));
NORMAL(is_trivially_constructible_type(^^int, {^^int}));
THROWS(is_trivially_default_constructible_type(^^NS));
NORMAL(is_trivially_default_constructible_type(^^int));
THROWS(is_trivially_copy_constructible_type(^^NS));
NORMAL(is_trivially_copy_constructible_type(^^int));
THROWS(is_trivially_move_constructible_type(^^NS));
NORMAL(is_trivially_move_constructible_type(^^int));
THROWS(is_trivially_assignable_type(^^NS, ^^int));
THROWS(is_trivially_assignable_type(^^int, ^^NS));
NORMAL(is_trivially_assignable_type(^^int, ^^int));
THROWS(is_trivially_copy_assignable_type(^^NS));
NORMAL(is_trivially_copy_assignable_type(^^int));
THROWS(is_trivially_move_assignable_type(^^NS));
NORMAL(is_trivially_move_assignable_type(^^int));
THROWS(is_trivially_destructible_type(^^NS));
NORMAL(is_trivially_destructible_type(^^int));
THROWS(is_nothrow_constructible_type(^^NS, {^^int}));
THROWS(is_nothrow_constructible_type(^^int, {^^NS}));
NORMAL(is_nothrow_constructible_type(^^int, {^^int}));
THROWS(is_nothrow_default_constructible_type(^^NS));
NORMAL(is_nothrow_default_constructible_type(^^int));
THROWS(is_nothrow_copy_constructible_type(^^NS));
NORMAL(is_nothrow_copy_constructible_type(^^int));
THROWS(is_nothrow_move_constructible_type(^^NS));
NORMAL(is_nothrow_move_constructible_type(^^int));
THROWS(is_nothrow_assignable_type(^^NS, ^^int));
THROWS(is_nothrow_assignable_type(^^int, ^^NS));
NORMAL(is_nothrow_assignable_type(^^int, ^^int));
THROWS(is_nothrow_copy_assignable_type(^^NS));
NORMAL(is_nothrow_copy_assignable_type(^^int));
THROWS(is_nothrow_move_assignable_type(^^NS));
NORMAL(is_nothrow_move_assignable_type(^^int));
THROWS(is_nothrow_swappable_with_type(^^NS, ^^int));
THROWS(is_nothrow_swappable_with_type(^^int, ^^NS));
NORMAL(is_nothrow_swappable_with_type(^^int, ^^int));
THROWS(is_nothrow_swappable_type(^^NS));
NORMAL(is_nothrow_swappable_type(^^int));
THROWS(is_nothrow_destructible_type(^^NS));
NORMAL(is_nothrow_destructible_type(^^int));
THROWS(is_implicit_lifetime_type(^^NS));
NORMAL(is_implicit_lifetime_type(^^int));
THROWS(has_virtual_destructor(^^NS));
NORMAL(has_virtual_destructor(^^int));
THROWS(has_unique_object_representations(^^NS));
NORMAL(has_unique_object_representations(^^int));
THROWS(reference_constructs_from_temporary(^^NS, ^^int));
THROWS(reference_constructs_from_temporary(^^int, ^^NS));
NORMAL(reference_constructs_from_temporary(^^int, ^^int));
THROWS(reference_converts_from_temporary(^^NS, ^^int));
THROWS(reference_converts_from_temporary(^^int, ^^NS));
NORMAL(reference_converts_from_temporary(^^int, ^^int));
THROWS(is_same_type(^^NS, ^^int));
THROWS(is_same_type(^^int, ^^NS));
NORMAL(is_same_type(^^int, ^^int));
THROWS(is_base_of_type(^^NS, ^^int));
THROWS(is_base_of_type(^^int, ^^NS));
NORMAL(is_base_of_type(^^int, ^^int));
THROWS(is_virtual_base_of_type(^^NS, ^^int));
THROWS(is_virtual_base_of_type(^^int, ^^NS));
NORMAL(is_virtual_base_of_type(^^int, ^^int));
THROWS(is_convertible_type(^^NS, ^^int));
THROWS(is_convertible_type(^^int, ^^NS));
NORMAL(is_convertible_type(^^int, ^^int));
THROWS(is_nothrow_convertible_type(^^NS, ^^int));
THROWS(is_nothrow_convertible_type(^^int, ^^NS));
NORMAL(is_nothrow_convertible_type(^^int, ^^int));
THROWS(is_layout_compatible_type(^^NS, ^^int));
THROWS(is_layout_compatible_type(^^int, ^^NS));
NORMAL(is_layout_compatible_type(^^int, ^^int));
THROWS(is_pointer_interconvertible_base_of_type(^^NS, ^^int));
THROWS(is_pointer_interconvertible_base_of_type(^^int, ^^NS));
NORMAL(is_pointer_interconvertible_base_of_type(^^int, ^^int));
THROWS(is_invocable_type(^^NS, {^^int}));
THROWS(is_invocable_type(^^void(int), {^^NS}));
NORMAL(is_invocable_type(^^void(int), {^^int}));
THROWS(is_invocable_r_type(^^NS, ^^void(int), {^^int}));
THROWS(is_invocable_r_type(^^void, ^^NS, {^^int}));
THROWS(is_invocable_r_type(^^void, ^^void(int), {^^NS}));
NORMAL(is_invocable_r_type(^^void, ^^void(int), {^^int}));
THROWS(is_nothrow_invocable_type(^^NS, {^^int}));
THROWS(is_nothrow_invocable_type(^^void(int), {^^NS}));
NORMAL(is_nothrow_invocable_type(^^void(int), {^^int}));
THROWS(is_nothrow_invocable_r_type(^^NS, ^^void(int), {^^int}));
THROWS(is_nothrow_invocable_r_type(^^void, ^^NS, {^^int}));
THROWS(is_nothrow_invocable_r_type(^^void, ^^void(int), {^^NS}));
NORMAL(is_nothrow_invocable_r_type(^^void, ^^void(int), {^^int}));
THROWS(remove_const(^^NS));
NORMAL(remove_const(^^int));
THROWS(remove_volatile(^^NS));
NORMAL(remove_volatile(^^int));
THROWS(remove_cv(^^NS));
NORMAL(remove_cv(^^int));
THROWS(add_const(^^NS));
NORMAL(add_const(^^int));
THROWS(add_volatile(^^NS));
NORMAL(add_volatile(^^int));
THROWS(add_cv(^^NS));
NORMAL(add_cv(^^int));
THROWS(remove_reference(^^NS));
NORMAL(remove_reference(^^int));
THROWS(add_lvalue_reference(^^NS));
NORMAL(add_lvalue_reference(^^int));
THROWS(add_rvalue_reference(^^NS));
NORMAL(add_rvalue_reference(^^int));
THROWS(make_signed(^^NS));
NORMAL(make_signed(^^int));
THROWS(make_unsigned(^^NS));
NORMAL(make_unsigned(^^int));
THROWS(remove_extent(^^NS));
NORMAL(remove_extent(^^int));
THROWS(remove_all_extents(^^NS));
NORMAL(remove_all_extents(^^int));
THROWS(remove_pointer(^^NS));
NORMAL(remove_pointer(^^int));
THROWS(add_pointer(^^NS));
NORMAL(add_pointer(^^int));
THROWS(remove_cvref(^^NS));
NORMAL(remove_cvref(^^int));
THROWS(decay(^^NS));
NORMAL(decay(^^int));
THROWS(common_type({^^NS}));
NORMAL(common_type({^^int}));
THROWS(common_reference({^^NS}));
NORMAL(common_reference({^^int}));
THROWS(underlying_type(^^NS));
NORMAL(underlying_type(^^E));
THROWS(invoke_result(^^NS, {^^int}));
THROWS(invoke_result(^^void(int), {^^NS}));
NORMAL(invoke_result(^^void(int), {^^int}));
THROWS(unwrap_reference(^^NS));
NORMAL(unwrap_reference(^^int));
THROWS(unwrap_ref_decay(^^NS));
NORMAL(unwrap_ref_decay(^^int));

// Clause 42:
// For a function or function template $F$ defined in this subclause,
// let $C$ be its associated class template.
// For the evaluation of a call to $F$,
// let $S$ be the specialization of $C$ in terms of which the call is specified.
// \begin{itemize}
// \item
//   If
//   \begin{itemize}
//   \item
//     the template arguments of $S$ violate a condition specified
//     in a \Fundescx{Mandates} element in the specification of $C$;
//   \item
//     the call is specified to produce a reflection of a type,
//     but $S$ would have no member named \tcode{type}; or
//   \item
//     the call is specified to return \tcode{$S$::value},
//     but that expression would not be a valid converted constant expression of type \tcode{R},
//     where \tcode{R} is the return type of $F$;
//   \end{itemize}
//   then an exception of type \tcode{meta::exception} is thrown.
//   \begin{note}
//   For the first case, $S$ is not instantiated.
//   \end{note}
//   \item
//     Otherwise, if the instantiation of $S$ would result in undefined behavior
//     due to dependence on an incomplete type\iref{meta.rqmts},
//     then the call is not a constant subexpression.
//   \item
//     Otherwise, if the template arguments of $S$ do not meet the preconditions of $C$,
//     then it is unspecified whether the call is a constant subexpression.
//     If it is, the call produces the result
//     that would be produced if $C$ had no preconditions.
// \end{itemize}
THROWS(make_signed(^^bool));
NORMAL(make_signed(^^int));
THROWS(make_unsigned(^^float));
NORMAL(make_unsigned(^^int));
THROWS(underlying_type(^^int));
NORMAL(underlying_type(^^E));
THROWS(common_type({^^int, ^^void}));
NORMAL(common_type({^^int, ^^long}));
THROWS(common_reference({^^int, ^^void}));
NORMAL(common_reference({^^int, ^^long}));
THROWS(invoke_result(^^int, {}));
NORMAL(invoke_result(^^void(), {}));
THROWS(tuple_size(^^int));
NORMAL(tuple_size(^^std::tuple<int>));
THROWS(tuple_element(1, ^^std::tuple<int>));
NORMAL(tuple_element(0, ^^std::tuple<int>));
THROWS(variant_size(^^int));
NORMAL(variant_size(^^std::variant<int>));
THROWS(variant_alternative(1, ^^std::variant<int>));
NORMAL(variant_alternative(0, ^^std::variant<int>));

// Additional trailing type-trait wrappers:
// Every function and function template declared in this subclause
// throws an exception of type \tcode{meta::exception}
// unless the following conditions are met:
// \begin{itemize}
// \item
//   For every parameter \tcode{p} of type \tcode{info},
//   \tcode{is_type(p)} is \tcode{true}.
// \item
//   For every parameter \tcode{r}
//   whose type is constrained on \libconcept{reflection_range},
//   \tcode{ranges::\brk{}all_of(\brk{}r, is_type)} is \tcode{true}.
// \end{itemize}
THROWS(rank(^^NS));
NORMAL(rank(^^int[2]));
THROWS(extent(^^NS, 0));
NORMAL(extent(^^int[2], 0));
THROWS(tuple_size(^^NS));
NORMAL(tuple_size(^^std::tuple<int>));
THROWS(tuple_element(0, ^^NS));
NORMAL(tuple_element(0, ^^std::tuple<int>));
THROWS(variant_size(^^NS));
NORMAL(variant_size(^^std::variant<int>));
THROWS(variant_alternative(0, ^^NS));
NORMAL(variant_alternative(0, ^^std::variant<int>));
THROWS(type_order(^^NS, ^^int));
NORMAL(type_order(^^int, ^^long));
THROWS(type_order(^^int, ^^NS));

#undef THROWS
#undef NORMAL
int main(int, char**) {}
