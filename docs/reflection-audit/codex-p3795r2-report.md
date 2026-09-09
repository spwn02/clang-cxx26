Completed and pushed:

- Scope functions: `current_function`, `current_class`, `current_namespace`
- Tuple metafunctions: `is_applicable_type`, `is_nothrow_applicable_type`, `apply_result`
- Verified existing parameter annotations support
- Documented remaining `data_member_options.annotations` work

Commits:

- `affd010255d5`
- `6533a01f6dcc`
- `52ee0eca17a5`

Tests passed through `libcxx-lit`; audit report: [codex-p3795r2-report.md](/home/spawn/dev/toolchains/clang-p2996/docs/reflection-audit/codex-p3795r2-report.md).