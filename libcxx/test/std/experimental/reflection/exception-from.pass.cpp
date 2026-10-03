//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// [meta.syn]: "When a function or function template specialization F specified
// in this header throws a meta::exception E, E.from() is a reflection
// representing F".
// The table follows the draft's std::meta synopsis (define_aggregate is below),
// including predicates that cannot throw: those accept the null negative probe.
// Invalid type operands use the global namespace, not an incomplete type (whose
// associated trait can have undefined behavior). Range functions use exactly the
// specialization named in the table. No overloaded name is compared unresolved.
// reflect_function has no invalid function reference obtainable in a core
// constant expression; its ordinary-function control exercises the specialization.
// define_aggregate specifies Constant When, not Throws: invalid inputs are not
// constant expressions. Its positive control defines an empty aggregate.
// current_function has both function-scope and namespace-scope controls.
// current_namespace cannot throw. current_class has no enclosing class here
// and must throw.

#include <meta>
#include <initializer_list>
#include <array>

using namespace std::meta;
using infos = std::initializer_list<info>;
struct pointer_record { const char* p; };
struct generated_aggregate;
void ordinary_function() {}

template <class R, class... A>
consteval info choose(R (&f)(A...)) { return reflect_function(f); }

consteval void probe_current_class() { (void)current_class(); }

template <info Function, bool MustThrow, class Probe>
consteval bool check_from(Probe probe) {
  try {
    probe();
  } catch (std::meta::exception& e) {
    return MustThrow && e.from() == Function;
  }
  return !MustThrow;
}

