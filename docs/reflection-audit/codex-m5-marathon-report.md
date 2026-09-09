# M5 reflection diagnostic marathon report

Date: 2026-09-10

This session continued from batch 6 after rereading the Ground Truth section of
`docs/REFLECTION_GAPS.md`, the M5 checklist, and `codex-m5-batch6-report.md`. Work stopped after
four additional facility-first batches at a natural boundary: remaining P3491 rows depend on
missing `define_static_object`/`is_string_literal`, while P3560 strategy-2 rows depend on the
explicitly deferred evaluator exception plumbing. Known NEW-1/NEW-2/NEW-3 were not re-investigated.

## Batch 7 — P2996R13 rows 2996-16, -17, -18, -20

Added and executed `m5-p2996-batch7.verify.cpp` through the libc++ lit wrapper (`1/1 passed`).
Covered invalid `template_arguments_of`, invalid `access_context::via` use in `is_accessible`,
non-class input to `has_inaccessible_nonstatic_data_members`, and non-class input to
`has_inaccessible_bases`.

The closure-type probe for 2996-19 was accepted instead of rejected. Recorded as NEW-4.
Commit: `30623140f8b6` (`reflection: cover P2996 M5 batch 7`), pushed and verified clean against
`origin/cxx26`.

## Batch 8 — P2996R13 rows 2996-21 through -25

Added and executed `m5-p2996-batch8.verify.cpp` (`1/1 passed`). Covered invalid domains for
`members_of`, `bases_of`, `static_data_members_of`, `nonstatic_data_members_of`, and
`enumerators_of`.

No new gap found. Commit: `5450cd67075c` (`reflection: cover P2996 M5 batch 8`), pushed and
verified clean against `origin/cxx26`.

## Batch 9 — P2996R13 rows 2996-26 through -32

Added and executed `m5-p2996-batch9.verify.cpp` (`1/1 passed`). Covered invalid `offset_of`,
invalid and incomplete `size_of`, invalid and incomplete `alignment_of`, and invalid and
incomplete `bit_size_of` cases.

No new gap found. Commit: `64d088ec3d07` (`reflection: cover P2996 M5 batch 9`), pushed and
verified clean against `origin/cxx26`.

## Batch 10 — P2996R13 rows 2996-33 through -37

Added and executed `m5-p2996-batch10.verify.cpp` (`1/1 passed`). Covered invalid value, member,
function, pointer, non-template `can_substitute`, and invalid `substitute` extraction/substitution
cases.

No new gap found. Commit: `056124cbf6c7` (`reflection: cover P2996 M5 batch 10`), pushed and
verified clean against `origin/cxx26`.

## Batch 11 — P2996R13 rows 2996-38 through -40

Added and executed `m5-p2996-batch11.verify.cpp` (`1/1 passed`). Covered invalid member type,
invalid member name, conflicting width/alignment, negative width, invalid alignment, and invalid
bit-field type options.

A separate `data_member_spec(^^void)` probe was accepted although the paper requires an object or
reference type. Recorded as NEW-5 without treating the passing reflected-value test as coverage
of the `void` case. Commit: `c78e78300bba` (`reflection: cover P2996 M5 batch 11`), pushed and
verified clean against `origin/cxx26`.

## New gaps

- NEW-4: `has_inaccessible_nonstatic_data_members` accepts a closure type.
- NEW-5: `data_member_spec(^^void)` is accepted despite the object/reference type requirement.

Existing NEW-1, NEW-2, and NEW-3 remain unchanged.

## Final checklist state

The checklist contains 109 condition rows. Exact status-column count is:

| Covered | Needs-New-Test | Blocked-On-Unimplemented-Facility | Not-Applicable |
|---:|---:|---:|---:|
| 67 | 23 | 18 | 1 |

The 67 covered rows include all newly completed rows listed above. Remaining unblocked rows are
P2996R13 2996-09, -19, -46 through -49; P3096R12 3096-06; P3394R4 3394-03 and -05; P3491R3
3491-03, -04, and -08; P3560R2 3560-14 through -19; P3617R0 3617-01; and P3795R2 3795-01
through -04. P3491 missing-facility rows and P3560 compiler-side exception rows remain blocked
as documented in the checklist.

