# Reflection Closure Epic — M4 batch 2

Date: 2026-09-09

## Closed

- #286, #298, #300, #312: fixed by `2262084ae5e1`; tracker reconciliation `e30b1d38e1c5`.
- #319: fixed by `357aff58a79b`; tracker reconciliation `e30b1d38e1c5`.
- #290: already correctly recorded as fixed by `16a3f705ef1a`.
- #326: covered by `c1d0c5075bbb`; tracker reconciliation `edff98950de9`.
- #280: `has_parent(info)` and regression coverage in `c21e31f8eb5e`; tracker hash `98b5a679110d`. Focused test passed; reflection Clang tests passed 18/18.
- #185: `annotations_of_with_type(info, info)` and coverage in `625ed6cec16a`; tracker hash `cd1f8726cd5b`. Focused test passed.

## Skipped

- #120: Windows/MSVC-only mangling; needs Microsoft `mangleReflection`, replacement of the reflection `llvm_unreachable`, and ClangCL ABI tests. `7728197fac9d`.
- #146: expansion `case`/`default` labels need a control-flow-limited rule across parser/Sema and CodeGen. `9edab83e8612`.
- #150: `~[:info:]` needs a new destructor-name parser/Sema path. `c8371b490ac4`.
- #182: CodeGen needs discarded-expansion-instance awareness for continuation destinations; overlaps the deferred consteval cluster. `6c14f61224a4`.
- #200: alias-layer preservation in `parent_of` needs wording-specific semantics and tests. `a03bcd42359c`.
- #254: evaluator constexpr-step budget limitation; callers can raise `-fconstexpr-steps`, and chunking cannot guarantee acceptance. `93e5abeed1c2`.
- #329: evaluator-state/callback PCH serialization redesign, not a safe local serializer edit. `5204818b6e5b`.
- #334: static-member constant-expression classification needs a focused implementation and regression gate. `5204818b6e5b`.

GitHub API access failed repeatedly with `error connecting to api.github.com`; local snapshot data was used where available.

## Left

The remaining paper gaps, #225/meta::exception (separate design session), P3795R2, P3293R3, and the M3 escalation cluster remain intentionally unattempted. The constrained full Clang gate reached 27,905 passes and 15 failures before interruption: five known consteval/baseline failures and ten unrelated ClangScanDeps failures. The focused reflection gate passed.
