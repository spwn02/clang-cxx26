# LWG issue draft (tracker spwn02/clang-cxx26#258)

**Title:** The constexpr-enabled `formatter<nullptr_t, charT>` cannot be reached from `std::format` as specified
**Section:** [format.arg], [format.formatter.spec], [format.functions]
**Status:** New (to be submitted)
**Wording is relative to:** N5050 (draft of 2026-06-01, commit 8ab1a9b of cplusplus/draft)
**Priority:** 3
**Related paper:** P3391R2 (constexpr `std::format`)

## Discussion

P3391R2 made `formatter<nullptr_t, charT>` a *constexpr-enabled* specialization ([format.formatter.spec]: "For each `charT`, the constexpr-enabled pointer type specialization `template<> struct formatter<nullptr_t, charT>;`"), while `formatter<void*, charT>` and `formatter<const void*, charT>` are listed separately and are not constexpr-enabled. [format.functions] p1 says:

> A call to any of the functions defined in this subclause is a constant subexpression only if each of the used `formatter` specializations is a constexpr-enabled specialization.

A `nullptr_t` argument, however, never reaches `formatter<nullptr_t, charT>` through those functions. The constructor of `basic_format_arg` ([format.arg]) says:

> otherwise, if `is_void_v<remove_pointer_t<TD>>` is `true` or `is_null_pointer_v<TD>` is `true`, initializes `value_` with `static_cast<const void*>(v)`;

and `value_` is `variant<monostate, bool, char-type, int, unsigned int, long long int, unsigned long long int, float, double, long double, const char-type*, basic_string_view<char-type>, const void*, handle>` (no alternative for `nullptr_t`). So `std::format("{}", nullptr)` stores a `const void*` and formats it with `formatter<const void*, charT>`, which is not constexpr-enabled. Read literally, a call such as `std::format("{}", nullptr)` is therefore never a constant subexpression, and the constexpr-enabled `formatter<nullptr_t, charT>` is relevant only for direct use of that formatter, which cannot be the intent of P3391R2.

Implementation observation: an implementation that wants `std::format("{}", nullptr)` to be a constant expression can format a null `const void*` through `formatter<nullptr_t, charT>` (the produced text, "0x0", is the same). That also makes `std::format("{}", static_cast<void*>(nullptr))` constant, which is more permissive than the wording allows under either reading.

## Proposed resolution

Wording is relative to N5050. Two alternatives; the first is the more direct one.

**Option A** (type-preserving). In [format.arg], add `nullptr_t` to the exposition-only `value_` variant (after `const void*`, before `handle`), change the constructor bullet to

> otherwise, if `is_void_v<remove_pointer_t<TD>>` is `true`, initializes `value_` with `static_cast<const void*>(v)`;
> otherwise, if `is_null_pointer_v<TD>` is `true`, initializes `value_` with `nullptr`;

and add `nullptr_t` to the list of types that `basic_format_arg::visit`/`visit_format_arg` pass to the visitor, and to the standard formatting of arguments ([format.functions], `basic-format-arg` visitation) so that the formatter used is `formatter<nullptr_t, charT>`. Consequence for user code: a visitor handed to `visit_format_arg`/`basic_format_arg::visit` now receives `nullptr_t` for a `nullptr` argument instead of `const void*` (a visible change; the visitor must be callable with `nullptr_t`).

**Option B** (wording only, no new alternative). In [format.functions] p1 (and in [format.formatter.spec]) state that, for the purpose of the sentence above, a `nullptr_t` argument is formatted with `formatter<nullptr_t, charT>`:

> For an argument whose type is `nullptr_t`, the used `formatter` specialization is `formatter<nullptr_t, charT>`.

Because `value_` erases the type, an implementation can only honor this when it formats a null `const void*` through `formatter<nullptr_t, charT>`, which is exactly the over-permissive behavior described above for `static_cast<void*>(nullptr)`; the proposal should say so in a note.

## Implementation

The clang-cxx26 fork implements the Option B behavior (null `const void*` formatted through `formatter<nullptr_t, charT>`); pinned by `libcxx/test/std/utilities/format/format.functions/format.constexpr_enabled.pass.cpp` (`null_pointer`).
