# M5 reflection diagnostic marathon 2

Date: 2026-09-10

This session reread the Ground Truth, M5 checklist, and marathon-1 report before continuing
facility-first. Each new test was compiled with the built `clang++` using `-fsyntax-only` and
Clang `-verify`. The libc++ wrapper could not start in this sandbox because Python's lit
forkserver is denied by the environment; the direct compiler runs completed successfully.

## Batch 12 — P2996R13

Added `m5-p2996-p3096-batch12.verify.cpp` and covered:

- 2996-46: reflection of a requires-expression local parameter is rejected.
- 2996-47: reflection naming a using-declarator is rejected.

Also classified P3617-01 as `Blocked-On-Unimplemented-Facility`: the adopted generic character
overloads are absent for `wchar_t`, `char16_t`, and `char32_t`.

Commit `c5fb52b44d35` (`reflection: cover P2996 M5 batch 12`) was pushed and verified against
`origin/cxx26`.

## Batch 13 — P2996R13 / P3096R12 audit

Added and ran `m5-p2996-batch13.verify.cpp`. The constructor and destructor probes, and the
dependent splice in the CTAD-like declaration, compiled without diagnostics. The probes were
therefore not marked Covered:

- 2996-48 became NEW-6: `^^S::S` and `^^S::~S` are accepted.
- 2996-49 became NEW-7: dependent `[:R:] value = {1}` is accepted.
- 3096-06 became NEW-8: non-parameter `identifier_of`, `u8identifier_of`, and
  `has_identifier` are accepted; `type_of(^^S)` still diagnoses because a type reflection has
  no type.

Commit `f269d385b759` (`reflection: audit P2996 M5 batch 13`) was pushed and verified against
`origin/cxx26`.

## Batch 14 — P3795R2

Added `m5-p3795-batch14.verify.cpp` and covered:

- 3795-01: `current_function()` outside a function fails constant evaluation.
- 3795-02: `current_class()` outside a class/member context fails constant evaluation.
- 3795-03: `current_namespace()` resolves the global namespace.

The test uses `-verify-ignore-unexpected=note` because the current exception construction emits
implementation-detail constant-evaluation notes in addition to the required failure.

Commit `fab26539718e` (`reflection: cover P3795 M5 batch 14`) was pushed and verified against
`origin/cxx26`.

## Batch 15 — P3491R3

Added `m5-p3491-batch15.verify.cpp` and covered:

- 3491-03: a non-structural element type is rejected during constant array reflection.
- 3491-08: static-array extent, initialization, and extraction preserve the requested values.

Commit `04b33be766d5` (`reflection: cover P3491 M5 batch 15`) was pushed and verified against
`origin/cxx26`.

## New gaps

NEW-6, NEW-7, and NEW-8 are documented in `docs/REFLECTION_GAPS.md` and the checklist. Existing
NEW-1 through NEW-5 were not re-investigated.

## Final checklist counts

The checklist now has 109 condition rows:

| Covered | Needs-New-Test | Blocked-On-Unimplemented-Facility | Not-Applicable |
|---:|---:|---:|---:|
| 74 | 15 | 19 | 1 |

Remaining Needs-New-Test rows are 2996-09, 2996-19, 2996-48, 2996-49, 3096-06, 3394-03, 3394-05,
3491-04, 3560-14 through 3560-19, 3795-04, plus the rows represented by NEW-6 through NEW-8.
M5 is not yet exhausted: Needs-New-Test rows remain. The 19 blocked rows are the P3491 missing
facilities and P3560 strategy-2 compiler-side exception paths, as recorded in the checklist.
