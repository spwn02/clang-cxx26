# M5 diagnostic and ill-formed-program checklist

Status: scoping baseline, 2026-09-09. This file is an execution checklist; it does not add
tests. `Covered` means an existing test exercises the condition and verifies the diagnostic (or
the specified rejection). `Needs-New-Test` means the facility exists and a focused negative test
is still required. `Blocked-On-Unimplemented-Facility` means the wording requires a facility or
failure model that this fork does not yet implement; the eventual test must be added after that
facility exists, or must explicitly test the current hard diagnostic when the paper's
`meta::exception` path is not yet available.

The inventory was made by reading the complete M2 table in `docs/REFLECTION_GAPS.md`, then
checking the adopted wording of each paper. Conditions are split when they require materially
different reproducers. A `Constant When` failure is an ill-formed call in a manifestly constant
context; it is not interchangeable with a library `Throws` test.

## Totals

| Total conditions | Covered | Needs-New-Test | Blocked-On-Unimplemented-Facility |
|---:|---:|---:|---:|
| 109 | 23 | 68 | 18 |

The count is by row below, not by diagnostic line. Several rows deliberately cover a conjunction
from one standard-library clause; future implementation sessions may split such a row if the
diagnostic needs independent coverage for each operand.

## P2996R13 — Reflection for C++26

