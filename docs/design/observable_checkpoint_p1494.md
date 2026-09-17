# Design note: `std::observable_checkpoint` (P1494R5 + P3641R0, issue #30)

Status: design-only, not implemented. Written per this project's standing
policy that greenfield facilities get a design note before any
implementation dispatch. **Correction to this issue's own stub**: the
`docs/CXX26_GAPS.md` entry this was filed from described it as "Rename +
deprecation, likely small." That is wrong — the actual work is a new
optimizer-visible LLVM IR primitive plus new optimizer-pass logic, not a
library-side rename. P3641R0 itself really is just a rename
(`std::observable` → `std::observable_checkpoint` plus the feature-test
macro); the substantive work belongs entirely to P1494R5, which this rename
target depends on for its actual semantics.

## What the papers actually specify

- **Signature**: `void observable_checkpoint() noexcept;` — a free function,
  no arguments, no return value, not `constexpr`.
- **Effects** (P1494R5, "General solution"): "Establishes an observable
  checkpoint." The normative hook, direct from the paper: a conforming
  implementation "can implement `std::observable` efficiently as an
  intrinsic that counts as a possible termination, which the optimizer thus
  cannot remove."
- **What "counts as a possible termination" actually buys**: this is not a
  plain memory-ordering/reordering barrier (`atomic_signal_fence`,
  `asm volatile("" ::: "memory")`). Those barriers stop instruction
  reordering and prevent stores from being coalesced/eliminated *across*
  the barrier — but they do **not** stop the specific class of optimization
  P1494R5 is worried about: an optimizer that proves later, unconditional
  undefined behavior is reached is currently permitted to retroactively
  treat *everything before that point* as unreachable too (the standard's
  own "if UB, the whole execution is undefined" licenses this backward
  propagation; real optimizers exploit it — e.g. deleting a bounds check
  because the following out-of-bounds access is provably UB, or discarding
  earlier `printf` calls whose only observable purpose was preceding a
  later null-pointer dereference). `observable_checkpoint()` exists so a
  program can mark a point after which "no matter what happens later, the
  observable behavior up to here already genuinely happened" — blocking
  that backward propagation across the checkpoint specifically.
- **No `constexpr`, no evaluation-order requirement, no interaction named
  with `[intro.progress]`'s forward-progress guarantees.** The guarantee is
  specifically about *optimizer* behavior; it is meaningless during
  constant evaluation, where no such backward-UB-propagation optimization
  ever applies. A `constexpr` overload is not required by the paper and
  isn't proposed here.

## Why this needs a genuinely new LLVM primitive, not an existing one

Surveyed LLVM's own intrinsic set for anything already combining "no side
effect on memory contents" + "may not return" + "opaque to the optimizer's
backward-UB reasoning":

- `llvm.donothing` (`IntrNoMem`) has no side effects at all — nothing stops
  the optimizer treating code around it as dead.
- `llvm.sideeffect` (`IntrInaccessibleMemOnly`) is explicitly documented as
  `willreturn` (LangRef) — it guarantees the call returns and only blocks
  loop-deletion/`mustprogress`-style transforms. `willreturn` is exactly the
  wrong property: it's what tells the optimizer this call can be assumed to
  complete normally, which is the opposite of "might terminate the
  program."
- `llvm.eh.sjlj.longjmp` (`IntrNoReturn`) and
  `llvm.experimental.deoptimize` (`Throws`) do model "may not return," but
  both are unconditional-noreturn/deopt constructs — calling either one
  *always* diverts control flow. `observable_checkpoint()` must be an
  ordinary call that *normally returns* (the common case), while still
  being treated by the optimizer as *possibly* not returning, for the
  narrow purpose of blocking backward-UB propagation across it.

No existing intrinsic occupies this combination. This needs a new one —
call it `llvm.observable.checkpoint()` in what follows — with:

- `IntrHasSideEffects` (it is not eliminable, not readnone/pure).
- **Explicitly not `willreturn`** (this is the entire point).
- `IntrNoMerge` + `IntrNoDuplicate` (two checkpoint call sites are distinct
  program points; CSE/loop-unrolling merging or duplicating them would
  change which points are considered "reached," which is observable for
  this primitive's purpose in a way it normally isn't for an ordinary call).

