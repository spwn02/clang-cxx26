# `meta::exception` strategy-1 batch 3

Date: 2026-09-09

## Result

Four additional strategy-1 wrappers were retained and committed:

- `template_of`
- `template_arguments_of`
- `access_context::via`
- `enumerators_of`

Together with the six functions completed in batches 1 and 2 (`size_of`, `bit_size_of`,
`alignment_of`, `has_inaccessible_nonstatic_data_members`, `has_inaccessible_bases`, and
`annotations_of_with_type`), the fork now has **10 library-side strategy-1 wrappers**.

`exception.pass.cpp` contains positive coverage for the retained query groups. Each focused
libc++ lit run passed through `libcxx/utils/libcxx-lit`, which refreshed staged headers.

## Strategy-2 remainder

The following were deliberately not retained as library-only guards:

- `members_of`, `bases_of`, `static_data_members_of`, and
  `nonstatic_data_members_of`: entity proxies, bit-fields, incomplete/defined aggregates, and
  definition effects require compiler-side state.
- `parameters_of` and `return_type_of`: builtin-template and parameter reflection diagnostics
  cannot be reproduced reliably from the exposed predicates.
- Deep access and annotation failures, including `is_accessible` and the deep paths of the two
  inaccessible-member queries.
- `offset_of`, `extract`, `can_substitute`/`substitute`, `reflect_constant`, `reflect_object`,
  `reflect_function`, `variable_of`, and `define_aggregate`.

These remain strategy-2 work requiring compiler-side conversion of the `DiagFn` path to a
catchable `meta::exception`.

## Final gate

Because this batch touched only libc++ headers/tests and documentation, the permitted final gate
was the libc++ reflection sweep:

```text
libcxx/test/std/experimental/reflection/: 75 passed, 1 unsupported, 0 failed
Total discovered: 76
```

The first sweep exposed unsafe member-query guards; those guards were reverted and the final
sweep above is clean. The layout-query wrappers were also adjusted to preserve existing
zero-width bit-field, aligned-reference-variable, and data-member-spec behavior; the focused
layout test and final sweep both pass.
