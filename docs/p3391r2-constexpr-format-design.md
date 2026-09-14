# P3391R2: `constexpr` `std::format`

P3391R2 adds `__cpp_lib_constexpr_format` with value `202511L`, provided by
`<format>`. This is separate from `__cpp_lib_format`. The macro remains
unimplemented until a compile-time end-to-end `std::format` test passes.

The constexpr-enabled surface is format entry points, argument storage and
visitation, buffers, parsing, and builtin formatters for booleans, characters,
integral values, pointers, strings, tuples, and output helpers. Floating-point
and all chrono formatting, locale-aware overloads, and the `stacktrace_entry`,
`filesystem::path`, `thread::id`, and `void const*` formatters remain
non-constexpr: P3391R2 explicitly excludes them.

Only functions whose operations support constant evaluation will be annotated.
Out-of-line definitions and matching friend declarations require separate
review in addition to the usual `_LIBCPP_HIDE_FROM_ABI` annotation pass.
