# Reflection Closure Epic — M1 PR Triage Final Report

Date: 2026-09-09

M1 is complete: the issue triage and all 35 open upstream PRs are now dispositioned. This
session reviewed the descriptions and diffs for the 11 formerly unassessed PRs and re-confirmed
the four previously dispositioned cross-check PR groups.

## Formerly unassessed PRs

- **#340 — `has_c_language_linkage`:** adopted P2996R13 facility; absent from this fork's
  `Missing` list. In scope. Upstream supplies a ready-made compiler/library/test implementation;
  defer port to M4 with wording review.
- **#345 — immediate-function values:** fix for reflecting pointers/member-pointers to immediate
  functions by allowing them inside reflected values and enforcing the restriction at splice time.
  Maps to the #184/#334 consteval-only cluster: #334 is already fixed, while #184 remains
  Needs-Build-To-Verify. Ready-made upstream fix; defer port until a buildable reproducer/gate.
- **#279 — unresolved lookup:** permits a single function template in `UnresolvedLookupExpr` and
  includes dependent splice-type canonicalization/test coverage. Maps to #273 and #276, both
  already fixed independently in this fork; no port required.
- **#261 — expansion body instantiation:** broad AST/Sema/TreeTransform change to defer body
  instantiation and avoid premature diagnostics. Maps to confirmed-open expansion issues #180
  and related #181. Ready-made upstream fix exists; future M4 port/adaptation.
- **#249 — upstream `91cdd350` AST representation merge:** 548-file, 12k-line general LLVM
  synchronization merge, not a reflection-specific fix or C++26 facility. Out of scope for this
  closure epic; relevant fork synchronization has been handled independently.
- **#244 — template falsely treated as overload set:** handles failed invented-`auto` deduction
  while reflecting a unique template such as a deduced-this lambda operator. Maps to confirmed-
  open #239. Ready-made upstream Sema fix exists; future M4 port/adaptation.
- **#207 — P2996 examples:** Docker/build scaffolding and examples only. Low-priority/out-of-scope
  for compiler/library conformance closure.
- **#168 — string literal manipulation:** early implementation of P3491R3 string-literal
  facilities (`is_string_literal` and helpers), matching the paper audit's confirmed missing
  facility. In scope; ready-made upstream implementation exists, but requires current-wording/API
  review before a future M4 port.
- **#166 — `is_reflection_type`:** one-line alias/dealias correction for `std::meta::info`.
  In-scope P2996R13 library correctness fix; ready-made upstream fix exists. Not ported during this
  documentation-only session; schedule with a focused library test.
- **#135 — AST dump support:** text/JSON dump visitors and tests for splice specifiers and splice
  types. Tooling coverage only, not a conformance gap; low priority/out of scope for closure.
- **#124 — annotation equality constraint:** adds `requires equality_comparable<T>` to the legacy
  `experimental/meta` helper. This is relevant to P3394R4 annotation Mandates, but the diff
  targets the legacy API; adapt against current `meta` in a future M4/M5 pass rather than port
  verbatim.

## Re-confirmed cross-checks

- **#227/#195/#170:** all implement competing `consteval_hash<std::meta::info>`/P3816 attempts.
  P3816 remains a separate, non-adopted proposal, so these are out of scope and are not completion
  blockers.
- **#163:** P3074 trivial-union work is language-side and belongs in `docs/CXX26_GAPS.md`, not
  this reflection tracker.

No PR implementation was ported in this session. The tracker table in
[`../REFLECTION_GAPS.md`](../REFLECTION_GAPS.md) contains the authoritative dispositions and
future-port notes.
