//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___META_METAFUNCTION_IDS_H
#define _LIBCPP___META_METAFUNCTION_IDS_H

#include <__config>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_STD_VER >= 26

_LIBCPP_BEGIN_NAMESPACE_REFLECTION_V2

// An opaque handle to a reflected entity.
using info = decltype(^^int);

// The numbers by which the compiler identifies a metafunction (the first
// argument of __metafunction). The one list, shared by <meta> and the headers
// that need a few metafunctions without all of <meta>.
namespace __detail {
enum : unsigned {

  // non-exposed metafunctions
  __metafn_get_begin_enumerator_decl_of,
  __metafn_get_get_next_enumerator_decl_of,
  __metafn_get_ith_base_of,
  __metafn_get_ith_template_argument_of,
  __metafn_get_begin_member_decl_of,
  __metafn_get_next_member_decl_of,
  __metafn_is_structural_type,
  __metafn_map_decl_to_entity,

  // CXX26 metafunctions
  __metafn_identifier_of,
  __metafn_has_identifier,
  __metafn_operator_of,
  __metafn_source_location_of,
  __metafn_type_of,
  __metafn_parent_of,
  __metafn_has_parent,
  __metafn_underlying_entity_of,
  __metafn_object_of,
  __metafn_constant_of,
  __metafn_template_of,
  __metafn_substitute,
  __metafn_extract,
  __metafn_is_public,
  __metafn_is_protected,
  __metafn_is_private,
  __metafn_is_virtual,
  __metafn_is_pure_virtual,
  __metafn_is_override,
  __metafn_is_deleted,
  __metafn_is_defaulted,
  __metafn_is_explicit,
  __metafn_is_noexcept,
  __metafn_is_bit_field,
  __metafn_is_enumerator,
  __metafn_is_final,
  __metafn_is_const,
  __metafn_is_volatile,
  __metafn_is_mutable_member,
  __metafn_is_lvalue_reference_qualified,
  __metafn_is_rvalue_reference_qualified,
  __metafn_has_static_storage_duration,
  __metafn_has_thread_storage_duration,
  __metafn_has_automatic_storage_duration,
  __metafn_has_internal_linkage,
  __metafn_has_module_linkage,
  __metafn_has_external_linkage,
  __metafn_has_linkage,
  __metafn_is_class_member,
  __metafn_is_namespace_member,
  __metafn_is_nonstatic_data_member,
  __metafn_is_static_member,
  __metafn_is_base,
  __metafn_is_data_member_spec,
  __metafn_is_namespace,
  __metafn_is_function,
  __metafn_is_variable,
  __metafn_is_type,
  __metafn_is_alias,
  __metafn_is_complete_type,
  __metafn_has_complete_definition,
  __metafn_is_enumerable_type,
  __metafn_is_template,
  __metafn_is_function_template,
  __metafn_is_variable_template,
  __metafn_is_class_template,
  __metafn_is_alias_template,
  __metafn_is_conversion_function_template,
  __metafn_is_operator_function_template,
  __metafn_is_literal_operator_template,
  __metafn_is_constructor_template,
  __metafn_is_concept,
  __metafn_is_structured_binding,
  __metafn_is_value,
  __metafn_is_object,
  __metafn_has_template_arguments,
  __metafn_has_default_member_initializer,
  __metafn_is_conversion_function,
  __metafn_is_operator_function,
  __metafn_is_literal_operator,
  __metafn_is_constructor,
  __metafn_is_default_constructor,
  __metafn_is_copy_constructor,
  __metafn_is_move_constructor,
  __metafn_is_assignment,
  __metafn_is_copy_assignment,
  __metafn_is_move_assignment,
  __metafn_is_destructor,
  __metafn_is_special_member_function,
  __metafn_is_user_provided,
  __metafn_is_user_declared,
  __metafn_reflect_result,
  __metafn_data_member_spec,
  __metafn_define_aggregate,
  __metafn_offset_of,
  __metafn_size_of,
  __metafn_bit_offset_of,
  __metafn_bit_size_of,
  __metafn_alignment_of,
  __metafn_has_c_language_linkage,

  // P3096 parameter reflection metafunctions
  __metafn_get_ith_parameter_of,
  __metafn_is_vararg_function,
  __metafn_has_default_argument,
  __metafn_is_explicit_object_parameter,
  __metafn_is_function_parameter,
  __metafn_return_type_of,
  __metafn_variable_of,

  // P3394 annotation metafunctions
  __metafn_get_ith_annotation_of,
  __metafn_is_annotation,

  // P3493 accessibility metafunctions
  __metafn_access_context,
  __metafn_current_function,
  __metafn_current_class,
  __metafn_current_namespace,
  __metafn_is_accessible,

  // Other bespoke functions (not proposed at this time)
  __metafn_is_access_specified,
  __metafn_reflect_invoke,

  // P4033 extension: completing unscoped (C-style) enums

  __metafn_is_closure_type,

  // P3491R3 string-literal helper
  __metafn_is_string_literal,
  __metafn_reflect_array_object,

  // Expression macros (experimental)
  __metafn_source_text_of,
  __metafn_is_binary_operation,
  __metafn_get_ith_operand_of,
  __metafn_token_count,
  __metafn_get_ith_token,
  __metafn_token_kind_of,
  __metafn_operator_of_token,
  __metafn_identifier_of_token,
  __metafn_is_constant_expression,
  __metafn_test_expression,
  __metafn_as_lvalue,
  __metafn_macro_expansion_context,
  __metafn_declaration_of,
  __metafn_forwarding_call_for,
  __metafn_is_declaration_spec,
  __metafn_make_override,
  __metafn_make_noexcept,
  __metafn_template_parameter_list_for,
  __metafn_template_argument_list_for,
  __metafn_argument_list_for,
  __metafn_expand_at_instantiation,
  __metafn_punctuator_of,
  __metafn_tokens_join,
  __metafn_tokens_fill,
  __metafn_tokens_materialize,
  __metafn_members_gather,
  __metafn_tokens_append,
  __metafn_tokens_subseq,
  __metafn_token_type,
  __metafn_tokens_equal,
};
} // namespace __detail

_LIBCPP_END_NAMESPACE_REFLECTION_V2

#endif // _LIBCPP_STD_VER >= 26

#endif // _LIBCPP___META_METAFUNCTION_IDS_H
