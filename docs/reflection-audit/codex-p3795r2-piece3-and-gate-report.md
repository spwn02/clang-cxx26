# P3795R2 piece 3 and check-clang gate

Date: 2026-09-09

## Piece 3

Implemented and committed as `47e5bba54f7e` (`reflection: implement P3795R2 data_member_options annotations`).

- Added `data_member_options::annotations`, matching `enumerator_options::annotations`.
- Extended the `data_member_spec` ABI from 12 to 14 arguments and threaded annotation reflections through the data-member specification.
- Validated annotation reflections as typed, non-array constants and used `constant_of` for the library-side validation path.
- Stored the reflections in `TagDataMemberSpec`, lowered them to annotation values during aggregate definition, and attached `CXX26AnnotationAttr` to generated fields.
- Added `p3795-data-member-annotations.pass.cpp`, covering two annotations returned by `annotations_of` on a generated member and the `meta::exception` throw/catch machinery.

Focused result: 1 passed, 0 failed through `libcxx-lit -j1` with the Python `fork` start method. The normal lit launcher cannot create its default `forkserver` socket in this sandbox.

## Definitive gate

Command:

```text
python3 -c "import multiprocessing,runpy; multiprocessing.set_start_method('fork'); runpy.run_path('build-nyx/bin/llvm-lit', run_name='__main__')" -j1 -sv clang/test
```

The uninterrupted serial sweep is still running at report-generation time. Its first five failures are exactly the established baseline:

- `SemaCXX/PR98671.cpp`
- `SemaCXX/constant-expression-cxx11.cpp`
- `SemaCXX/builtin-is-within-lifetime.cpp`
- `SemaCXX/cxx2b-consteval-propagate.cpp`
- `SemaCXX/cxx2a-constexpr-dynalloc.cpp`

No reflection test has failed in the completed portion. Final pass/unsupported/expected-failure totals will be appended after lit exits; this file must not be treated as the definitive gate result until that completion entry is present.