## Optimizer-side work (the actual scope, and the actual risk)

A new intrinsic with the right attributes is necessary but not sufficient.
Declaring it non-`willreturn` stops the *forward* inferences that key off
`willreturn` (e.g. treating a loop containing only this call as
provably-terminating), but the backward-UB-propagation behavior P1494R5
targets is driven by separate logic: `Attributor`'s undefined-behavior and
`AAWillReturn`/`AAUndefinedBehavior` analyses, and pattern-based folding in
`InstCombine`/`SimplifyCFG` that recognizes "this block provably reaches
`unreachable`" and propagates that fact to predecessors, deleting or
reordering their code. Making `observable_checkpoint()` actually work
requires auditing and teaching each such pass to treat a call to
`llvm.observable.checkpoint()` as an opaque barrier to that specific
propagation — stop the backward walk there rather than continuing into the
checkpoint's predecessors.

This is meaningfully different in kind from every other piece of work this
fork has done to date: it is not a new builtin plus a CodeGen lowering (that
part is the easy, well-precedented half — see below); it is new *general
optimizer semantics* that must hold correctly across every pass that does
UB-based backward reasoning, most of which live in shared LLVM
infrastructure exercised by every consumer of LLVM, not just this fork's
own test suite. Getting this wrong either leaves the guarantee unenforced
(the checkpoint compiles but does nothing, silently) or, more dangerously,
could interact badly with legitimate UB-based optimizations elsewhere if
the barrier is modeled too broadly. This is the kind of change that would
normally go through an upstream LLVM RFC before landing, not a
same-session Sema/CodeGen patch — hence "its own multi-session
undertaking," not implemented in this pass.

## The easy half, for when the optimizer-side work is scoped: builtin + library plumbing

For reference, the Clang-side plumbing is straightforward and has a direct
precedent already in this tree: `AllowRuntimeCheck`
(`clang/include/clang/Basic/Builtins.td:1235-1239`, attributes
`[NoThrow, Pure, Const]` — note: `observable_checkpoint` must *not* copy
`Pure`/`Const`, since it has a real side effect) →
`SemaChecking.cpp:3580-3588` (argument/arity checking — trivial here, no
arguments) → `CGBuiltin.cpp:3546-3554`
(`Builder.CreateCall(CGM.getIntrinsic(Intrinsic::allow_runtime_check), ...)`).
The same shape applies: a `__builtin_observable_checkpoint()` Builtins.td
entry (no `Pure`/`Const`/`NoThrow` misuse — it should be `NoThrow` since it
can't throw, but not side-effect-free), a no-op Sema arity check, and a
CodeGen lowering that emits a call to the new intrinsic. `<version few
headers>`'s `std::observable_checkpoint()` would then be a one-line
`_LIBCPP_HIDE_FROM_ABI void observable_checkpoint() noexcept { __builtin_observable_checkpoint(); }`,
matching how other builtin-backed library entry points are exposed (e.g.
`std::is_constant_evaluated()` → `__builtin_is_constant_evaluated()`).

## Testing shape (once implemented)

Unlike almost everything else in this fork's `clang/test/Sema*`-heavy
verification discipline, this facility's *entire value proposition* is
invisible to a Sema-only test — a program using it type-checks and runs
identically whether or not the optimizer honors the guarantee. Verification
needs an actual CodeGen/optimizer-level test: a synthetic function with an
observable side effect (e.g. a call to an external, unanalyzable function),
a checkpoint, and then a path that provably hits `unreachable` UB later;
compile at `-O2`/`-O3` and inspect the emitted IR or assembly to confirm the
pre-checkpoint side effect survives (i.e. is not eliminated by the backward
propagation this primitive is meant to block). A Sema-only test can at most
confirm the builtin exists, type-checks, and lowers to *some* call — not
that it does anything.

## Recommendation

Roadmap-only for now. Do not attempt implementation without either (a)
scoping the optimizer-pass audit first as its own dedicated investigation
(which specific passes actually perform backward-UB-propagation in this
LLVM version, and what teaching each one needs), or (b) checking whether
upstream LLVM has since gained an intrinsic or attribute combination closer
to this shape, since this survey was against this fork's current LLVM 22
base and upstream's own intrinsic set does evolve. Correct the issue's
"likely small" framing regardless of when implementation is picked up.