Normative source: [P2996R13](https://wg21.link/P2996R13), especially [meta.reflection.names],
[meta.reflection.queries], [meta.reflection.access.queries], [meta.reflection.member.queries],
[meta.reflection.layout], [meta.reflection.extract], [meta.reflection.substitute],
[meta.reflection.result], and [meta.reflection.define.aggregate].

| ID | Condition | Status | Existing coverage |
|---|---|---|---|
| 2996-01 | `reflect_constant<T>`: `T` is copy-constructible. | Covered | [m5-p2996-batch1.verify.cpp](../../libcxx/test/std/experimental/reflection/m5-p2996-batch1.verify.cpp) |
| 2996-02 | `reflect_constant<T>`: `T` is cv-unqualified structural and not a reference type. | Covered | [m5-p2996-batch1.verify.cpp](../../libcxx/test/std/experimental/reflection/m5-p2996-batch1.verify.cpp) |
| 2996-03 | `reflect_constant(expr)`: the invented template argument object can be formed; otherwise the call is not a constant subexpression. | Covered | [m5-p2996-batch1.verify.cpp](../../libcxx/test/std/experimental/reflection/m5-p2996-batch1.verify.cpp) |
| 2996-04 | `reflect_object<T>`: `T` is an object type. | Covered | [m5-p2996-batch1.verify.cpp](../../libcxx/test/std/experimental/reflection/m5-p2996-batch1.verify.cpp) |
| 2996-05 | `reflect_object(expr)`: `expr` is suitable as a constant template argument for `T&`. | Covered | [m5-p2996-batch2.verify.cpp](../../libcxx/test/std/experimental/reflection/m5-p2996-batch2.verify.cpp) |
| 2996-06 | `reflect_function<T>`: `T` is a function type. | Covered | [m5-p2996-batch2.verify.cpp](../../libcxx/test/std/experimental/reflection/m5-p2996-batch2.verify.cpp) |
| 2996-07 | `reflect_function(fn)`: `fn` is suitable as a constant template argument for `T&`. | Covered | [m5-p2996-batch2.verify.cpp](../../libcxx/test/std/experimental/reflection/m5-p2996-batch2.verify.cpp) |
| 2996-08 | `identifier_of(r)` / `u8identifier_of(r)`: `r` represents a declaration with an identifier (including the specified operator/literal-operator cases). | Covered | [m5-p2996-batch2.verify.cpp](../../libcxx/test/std/experimental/reflection/m5-p2996-batch2.verify.cpp) |
| 2996-09 | `display_string_of(r)`: `r` represents a construct for which a display string can be produced. | Needs-New-Test | — |
| 2996-10 | `source_location_of(r)`: `r` represents a declaration with a source location. | Needs-New-Test | — |
| 2996-11 | `type_of(r)`: `r` represents a construct having a type, and the type is available under the clause's completeness/containing-enum rules. | Covered | [m5-p2996-batch3.verify.cpp](../../libcxx/test/std/experimental/reflection/m5-p2996-batch3.verify.cpp) |
| 2996-12 | `parent_of(r)`: `r` represents a construct with a parent. | Covered | [m5-p2996-batch3.verify.cpp](../../libcxx/test/std/experimental/reflection/m5-p2996-batch3.verify.cpp) |
| 2996-13 | `object_of(r)`: `r` represents a variable or object whose object can be designated. | Covered | [m5-p2996-batch3.verify.cpp](../../libcxx/test/std/experimental/reflection/m5-p2996-batch3.verify.cpp) |
| 2996-14 | `constant_of(r)`: `r` represents a value or object usable as the required constant expression. | Covered | [m5-p2996-batch3.verify.cpp](../../libcxx/test/std/experimental/reflection/m5-p2996-batch3.verify.cpp) |
| 2996-15 | `template_of(r)`: `has_template_arguments(r)` is true. | Covered | [m5-p2996-batch3.verify.cpp](../../libcxx/test/std/experimental/reflection/m5-p2996-batch3.verify.cpp) |
| 2996-16 | `template_arguments_of(r)`: `has_template_arguments(r)` is true. | Needs-New-Test | — |
| 2996-17 | `is_accessible(r, ctx)`: `ctx` is a valid access context and the represented construct is one to which access can be applied. | Needs-New-Test | — |
| 2996-18 | `has_inaccessible_nonstatic_data_members(r, ctx)`: `nonstatic_data_members_of(r, unchecked())` is a constant subexpression. | Needs-New-Test | — |
| 2996-19 | `has_inaccessible_nonstatic_data_members(r, ctx)`: `r` does not represent a closure type. | Needs-New-Test | — |
| 2996-20 | `has_inaccessible_bases(r, ctx)`: `bases_of(r, unchecked())` is a constant subexpression. | Needs-New-Test | — |
| 2996-21 | `members_of(r, ctx)`: `dealias(r)` is a complete class type at a point in the evaluation context or a namespace. | Needs-New-Test | — |
| 2996-22 | `bases_of(type, ctx)`: `dealias(type)` is a complete class type at a point in the evaluation context. | Needs-New-Test | — |
| 2996-23 | `static_data_members_of(type, ctx)`: `dealias(type)` is a complete class type. | Needs-New-Test | — |
| 2996-24 | `nonstatic_data_members_of(type, ctx)`: `dealias(type)` is a complete class type. | Needs-New-Test | — |
| 2996-25 | `enumerators_of(type_enum)`: `dealias(type_enum)` is an enumeration and `is_enumerable_type(type_enum)` is true. | Needs-New-Test | — |
| 2996-26 | `offset_of(r)`: `r` is a non-static data member, unnamed bit-field, or permitted direct-base relationship (non-virtual base, or non-abstract derived class). | Needs-New-Test | — |
| 2996-27 | `size_of(r)`: `r` represents one of the permitted type/object/value/variable/member/base/data-member-spec kinds. | Needs-New-Test | — |
| 2996-28 | `size_of(r)`: a reflected type is complete. | Needs-New-Test | — |
| 2996-29 | `alignment_of(r)`: `r` represents one of the permitted type/object/variable/member/base/data-member-spec kinds. | Needs-New-Test | — |
| 2996-30 | `alignment_of(r)`: a reflected type is complete. | Needs-New-Test | — |
| 2996-31 | `bit_size_of(r)`: `r` is a permitted type/object/value/variable/member/bit-field/base/data-member-spec reflection. | Needs-New-Test | — |
| 2996-32 | `bit_size_of(r)`: a reflected type is not incomplete at a point in the evaluation context. | Needs-New-Test | — |
| 2996-33 | `extract<T>(r)` value extraction: `r` represents a variable or object of type `U`, the qualification conversion to `T` is permitted, and a variable is usable in constant expressions or began its lifetime in the current core constant expression. | Needs-New-Test | — |
| 2996-34 | `extract<T>(r)` member/function extraction: `r` is a non-bit-field direct member or implicit-object member function and `T` is the corresponding permitted member-pointer type. | Needs-New-Test | — |
| 2996-35 | `extract<T>(r)` value extraction: pointer types are similar/compatible as required, or non-pointer cv-unqualified types match. | Needs-New-Test | — |
| 2996-36 | `can_substitute(templ, args)`: `templ` represents a template and every argument reflection represents a usable template argument. | Needs-New-Test | — |
| 2996-37 | `substitute(templ, args)`: `can_substitute(templ, args)` is true. | Needs-New-Test | — |
| 2996-38 | `data_member_spec(type, options)`: `dealias(type)` is an object or reference type. | Needs-New-Test | — |
| 2996-39 | `data_member_spec`: a supplied name is a valid identifier in the required encoding and does not conflict with the generated member rules. | Needs-New-Test | — |
| 2996-40 | `data_member_spec`: supplied `type`, `width`, `alignment`, and `no_unique_address` options satisfy their individual type/value and combination constraints. | Needs-New-Test | — |
| 2996-41 | `define_aggregate(type, members)`: the reflected type is an incomplete class definition in the permitted context. | Covered | [define-aggregate.verify.cpp](../../libcxx/test/std/experimental/reflection/define-aggregate.verify.cpp) |
| 2996-42 | `define_aggregate`: the evaluation is plainly/manifestly constant-evaluated as required. | Covered | [define-aggregate.verify.cpp](../../libcxx/test/std/experimental/reflection/define-aggregate.verify.cpp) |
| 2996-43 | `define_aggregate`: the evaluation function/class encloses both the declaration and definition. | Covered | [define-aggregate.verify.cpp](../../libcxx/test/std/experimental/reflection/define-aggregate.verify.cpp) |
| 2996-44 | `define_aggregate`: the target is not already complete. | Covered | [define-aggregate.verify.cpp](../../libcxx/test/std/experimental/reflection/define-aggregate.verify.cpp) |
| 2996-45 | `define_aggregate`: injected declarations obey the scope, reachability, sequencing, and complete-class restrictions. | Covered | [define-aggregate.verify.cpp](../../libcxx/test/std/experimental/reflection/define-aggregate.verify.cpp) |
| 2996-46 | Reflection of a local parameter introduced by a requires-expression is ill-formed. | Needs-New-Test | — |
| 2996-47 | A reflection operator naming a `using-declarator` is ill-formed under the adopted R13 wording. | Needs-New-Test | — |
| 2996-48 | A splice of a constructor or destructor is ill-formed. | Needs-New-Test | — |
| 2996-49 | A dependent splice-specifier in the forbidden CTAD position is ill-formed. | Needs-New-Test | — |

## P1306R5 — Expansion Statements

Normative source: [P1306R5](https://wg21.link/P1306R5).

| ID | Condition | Status | Existing coverage |
|---|---|---|---|
| 1306-01 | The expansion initializer/range must be a constant expression when the expansion is instantiated. | Needs-New-Test | — |
| 1306-02 | The range-based form requires a valid `begin`/`end` range and dereferenceable iteration for the generated expansion. | Needs-New-Test | — |
| 1306-03 | The for-range declaration may contain only the permitted declaration specifiers. | Needs-New-Test | — |
| 1306-04 | Labels and control flow that escape the expansion statement are ill-formed. | Needs-New-Test | — |
| 1306-05 | Destructuring expansion requires an initializer usable for the generated structured binding. | Needs-New-Test | — |

## P3096R12 — Function Parameter Reflection

Normative source: [P3096R12](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3096r12.pdf).

| ID | Condition | Status | Existing coverage |
|---|---|---|---|
| 3096-01 | `parameters_of(r)`: `r` represents a function or function template for which parameters can be reflected. | Needs-New-Test | — |
| 3096-02 | `return_type_of(r)`: `r` represents a function or function template with a return type that can be reflected. | Needs-New-Test | — |
| 3096-03 | `variable_of(r)`: `r` represents a function parameter and the parameter has a valid invocation frame in the required constant-evaluation context. | Needs-New-Test | — |
| 3096-04 | `has_ellipsis_parameter(r)` is total: for a non-function reflection it returns false rather than diagnosing. | Needs-New-Test | — |
| 3096-05 | `has_default_argument(r)` is total: for a non-function-parameter reflection it returns false rather than diagnosing. | Needs-New-Test | — |
| 3096-06 | Parameter `identifier_of`, `u8identifier_of`, `type_of`, and `has_identifier` are ill-formed when applied outside the parameter cases specified by the paper. | Needs-New-Test | — |
| 3096-07 | Local parameters introduced by a requires-expression cannot be reflected. | Needs-New-Test | — |

## P3293R3 — Splicing a Base Class Subobject

Normative source: [P3293R3](https://wg21.link/P3293R3).

| ID | Condition | Status | Existing coverage |
|---|---|---|---|
| 3293-01 | A base-subobject splice must name a direct base class relationship of the object expression. | Covered | [base-splice.verify.cpp](../../libcxx/test/std/experimental/reflection/base-splice.verify.cpp) |
| 3293-02 | A virtual base class subobject cannot be spliced. | Covered | [base-splice.verify.cpp](../../libcxx/test/std/experimental/reflection/base-splice.verify.cpp) |
| 3293-03 | A base-subobject splice cannot be applied through an array element or to a non-base reflection. | Covered | [base-splice.verify.cpp](../../libcxx/test/std/experimental/reflection/base-splice.verify.cpp) |

## P3394R4 — Annotations for Reflection

Normative source: [P3394R4](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3394r4.html).

| ID | Condition | Status | Existing coverage |
|---|---|---|---|
| 3394-01 | The annotation operand is a constant expression and, after the required conversion, has structural type. | Needs-New-Test | — |
| 3394-02 | An annotation cannot appear in the attribute-specifier-seq of a type-specifier-seq. | Covered | [annotations-regression.verify.cpp](../../libcxx/test/std/experimental/reflection/annotations-regression.verify.cpp) |
| 3394-03 | An annotation cannot appear on an empty-declaration. | Needs-New-Test | — |
| 3394-04 | An annotation cannot be mixed with ordinary attributes in one attribute-specifier. | Needs-New-Test | — |
| 3394-05 | `annotations_of_with_type(item, type)` returns only annotations whose `type_of` equals `type`; invalid item/type reflections are ill-formed as specified. | Needs-New-Test | — |
| 3394-06 | Annotation accumulation and order preservation across repeated annotations and redeclarations must be diagnosed/rejected correctly when the syntax is invalid. | Needs-New-Test | — |

## P3491R3 — `define_static_{string,object,array}`

Normative source: [P3491R3](https://www.open-std.org/JTC1/SC22/WG21/docs/papers/2025/p3491r3.html).

| ID | Condition | Status | Existing coverage |
|---|---|---|---|
| 3491-01 | `reflect_constant_string`/`define_static_string`: range value type is `char`, `wchar_t`, `char8_t`, `char16_t`, or `char32_t`. | Blocked-On-Unimplemented-Facility | `reflect_constant_string` is currently only `char`/`char8_t`; `define_static_string` is not fully aligned. |
| 3491-02 | String input's elements are constant-evaluable; a string-literal input excludes exactly its trailing null character before re-termination. | Blocked-On-Unimplemented-Facility | String-literal detection (`is_string_literal`) is absent. |
| 3491-03 | `reflect_constant_array`/`define_static_array`: element type is structural, constructible from the range reference, and copy-constructible. | Needs-New-Test | — |
| 3491-04 | `reflect_constant_array`/`define_static_array`: every element's `reflect_constant` is a constant subexpression. | Needs-New-Test | — |
| 3491-05 | `define_static_object`: `remove_cvref_t<T>` is structural and constructible from `T`. | Blocked-On-Unimplemented-Facility | `define_static_object` is absent. |
| 3491-06 | `define_static_object`: the argument initializes the required template-parameter object. | Blocked-On-Unimplemented-Facility | `define_static_object` is absent. |
| 3491-07 | `is_string_literal` overloads require an accepted string-literal/reference form and reject non-string objects. | Blocked-On-Unimplemented-Facility | Facility is absent. |
| 3491-08 | Results are potentially non-unique objects and array/string extraction must preserve the specified extent and element initialization. | Needs-New-Test | — |

## P3560R2 — Error Handling in Reflection

Normative source: [P3560R2](https://wg21.link/P3560R2). These rows are `Throws` obligations, not
ordinary hard-diagnostic obligations. The thirteen library-side wrappers listed in the M2 audit
exist, but their failure paths still need catch-and-inspect tests. The remaining compiler-side
paths cannot be tested as `meta::exception` until the strategy-2 evaluator/failure-sink design is
implemented; each still needs a separate `-verify` hard-diagnostic test where the old `DiagFn`
path is the current contract.

| ID | Condition | Status | Existing coverage |
|---|---|---|---|
| 3560-01 | `members_of` is called on a reflection that is not a complete class type or namespace. | Blocked-On-Unimplemented-Facility | Compiler-side throw path not wired. |
| 3560-02 | `bases_of`, `static_data_members_of`, or `nonstatic_data_members_of` is called on a non-complete/non-class type. | Blocked-On-Unimplemented-Facility | Compiler-side throw path not wired. |
| 3560-03 | `parameters_of` or `return_type_of` is called on a non-function reflection. | Blocked-On-Unimplemented-Facility | Compiler-side throw path not wired. |
| 3560-04 | `variable_of` is called on a non-parameter reflection or outside its valid frame. | Blocked-On-Unimplemented-Facility | Compiler-side throw path not wired. |
| 3560-05 | `identifier_of`, `u8identifier_of`, `display_string_of`, or `source_location_of` is called where the represented construct has no applicable name/location. | Blocked-On-Unimplemented-Facility | Compiler-side throw path not wired. |
| 3560-06 | `type_of`, `parent_of`, `object_of`, or `constant_of` is called where the reflection has no corresponding result. | Blocked-On-Unimplemented-Facility | Compiler-side throw path not wired. |
| 3560-07 | `dealias`/`underlying_entity_of`/`proxied_entity_of` is called for a reflection with no applicable underlying entity. | Blocked-On-Unimplemented-Facility | Entity-proxy failure path is not exception-enabled. |
| 3560-08 | `is_accessible` receives an invalid construct or invalid access context. | Blocked-On-Unimplemented-Facility | Compiler-side throw path not wired. |
| 3560-09 | `extract<T>` is used when the reflected value/member/function cannot be converted to `T`. | Blocked-On-Unimplemented-Facility | Compiler-side throw path not wired. |
| 3560-10 | `can_substitute`/`substitute` receives a non-template or unusable template arguments. | Blocked-On-Unimplemented-Facility | Compiler-side throw path not wired. |
| 3560-11 | `annotations_of`/`annotations_of_with_type` receives an invalid target or filter reflection. | Blocked-On-Unimplemented-Facility | Annotation traversal still uses `DiagFn`. |
| 3560-12 | `reflect_constant`, `reflect_object`, or `reflect_function` violates its type or constant-template-argument requirements. | Blocked-On-Unimplemented-Facility | Compiler-side throw path not wired. |
| 3560-13 | `data_member_spec` violates its type, name, width, alignment, or option-combination requirements. | Blocked-On-Unimplemented-Facility | Compiler-side throw path not wired. |
| 3560-14 | `has_inaccessible_nonstatic_data_members` is called when its member query is not a constant subexpression or on a closure type. | Needs-New-Test | `meta::exception` wrapper exists; no negative catch test. |
| 3560-15 | `has_inaccessible_bases` is called when its base query is not a constant subexpression. | Needs-New-Test | `meta::exception` wrapper exists; no negative catch test. |
| 3560-16 | `size_of`, `bit_size_of`, or `alignment_of` receives a disallowed reflection or incomplete type. | Needs-New-Test | `exception.pass.cpp` has positive coverage only. |
| 3560-17 | `template_of` or `template_arguments_of` receives a reflection without template arguments. | Needs-New-Test | `exception.pass.cpp` has positive coverage only. |
| 3560-18 | `access_context::via` receives a non-class reflection. | Needs-New-Test | `exception.pass.cpp` has positive coverage only. |
| 3560-19 | `enumerators_of`, `offset_of`, `operator_of`, or `subobjects_of` receives a reflection outside its specified domain. | Needs-New-Test | `exception.pass.cpp` has positive coverage only. |
| 3560-20 | `define_aggregate` failure remains unrecoverable and must retain its required hard diagnostic rather than become a catchable throw. | Covered | [define-aggregate.verify.cpp](../../libcxx/test/std/experimental/reflection/define-aggregate.verify.cpp) |

## P3617R0 — `reflect_constant_{array,string}`

Normative source: [P3617R0](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3617r0.html).

| ID | Condition | Status | Existing coverage |
|---|---|---|---|
| 3617-01 | `reflect_constant_string`: range value type is one of the five permitted character types. | Needs-New-Test | — |
| 3617-02 | String-literal input omits its trailing null character before the new terminator is added. | Needs-New-Test | — |
| 3617-03 | `reflect_constant_array`: element type is structural, constructible from the range reference, and copy-constructible. | Needs-New-Test | — |
| 3617-04 | The resulting array/string object is potentially non-unique and has the specified template-parameter-object type and extent. | Needs-New-Test | — |

## P3687R1 — Final Adjustments to C++26 Reflection

Normative source: [P3687R1](https://wg21.link/P3687R1).

| ID | Condition | Status | Existing coverage |
|---|---|---|---|
| 3687-01 | Splice-template-arguments are removed from C++26 and must be rejected in template-argument position. | Needs-New-Test | — |
| 3687-02 | `^^qualified-id` is ill-formed when lookup finds a declaration replacing a using-declarator. | Needs-New-Test | — |
| 3687-03 | If lookup finds multiple entity proxies, the reflection is ambiguous and ill-formed. | Needs-New-Test | — |

## P3795R2 — Miscellaneous Reflection Cleanup

Normative source: [P3795R2](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p3795r2.html).

| ID | Condition | Status | Existing coverage |
|---|---|---|---|
| 3795-01 | `current_function()` is usable only while evaluating within a function; otherwise the required failure is produced. | Needs-New-Test | — |
| 3795-02 | `current_class()` is usable only while evaluating within a class member context; otherwise the required failure is produced. | Needs-New-Test | — |
| 3795-03 | `current_namespace()` is usable only where a current namespace can be identified; otherwise the required failure is produced. | Needs-New-Test | — |
| 3795-04 | Generated data-member annotations must satisfy the same constant-expression/structural-value constraints as source annotations. | Needs-New-Test | — |

## Execution plan

Work facility-first, with paper ownership retained in the checklist. Start with the implemented
hard-diagnostic rows in P2996R13/P1306R5/P3096R12 and the small P3293R3/P3394R4/P3617R0/P3687R1
clusters. These are cheap one-reproducer rows and expose diagnostic-shape problems before the
larger exception migration. Then do P3491R3 once its missing facilities are implemented.

Treat P3560R2 as its own epic-sized track: first add catch-and-inspect tests for the thirteen
existing library wrappers in batches of 3–5 functions, then implement and test strategy 2 in
facility slices (`members/bases`, parameter/result queries, extraction/substitution,
annotations/data-member-spec). Keep each batch independently buildable and committed. This
matches the successful P3560 strategy-1 breakdown and avoids mixing hard diagnostics with the
evaluator redesign that blocked the earlier pilot.

Recommended session size is one focused batch of 4–8 rows, never a whole paper if it crosses
facility boundaries. After each batch: refresh staged libc++ headers, run the focused libc++ lit
test and the relevant Clang reflection tests, inspect diagnostics with `-verify`, and update this
table. Do not mark a row Covered merely because compilation fails: the expected diagnostic must
be matched. The final M5 gate is a zero-count scan of this table, followed by the M6 full-suite
baseline comparison.
