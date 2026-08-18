// The implementation header is textually included in the global module
// fragment.  Re-export its public declarations explicitly; a textual include
// alone does not make declarations visible to importers of std.
export namespace std {
  using std::define_static_array;
  using std::define_static_string;

  namespace meta {
    inline namespace reflection_v2 {
      using std::meta::reflection_v2::info;
      using std::meta::reflection_v2::reflection_range;
      using std::meta::reflection_v2::AttributeNamespace;
      using std::meta::reflection_v2::identifier_of;
      using std::meta::reflection_v2::display_string_of;
      using std::meta::reflection_v2::u8identifier_of;
      using std::meta::reflection_v2::u8display_string_of;
      using std::meta::reflection_v2::has_identifier;
      using std::meta::reflection_v2::operators;
      using std::meta::reflection_v2::operator_of;
      using std::meta::reflection_v2::source_location_of;
      using std::meta::reflection_v2::type_of;
      using std::meta::reflection_v2::parent_of;
      using std::meta::reflection_v2::dealias;
      using std::meta::reflection_v2::object_of;
      using std::meta::reflection_v2::constant_of;
      using std::meta::reflection_v2::template_of;
      using std::meta::reflection_v2::template_arguments_of;
      using std::meta::reflection_v2::members_of;
      using std::meta::reflection_v2::bases_of;
      using std::meta::reflection_v2::static_data_members_of;
      using std::meta::reflection_v2::nonstatic_data_members_of;
      using std::meta::reflection_v2::enumerators_of;
      using std::meta::reflection_v2::can_substitute;
      using std::meta::reflection_v2::substitute;
      using std::meta::reflection_v2::reflect_constant;
      using std::meta::reflection_v2::reflect_object;
      using std::meta::reflection_v2::reflect_function;
      using std::meta::reflection_v2::extract;

#define _LIBCPP_META_EXPORT_PREDICATE(name) using std::meta::reflection_v2::name;
      _LIBCPP_META_EXPORT_PREDICATE(is_public)
      _LIBCPP_META_EXPORT_PREDICATE(is_protected)
      _LIBCPP_META_EXPORT_PREDICATE(is_private)
      _LIBCPP_META_EXPORT_PREDICATE(is_virtual)
      _LIBCPP_META_EXPORT_PREDICATE(is_pure_virtual)
      _LIBCPP_META_EXPORT_PREDICATE(is_override)
      _LIBCPP_META_EXPORT_PREDICATE(is_deleted)
      _LIBCPP_META_EXPORT_PREDICATE(is_defaulted)
      _LIBCPP_META_EXPORT_PREDICATE(is_explicit)
      _LIBCPP_META_EXPORT_PREDICATE(is_noexcept)
      _LIBCPP_META_EXPORT_PREDICATE(is_bit_field)
      _LIBCPP_META_EXPORT_PREDICATE(is_enumerator)
      _LIBCPP_META_EXPORT_PREDICATE(is_const)
      _LIBCPP_META_EXPORT_PREDICATE(is_volatile)
      _LIBCPP_META_EXPORT_PREDICATE(is_mutable_member)
      _LIBCPP_META_EXPORT_PREDICATE(is_lvalue_reference_qualified)
      _LIBCPP_META_EXPORT_PREDICATE(is_rvalue_reference_qualified)
      _LIBCPP_META_EXPORT_PREDICATE(has_static_storage_duration)
      _LIBCPP_META_EXPORT_PREDICATE(has_thread_storage_duration)
      _LIBCPP_META_EXPORT_PREDICATE(has_automatic_storage_duration)
      _LIBCPP_META_EXPORT_PREDICATE(has_internal_linkage)
      _LIBCPP_META_EXPORT_PREDICATE(has_module_linkage)
      _LIBCPP_META_EXPORT_PREDICATE(has_external_linkage)
      _LIBCPP_META_EXPORT_PREDICATE(has_linkage)
      _LIBCPP_META_EXPORT_PREDICATE(is_class_member)
      _LIBCPP_META_EXPORT_PREDICATE(is_namespace_member)
      _LIBCPP_META_EXPORT_PREDICATE(is_nonstatic_data_member)
      _LIBCPP_META_EXPORT_PREDICATE(is_static_member)
      _LIBCPP_META_EXPORT_PREDICATE(is_base)
      _LIBCPP_META_EXPORT_PREDICATE(is_data_member_spec)
      _LIBCPP_META_EXPORT_PREDICATE(is_namespace)
      _LIBCPP_META_EXPORT_PREDICATE(is_function)
      _LIBCPP_META_EXPORT_PREDICATE(is_variable)
      _LIBCPP_META_EXPORT_PREDICATE(is_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_type_alias)
      _LIBCPP_META_EXPORT_PREDICATE(is_namespace_alias)
      _LIBCPP_META_EXPORT_PREDICATE(is_complete_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_enumerable_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_template)
      _LIBCPP_META_EXPORT_PREDICATE(is_function_template)
      _LIBCPP_META_EXPORT_PREDICATE(is_variable_template)
      _LIBCPP_META_EXPORT_PREDICATE(is_class_template)
      _LIBCPP_META_EXPORT_PREDICATE(is_alias_template)
      _LIBCPP_META_EXPORT_PREDICATE(is_conversion_function_template)
      _LIBCPP_META_EXPORT_PREDICATE(is_operator_function_template)
      _LIBCPP_META_EXPORT_PREDICATE(is_literal_operator_template)
      _LIBCPP_META_EXPORT_PREDICATE(is_constructor_template)
      _LIBCPP_META_EXPORT_PREDICATE(is_concept)
      _LIBCPP_META_EXPORT_PREDICATE(is_structured_binding)
      _LIBCPP_META_EXPORT_PREDICATE(has_template_arguments)
      _LIBCPP_META_EXPORT_PREDICATE(has_default_member_initializer)
      _LIBCPP_META_EXPORT_PREDICATE(is_conversion_function)
      _LIBCPP_META_EXPORT_PREDICATE(is_operator_function)
      _LIBCPP_META_EXPORT_PREDICATE(is_literal_operator)
      _LIBCPP_META_EXPORT_PREDICATE(is_constructor)
      _LIBCPP_META_EXPORT_PREDICATE(is_default_constructor)
      _LIBCPP_META_EXPORT_PREDICATE(is_copy_constructor)
      _LIBCPP_META_EXPORT_PREDICATE(is_move_constructor)
      _LIBCPP_META_EXPORT_PREDICATE(is_assignment)
      _LIBCPP_META_EXPORT_PREDICATE(is_copy_assignment)
      _LIBCPP_META_EXPORT_PREDICATE(is_move_assignment)
      _LIBCPP_META_EXPORT_PREDICATE(is_destructor)
      _LIBCPP_META_EXPORT_PREDICATE(is_special_member_function)
      _LIBCPP_META_EXPORT_PREDICATE(is_user_provided)
      _LIBCPP_META_EXPORT_PREDICATE(is_user_declared)
      _LIBCPP_META_EXPORT_PREDICATE(data_member_spec)
      using std::meta::reflection_v2::data_member_options;
      using std::meta::reflection_v2::define_aggregate;
      using std::meta::reflection_v2::member_offset;
      using std::meta::reflection_v2::offset_of;
      using std::meta::reflection_v2::size_of;
      using std::meta::reflection_v2::alignment_of;
      using std::meta::reflection_v2::bit_size_of;

      _LIBCPP_META_EXPORT_PREDICATE(is_void_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_null_pointer_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_integral_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_floating_point_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_array_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_pointer_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_lvalue_reference_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_rvalue_reference_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_member_object_pointer_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_member_function_pointer_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_enum_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_union_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_class_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_function_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_reflection_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_reference_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_arithmetic_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_fundamental_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_object_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_scalar_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_compound_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_member_pointer_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_const_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_volatile_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_trivial_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_trivially_copyable_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_standard_layout_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_empty_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_polymorphic_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_abstract_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_final_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_aggregate_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_consteval_only_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_signed_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_unsigned_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_bounded_array_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_unbounded_array_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_scoped_enum_type)
      using std::meta::reflection_v2::is_constructible_type;
      _LIBCPP_META_EXPORT_PREDICATE(is_default_constructible_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_copy_constructible_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_move_constructible_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_assignable_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_copy_assignable_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_move_assignable_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_swappable_with_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_swappable_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_destructible_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_trivially_default_constructible_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_trivially_copy_constructible_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_trivially_move_constructible_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_trivially_assignable_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_trivially_copy_assignable_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_trivially_move_assignable_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_trivially_destructible_type)
      using std::meta::reflection_v2::is_nothrow_constructible_type;
      _LIBCPP_META_EXPORT_PREDICATE(is_nothrow_default_constructible_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_nothrow_copy_constructible_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_nothrow_move_constructible_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_nothrow_assignable_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_nothrow_copy_assignable_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_nothrow_move_assignable_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_nothrow_swappable_with_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_nothrow_swappable_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_nothrow_destructible_type)
      _LIBCPP_META_EXPORT_PREDICATE(has_virtual_destructor)
      _LIBCPP_META_EXPORT_PREDICATE(has_unique_object_representations)
      using std::meta::reflection_v2::rank;
      using std::meta::reflection_v2::extent;
      _LIBCPP_META_EXPORT_PREDICATE(is_same_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_base_of_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_convertible_type)
      _LIBCPP_META_EXPORT_PREDICATE(is_nothrow_convertible_type)
      using std::meta::reflection_v2::is_invocable_type;
      using std::meta::reflection_v2::is_invocable_r_type;
      using std::meta::reflection_v2::is_nothrow_invocable_type;
      using std::meta::reflection_v2::is_nothrow_invocable_r_type;
      using std::meta::reflection_v2::remove_const;
      using std::meta::reflection_v2::remove_volatile;
      using std::meta::reflection_v2::remove_cv;
      using std::meta::reflection_v2::add_const;
      using std::meta::reflection_v2::add_volatile;
      using std::meta::reflection_v2::add_cv;
      using std::meta::reflection_v2::remove_reference;
      using std::meta::reflection_v2::add_lvalue_reference;
      using std::meta::reflection_v2::add_rvalue_reference;
      using std::meta::reflection_v2::make_signed;
      using std::meta::reflection_v2::make_unsigned;
      using std::meta::reflection_v2::remove_extent;
      using std::meta::reflection_v2::remove_all_extents;
      using std::meta::reflection_v2::remove_pointer;
      using std::meta::reflection_v2::add_pointer;
      using std::meta::reflection_v2::remove_cvref;
      using std::meta::reflection_v2::decay;
      using std::meta::reflection_v2::common_type;
      using std::meta::reflection_v2::common_reference;
      using std::meta::reflection_v2::underlying_type;
      using std::meta::reflection_v2::invoke_result;
      using std::meta::reflection_v2::unwrap_reference;
      using std::meta::reflection_v2::unwrap_ref_decay;
      using std::meta::reflection_v2::tuple_size;
      using std::meta::reflection_v2::tuple_element;
      using std::meta::reflection_v2::variant_size;
      using std::meta::reflection_v2::variant_alternative;
      using std::meta::reflection_v2::parameters_of;
      using std::meta::reflection_v2::has_ellipsis_parameter;
      using std::meta::reflection_v2::has_default_argument;
      using std::meta::reflection_v2::is_explicit_object_parameter;
      using std::meta::reflection_v2::is_function_parameter;
      using std::meta::reflection_v2::return_type_of;
      using std::meta::reflection_v2::variable_of;
      using std::meta::reflection_v2::annotations_of;
      using std::meta::reflection_v2::annotation_of_type;
      using std::meta::reflection_v2::is_annotation;
      using std::meta::reflection_v2::annotate;
      using std::meta::reflection_v2::is_attribute;
      using std::meta::reflection_v2::attributes_of;
      using std::meta::reflection_v2::attribute_namespace_of;
      using std::meta::reflection_v2::attribute_token_of;
      using std::meta::reflection_v2::has_attribute_namespace;
      using std::meta::reflection_v2::access_context;
      using std::meta::reflection_v2::is_accessible;
      using std::meta::reflection_v2::has_inaccessible_nonstatic_data_members;
      using std::meta::reflection_v2::has_inaccessible_bases;
      using std::meta::reflection_v2::is_access_specified;
      using std::meta::reflection_v2::reflect_invoke;
#undef _LIBCPP_META_EXPORT_PREDICATE
    }
  }
}
