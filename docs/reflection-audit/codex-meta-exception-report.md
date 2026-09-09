Implemented and pushed:

- `std::meta::exception` with required constructors, accessors, special members, and `std::exception` inheritance.
- Initial LWG 4428 strategy (1) wrappers for three metafunctions.
- Regression test covering construction, accessors, and direct consteval try/catch.
- Tracker updated to Partial.
- Final report: [codex-meta-exception-report.md](/home/spawn/dev/toolchains/clang-p2996/docs/reflection-audit/codex-meta-exception-report.md)

Commits:

- `83ad2eaa8193`
- `d90c3cfca29c`

Both pushed to `origin/cxx26`. Focused exception test passes. Nested consteval-call exception catching remains a compiler limitation documented in the report.