#define METAFUNCTION_TABLE(X) \
  X(operator_of, true, (info{}), ^^std::meta::operator_of) \
  X(symbol_of, true, (static_cast<operators>(0)), ^^std::meta::symbol_of) \
  X(u8symbol_of, true, (static_cast<operators>(0)), ^^std::meta::u8symbol_of) \
  X(has_identifier, false, (info{}), ^^std::meta::has_identifier) \
  X(identifier_of, true, (info{}), ^^std::meta::identifier_of) \
  X(u8identifier_of, true, (info{}), ^^std::meta::u8identifier_of) \
  X(display_string_of, false, (info{}), ^^std::meta::display_string_of) \
  X(u8display_string_of, false, (info{}), ^^std::meta::u8display_string_of) \
  X(source_location_of, false, (info{}), ^^std::meta::source_location_of) \
  X(type_of, true, (info{}), ^^std::meta::type_of) \
  X(object_of, true, (info{}), ^^std::meta::object_of) \
  X(constant_of, true, (info{}), ^^std::meta::constant_of) \
  X(is_public, false, (info{}), ^^std::meta::is_public) \
  X(is_protected, false, (info{}), ^^std::meta::is_protected) \
  X(is_private, false, (info{}), ^^std::meta::is_private) \
  X(is_virtual, false, (info{}), ^^std::meta::is_virtual) \
  X(is_pure_virtual, false, (info{}), ^^std::meta::is_pure_virtual) \
  X(is_override, false, (info{}), ^^std::meta::is_override) \
  X(is_final, false, (info{}), ^^std::meta::is_final) \
  X(is_deleted, false, (info{}), ^^std::meta::is_deleted) \
  X(is_defaulted, false, (info{}), ^^std::meta::is_defaulted) \
  X(is_user_provided, false, (info{}), ^^std::meta::is_user_provided) \
  X(is_user_declared, false, (info{}), ^^std::meta::is_user_declared) \
  X(is_explicit, false, (info{}), ^^std::meta::is_explicit) \
  X(is_noexcept, false, (info{}), ^^std::meta::is_noexcept) \
  X(is_bit_field, false, (info{}), ^^std::meta::is_bit_field) \
  X(is_enumerator, false, (info{}), ^^std::meta::is_enumerator) \
  X(is_annotation, false, (info{}), ^^std::meta::is_annotation) \
  X(is_const, false, (info{}), ^^std::meta::is_const) \
  X(is_volatile, false, (info{}), ^^std::meta::is_volatile) \
  X(is_mutable_member, false, (info{}), ^^std::meta::is_mutable_member) \
  X(is_lvalue_reference_qualified, false, (info{}), ^^std::meta::is_lvalue_reference_qualified) \
  X(is_rvalue_reference_qualified, false, (info{}), ^^std::meta::is_rvalue_reference_qualified) \
  X(has_static_storage_duration, false, (info{}), ^^std::meta::has_static_storage_duration) \
  X(has_thread_storage_duration, false, (info{}), ^^std::meta::has_thread_storage_duration) \
  X(has_automatic_storage_duration, false, (info{}), ^^std::meta::has_automatic_storage_duration) \
  X(has_internal_linkage, false, (info{}), ^^std::meta::has_internal_linkage) \
  X(has_module_linkage, false, (info{}), ^^std::meta::has_module_linkage) \
  X(has_external_linkage, false, (info{}), ^^std::meta::has_external_linkage) \
  X(has_c_language_linkage, false, (info{}), ^^std::meta::has_c_language_linkage) \
  X(has_linkage, false, (info{}), ^^std::meta::has_linkage) \
  X(is_complete_type, false, (info{}), ^^std::meta::is_complete_type) \
  X(is_enumerable_type, false, (info{}), ^^std::meta::is_enumerable_type) \
  X(is_variable, false, (info{}), ^^std::meta::is_variable) \
  X(is_type, false, (info{}), ^^std::meta::is_type) \
  X(is_namespace, false, (info{}), ^^std::meta::is_namespace) \
  X(is_type_alias, false, (info{}), ^^std::meta::is_type_alias) \
  X(is_namespace_alias, false, (info{}), ^^std::meta::is_namespace_alias) \
  X(is_function, false, (info{}), ^^std::meta::is_function) \
  X(is_conversion_function, false, (info{}), ^^std::meta::is_conversion_function) \
  X(is_operator_function, false, (info{}), ^^std::meta::is_operator_function) \
  X(is_literal_operator, false, (info{}), ^^std::meta::is_literal_operator) \
  X(is_special_member_function, false, (info{}), ^^std::meta::is_special_member_function) \
  X(is_constructor, false, (info{}), ^^std::meta::is_constructor) \
  X(is_default_constructor, false, (info{}), ^^std::meta::is_default_constructor) \
  X(is_copy_constructor, false, (info{}), ^^std::meta::is_copy_constructor) \
  X(is_move_constructor, false, (info{}), ^^std::meta::is_move_constructor) \
  X(is_assignment, false, (info{}), ^^std::meta::is_assignment) \
  X(is_copy_assignment, false, (info{}), ^^std::meta::is_copy_assignment) \
  X(is_move_assignment, false, (info{}), ^^std::meta::is_move_assignment) \
  X(is_destructor, false, (info{}), ^^std::meta::is_destructor) \
  X(is_function_parameter, false, (info{}), ^^std::meta::is_function_parameter) \
  X(is_explicit_object_parameter, false, (info{}), ^^std::meta::is_explicit_object_parameter) \
  X(has_default_argument, false, (info{}), ^^std::meta::has_default_argument) \
  X(is_vararg_function, false, (info{}), ^^std::meta::is_vararg_function) \
  X(is_template, false, (info{}), ^^std::meta::is_template) \
  X(is_function_template, false, (info{}), ^^std::meta::is_function_template) \
  X(is_variable_template, false, (info{}), ^^std::meta::is_variable_template) \
  X(is_class_template, false, (info{}), ^^std::meta::is_class_template) \
  X(is_alias_template, false, (info{}), ^^std::meta::is_alias_template) \
  X(is_conversion_function_template, false, (info{}), ^^std::meta::is_conversion_function_template) \
  X(is_operator_function_template, false, (info{}), ^^std::meta::is_operator_function_template) \
  X(is_literal_operator_template, false, (info{}), ^^std::meta::is_literal_operator_template) \
  X(is_constructor_template, false, (info{}), ^^std::meta::is_constructor_template) \
  X(is_concept, false, (info{}), ^^std::meta::is_concept) \
  X(is_value, false, (info{}), ^^std::meta::is_value) \
  X(is_object, false, (info{}), ^^std::meta::is_object) \
  X(is_structured_binding, false, (info{}), ^^std::meta::is_structured_binding) \
  X(is_class_member, false, (info{}), ^^std::meta::is_class_member) \
  X(is_namespace_member, false, (info{}), ^^std::meta::is_namespace_member) \
  X(is_nonstatic_data_member, false, (info{}), ^^std::meta::is_nonstatic_data_member) \
  X(is_static_member, false, (info{}), ^^std::meta::is_static_member) \
  X(is_base, false, (info{}), ^^std::meta::is_base) \
  X(has_default_member_initializer, false, (info{}), ^^std::meta::has_default_member_initializer) \
  X(has_parent, false, (info{}), ^^std::meta::has_parent) \
  X(parent_of, true, (info{}), ^^std::meta::parent_of) \
  X(dealias, false, (info{}), ^^std::meta::dealias) \
  X(has_template_arguments, false, (info{}), ^^std::meta::has_template_arguments) \
  X(template_of, true, (info{}), ^^std::meta::template_of) \
  X(template_arguments_of, true, (info{}), ^^std::meta::template_arguments_of) \
  X(parameters_of, true, (info{}), ^^std::meta::parameters_of) \
  X(variable_of, true, (info{}), ^^std::meta::variable_of) \
  X(return_type_of, true, (info{}), ^^std::meta::return_type_of) \
  X(is_accessible, false, (info{}, access_context::unchecked()), ^^std::meta::is_accessible) \
  X(has_inaccessible_nonstatic_data_members, true, (info{}, access_context::unchecked()), ^^std::meta::has_inaccessible_nonstatic_data_members) \
  X(has_inaccessible_bases, true, (info{}, access_context::unchecked()), ^^std::meta::has_inaccessible_bases) \
  X(has_inaccessible_subobjects, true, (info{}, access_context::unchecked()), ^^std::meta::has_inaccessible_subobjects) \
  X(current_function, false, (), ^^std::meta::current_function) \
  X(probe_current_class, true, (), ^^std::meta::current_class) \
  X(current_namespace, false, (), ^^std::meta::current_namespace) \
  X(members_of, true, (info{}, access_context::unchecked()), (choose<std::vector<info>, info, access_context>(std::meta::members_of))) \
  X(bases_of, true, (info{}, access_context::unchecked()), (choose<std::vector<info>, info, access_context>(std::meta::bases_of))) \
  X(static_data_members_of, true, (info{}, access_context::unchecked()), (choose<std::vector<info>, info, access_context>(std::meta::static_data_members_of))) \
  X(nonstatic_data_members_of, true, (info{}, access_context::unchecked()), (choose<std::vector<info>, info, access_context>(std::meta::nonstatic_data_members_of))) \
  X(subobjects_of, true, (info{}, access_context::unchecked()), (choose<std::vector<info>, info, access_context>(std::meta::subobjects_of))) \
  X(enumerators_of, true, (info{}), ^^std::meta::enumerators_of) \
  X(offset_of, true, (info{}), ^^std::meta::offset_of) \
  X(size_of, true, (info{}), ^^std::meta::size_of) \
  X(alignment_of, true, (info{}), ^^std::meta::alignment_of) \
  X(bit_size_of, true, (info{}), ^^std::meta::bit_size_of) \
  X(annotations_of, true, (info{}), (choose<std::vector<info>, info>(std::meta::annotations_of))) \
  X(annotations_of_with_type, true, (info{}, info{}), ^^std::meta::annotations_of_with_type) \
  X(extract<int>, true, (info{}), ^^std::meta::extract<int>) \
  X(can_substitute<infos>, true, (^^::, infos{^^::}), ^^std::meta::can_substitute<infos>) \
  X(substitute<infos>, true, (info{}, infos{^^::}), ^^std::meta::substitute<infos>) \
  X(reflect_constant<pointer_record>, true, (pointer_record{"invalid"}), ^^std::meta::reflect_constant<pointer_record>) \
  X(reflect_object<int>, true, (local), ^^std::meta::reflect_object<int>) \
  X(reflect_function<void()> , false, (ordinary_function), ^^std::meta::reflect_function<void()> ) \
  X(data_member_spec, true, (info{}, {}), ^^std::meta::data_member_spec) \
  X(is_data_member_spec, false, (info{}), ^^std::meta::is_data_member_spec) \
  X(is_void_type, true, (^^::), ^^std::meta::is_void_type) \
  X(is_null_pointer_type, true, (^^::), ^^std::meta::is_null_pointer_type) \
  X(is_integral_type, true, (^^::), ^^std::meta::is_integral_type) \
  X(is_floating_point_type, true, (^^::), ^^std::meta::is_floating_point_type) \
  X(is_array_type, true, (^^::), ^^std::meta::is_array_type) \
  X(is_pointer_type, true, (^^::), ^^std::meta::is_pointer_type) \
  X(is_lvalue_reference_type, true, (^^::), ^^std::meta::is_lvalue_reference_type) \
  X(is_rvalue_reference_type, true, (^^::), ^^std::meta::is_rvalue_reference_type) \
  X(is_member_object_pointer_type, true, (^^::), ^^std::meta::is_member_object_pointer_type) \
  X(is_member_function_pointer_type, true, (^^::), ^^std::meta::is_member_function_pointer_type) \
  X(is_enum_type, true, (^^::), ^^std::meta::is_enum_type) \
  X(is_union_type, true, (^^::), ^^std::meta::is_union_type) \
  X(is_class_type, true, (^^::), ^^std::meta::is_class_type) \
  X(is_function_type, true, (^^::), ^^std::meta::is_function_type) \
  X(is_reflection_type, true, (^^::), ^^std::meta::is_reflection_type) \
  X(is_reference_type, true, (^^::), ^^std::meta::is_reference_type) \
  X(is_arithmetic_type, true, (^^::), ^^std::meta::is_arithmetic_type) \
  X(is_fundamental_type, true, (^^::), ^^std::meta::is_fundamental_type) \
  X(is_object_type, true, (^^::), ^^std::meta::is_object_type) \
  X(is_scalar_type, true, (^^::), ^^std::meta::is_scalar_type) \
  X(is_compound_type, true, (^^::), ^^std::meta::is_compound_type) \
  X(is_member_pointer_type, true, (^^::), ^^std::meta::is_member_pointer_type) \
  X(is_const_type, true, (^^::), ^^std::meta::is_const_type) \
  X(is_volatile_type, true, (^^::), ^^std::meta::is_volatile_type) \
  X(is_trivially_copyable_type, true, (^^::), ^^std::meta::is_trivially_copyable_type) \
  X(is_standard_layout_type, true, (^^::), ^^std::meta::is_standard_layout_type) \
  X(is_empty_type, true, (^^::), ^^std::meta::is_empty_type) \
  X(is_polymorphic_type, true, (^^::), ^^std::meta::is_polymorphic_type) \
  X(is_abstract_type, true, (^^::), ^^std::meta::is_abstract_type) \
  X(is_final_type, true, (^^::), ^^std::meta::is_final_type) \
  X(is_aggregate_type, true, (^^::), ^^std::meta::is_aggregate_type) \
  X(is_structural_type, true, (^^::), ^^std::meta::is_structural_type) \
  X(is_signed_type, true, (^^::), ^^std::meta::is_signed_type) \
  X(is_unsigned_type, true, (^^::), ^^std::meta::is_unsigned_type) \
  X(is_bounded_array_type, true, (^^::), ^^std::meta::is_bounded_array_type) \
  X(is_unbounded_array_type, true, (^^::), ^^std::meta::is_unbounded_array_type) \
  X(is_scoped_enum_type, true, (^^::), ^^std::meta::is_scoped_enum_type) \
  X(is_constructible_type<infos>, true, (^^::, infos{^^::}), ^^std::meta::is_constructible_type<infos>) \
  X(is_default_constructible_type, true, (^^::), ^^std::meta::is_default_constructible_type) \
  X(is_copy_constructible_type, true, (^^::), ^^std::meta::is_copy_constructible_type) \
  X(is_move_constructible_type, true, (^^::), ^^std::meta::is_move_constructible_type) \
  X(is_assignable_type, true, (^^::, ^^::), ^^std::meta::is_assignable_type) \
  X(is_copy_assignable_type, true, (^^::), ^^std::meta::is_copy_assignable_type) \
  X(is_move_assignable_type, true, (^^::), ^^std::meta::is_move_assignable_type) \
  X(is_swappable_with_type, true, (^^::, ^^::), ^^std::meta::is_swappable_with_type) \
  X(is_swappable_type, true, (^^::), ^^std::meta::is_swappable_type) \
  X(is_destructible_type, true, (^^::), ^^std::meta::is_destructible_type) \
  X(is_trivially_constructible_type<infos>, true, (^^::, infos{^^::}), ^^std::meta::is_trivially_constructible_type<infos>) \
  X(is_trivially_default_constructible_type, true, (^^::), ^^std::meta::is_trivially_default_constructible_type) \
  X(is_trivially_copy_constructible_type, true, (^^::), ^^std::meta::is_trivially_copy_constructible_type) \
  X(is_trivially_move_constructible_type, true, (^^::), ^^std::meta::is_trivially_move_constructible_type) \
  X(is_trivially_assignable_type, true, (^^::, ^^::), ^^std::meta::is_trivially_assignable_type) \
  X(is_trivially_copy_assignable_type, true, (^^::), ^^std::meta::is_trivially_copy_assignable_type) \
  X(is_trivially_move_assignable_type, true, (^^::), ^^std::meta::is_trivially_move_assignable_type) \
  X(is_trivially_destructible_type, true, (^^::), ^^std::meta::is_trivially_destructible_type) \
  X(is_nothrow_constructible_type<infos>, true, (^^::, infos{^^::}), ^^std::meta::is_nothrow_constructible_type<infos>) \
  X(is_nothrow_default_constructible_type, true, (^^::), ^^std::meta::is_nothrow_default_constructible_type) \
  X(is_nothrow_copy_constructible_type, true, (^^::), ^^std::meta::is_nothrow_copy_constructible_type) \
  X(is_nothrow_move_constructible_type, true, (^^::), ^^std::meta::is_nothrow_move_constructible_type) \
  X(is_nothrow_assignable_type, true, (^^::, ^^::), ^^std::meta::is_nothrow_assignable_type) \
  X(is_nothrow_copy_assignable_type, true, (^^::), ^^std::meta::is_nothrow_copy_assignable_type) \
  X(is_nothrow_move_assignable_type, true, (^^::), ^^std::meta::is_nothrow_move_assignable_type) \
  X(is_nothrow_swappable_with_type, true, (^^::, ^^::), ^^std::meta::is_nothrow_swappable_with_type) \
  X(is_nothrow_swappable_type, true, (^^::), ^^std::meta::is_nothrow_swappable_type) \
  X(is_nothrow_destructible_type, true, (^^::), ^^std::meta::is_nothrow_destructible_type) \
  X(is_implicit_lifetime_type, true, (^^::), ^^std::meta::is_implicit_lifetime_type) \
  X(has_virtual_destructor, true, (^^::), ^^std::meta::has_virtual_destructor) \
  X(has_unique_object_representations, true, (^^::), ^^std::meta::has_unique_object_representations) \
  X(reference_constructs_from_temporary, true, (^^::, ^^::), ^^std::meta::reference_constructs_from_temporary) \
  X(reference_converts_from_temporary, true, (^^::, ^^::), ^^std::meta::reference_converts_from_temporary) \
  X(rank, true, (^^::), ^^std::meta::rank) \
  X(extent, true, (^^::, 0), ^^std::meta::extent) \
  X(is_same_type, true, (^^::, ^^::), ^^std::meta::is_same_type) \
  X(is_base_of_type, true, (^^::, ^^::), ^^std::meta::is_base_of_type) \
  X(is_virtual_base_of_type, true, (^^::, ^^::), ^^std::meta::is_virtual_base_of_type) \
  X(is_convertible_type, true, (^^::, ^^::), ^^std::meta::is_convertible_type) \
  X(is_nothrow_convertible_type, true, (^^::, ^^::), ^^std::meta::is_nothrow_convertible_type) \
  X(is_layout_compatible_type, true, (^^::, ^^::), ^^std::meta::is_layout_compatible_type) \
  X(is_pointer_interconvertible_base_of_type, true, (^^::, ^^::), ^^std::meta::is_pointer_interconvertible_base_of_type) \
  X(is_invocable_type<infos>, true, (^^::, infos{^^::}), ^^std::meta::is_invocable_type<infos>) \
  X(is_invocable_r_type<infos>, true, (^^::, ^^::, infos{^^::}), ^^std::meta::is_invocable_r_type<infos>) \
  X(is_nothrow_invocable_type<infos>, true, (^^::, infos{^^::}), ^^std::meta::is_nothrow_invocable_type<infos>) \
  X(is_nothrow_invocable_r_type<infos>, true, (^^::, ^^::, infos{^^::}), ^^std::meta::is_nothrow_invocable_r_type<infos>) \
  X(remove_const, true, (^^::), ^^std::meta::remove_const) \
  X(remove_volatile, true, (^^::), ^^std::meta::remove_volatile) \
  X(remove_cv, true, (^^::), ^^std::meta::remove_cv) \
  X(add_const, true, (^^::), ^^std::meta::add_const) \
  X(add_volatile, true, (^^::), ^^std::meta::add_volatile) \
  X(add_cv, true, (^^::), ^^std::meta::add_cv) \
  X(remove_reference, true, (^^::), ^^std::meta::remove_reference) \
  X(add_lvalue_reference, true, (^^::), ^^std::meta::add_lvalue_reference) \
  X(add_rvalue_reference, true, (^^::), ^^std::meta::add_rvalue_reference) \
  X(make_signed, true, (^^::), ^^std::meta::make_signed) \
  X(make_unsigned, true, (^^::), ^^std::meta::make_unsigned) \
  X(remove_extent, true, (^^::), ^^std::meta::remove_extent) \
  X(remove_all_extents, true, (^^::), ^^std::meta::remove_all_extents) \
  X(remove_pointer, true, (^^::), ^^std::meta::remove_pointer) \
  X(add_pointer, true, (^^::), ^^std::meta::add_pointer) \
  X(remove_cvref, true, (^^::), ^^std::meta::remove_cvref) \
  X(decay, true, (^^::), ^^std::meta::decay) \
  X(common_type<infos>, true, (infos{^^::}), ^^std::meta::common_type<infos>) \
  X(common_reference<infos>, true, (infos{^^::}), ^^std::meta::common_reference<infos>) \
  X(underlying_type, true, (^^::), ^^std::meta::underlying_type) \
  X(invoke_result<infos>, true, (^^::, infos{^^::}), ^^std::meta::invoke_result<infos>) \
  X(unwrap_reference, true, (^^::), ^^std::meta::unwrap_reference) \
  X(unwrap_ref_decay, true, (^^::), ^^std::meta::unwrap_ref_decay) \
  X(tuple_size, true, (^^::), ^^std::meta::tuple_size) \
  X(tuple_element, true, (0, ^^::), ^^std::meta::tuple_element) \
  X(is_applicable_type, true, (^^::, ^^::), ^^std::meta::is_applicable_type) \
  X(is_nothrow_applicable_type, true, (^^::, ^^::), ^^std::meta::is_nothrow_applicable_type) \
  X(apply_result, true, (^^::, ^^::), ^^std::meta::apply_result) \
  X(variant_size, true, (^^::), ^^std::meta::variant_size) \
  X(variant_alternative, true, (0, ^^::), ^^std::meta::variant_alternative) \
  X(type_order, true, (^^::, ^^::), ^^std::meta::type_order)

#define CHECK(F, MUST_THROW, ARGS, REFLECTION) \
  static_assert(check_from<REFLECTION, MUST_THROW>([] consteval { \
    [[maybe_unused]] int local = 0; \
    (void)F ARGS; \
  }), #F);
METAFUNCTION_TABLE(CHECK)
#undef CHECK
#undef METAFUNCTION_TABLE

// Both extract specializations must identify the invoked public specialization.
static_assert(check_from<^^std::meta::extract<int&>, true>([] consteval {
  (void)std::meta::extract<int&>(info{});
}));
static_assert(check_from<^^std::meta::reflect_constant<const char*>, true>([] consteval {
  (void)std::meta::reflect_constant<const char*>("invalid");
}));

static_assert(check_from<^^std::meta::reflect_constant_string<const char (&)[3]>, false>([] consteval {
  (void)std::meta::reflect_constant_string("ok");
}));
static_assert(check_from<^^std::meta::reflect_constant_array<std::array<pointer_record, 1>>, true>([] consteval {
  (void)std::meta::reflect_constant_array(std::array{pointer_record{"invalid"}});
}));

// is_accessible throws only for a class member of an incomplete class.
// Its null-reflection probe in the table therefore cannot exercise Throws.
struct incomplete_access {
  int member;
  static constexpr bool from_is_public = check_from<^^std::meta::is_accessible, true>([] consteval {
    (void)std::meta::is_accessible(^^member, access_context::unchecked());
  });
};
static_assert(incomplete_access::from_is_public);

static_assert(check_from<^^std::define_static_object<pointer_record>, true>([] consteval {
  (void)std::define_static_object(pointer_record{"invalid"});
}));
static_assert(check_from<^^std::define_static_array<std::array<pointer_record, 1>>, true>([] consteval {
  (void)std::define_static_array(std::array{pointer_record{"invalid"}});
}));
static_assert(check_from<^^std::define_static_string<const char (&)[3]>, false>([] consteval {
  (void)std::define_static_string("ok");
}));

// All five is_string_literal overloads are nonthrowing for null pointers.
static_assert(!std::is_string_literal(static_cast<const char*>(nullptr)));
static_assert(!std::is_string_literal(static_cast<const wchar_t*>(nullptr)));
static_assert(!std::is_string_literal(static_cast<const char8_t*>(nullptr)));
static_assert(!std::is_string_literal(static_cast<const char16_t*>(nullptr)));
static_assert(!std::is_string_literal(static_cast<const char32_t*>(nullptr)));

static_assert(check_from<^^std::meta::access_context::via, true>([] consteval {
  (void)access_context::unchecked().via(^^int);
}));
static_assert(access_context::unchecked().scope() == info{});
static_assert(access_context::unchecked().designating_class() == info{});
static_assert(access_context::unprivileged().designating_class() == info{});
static_assert(access_context::current().designating_class() == info{});

// [meta.reflection.scope]: current_function throws "unless S represents a
// function". A namespace consteval block preserves the namespace scope; a
// lambda would introduce a function scope and defeat the negative control.
consteval {
  bool caught_from_public = false;
  try {
    (void)std::meta::current_function();
  } catch (std::meta::exception& e) {
    caught_from_public = e.from() == ^^std::meta::current_function;
  }
  if (!caught_from_public)
    throw "current_function must report its public origin at namespace scope";
}

// Injected declarations require a plainly constant-evaluated context, so this
// nonthrowing synopsis entry has its control outside the generic table helper.
consteval {
  (void)std::meta::define_aggregate<infos>(^^generated_aggregate, infos{});
}
static_assert(std::meta::is_complete_type(^^generated_aggregate));

int main(int, char**) {}
