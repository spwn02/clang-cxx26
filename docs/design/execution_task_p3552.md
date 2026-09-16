# Design note: `execution::task<T, Environment>` (P3552R3, issue #12)

Status: draft, not yet implemented. Written per this project's standing policy
that greenfield facilities get a design note before any Codex dispatch.

## What already exists and is reusable as-is

- `libcxx/include/__execution/with_awaitable_senders.h` — complete CRTP
  promise base (`set_continuation`, `continuation()`, `unhandled_stopped()`,
  `await_transform` delegating to `as_awaitable`). `task`'s promise can derive
  from this directly.
- `libcxx/include/__execution/as_awaitable.h` — complete for the non-affinity
  path: `__sender_awaitable`/`as_awaitable_t` correctly implements
  `[exec.as.awaitable]` branches (7.1)/(7.3)/(7.4)/(7.5). Its own code comment
  marks branches (7.2)/(8.1) — routing through the generic
  `get_await_completion_adaptor`/`adapt-for-await-completion` customization
  point — as unimplemented. **This does not block `task` itself**: `task`'s
  own `await_transform(Sender&&)` per the paper does its own explicit
  affinity wrap via `affine_on`, not through that generic customization
  point. See below.
- `libcxx/include/__execution/continues_on.h` — fully real, already-shipped
  sender adaptor with exactly the semantics `affine_on` needs: run the child
  sender to completion, then transition onto a given scheduler before
  forwarding the result. This was the missing piece I'd assumed didn't exist;
  it does. **`affine_on(sndr, sch)` can be implemented as a thin wrapper
  around `execution::continues_on(sndr, sch)`**, deferring only the
  "already completing on `sch`, skip the redundant hop" optimization
  ([exec.affine.on]'s efficiency clause, not a correctness one) — a real,
  documented scope cut, not a degenerate stub.
- `libcxx/include/__stop_token/inplace_stop_source.h` (+ `_token.h`,
  `_callback.h`) — already implemented, already used by
  `libcxx/include/__execution/when_all.h`. Directly usable as `task`'s
  default `stop_source_type`.
- `libcxx/include/__execution/get_allocator.h` (P3433R1, already landed) —
  `get_allocator_t`, `__allocator_aware_forward` — directly usable for the
  promise's allocator-extracting `operator new`/`operator delete`.
- `libcxx/include/__execution/scheduler.h` — the `scheduler` concept and
  `schedule_result_t` already exist, needed to constrain `task_scheduler`'s
  wrapped type and `change_coroutine_scheduler<Sch>`.

Net effect: this facility is **less greenfield than Wave 3b's plan assumed**.
The only genuinely-new pieces are `task` itself, its `promise_type`,
`task_scheduler`, `inline_scheduler`, `with_error<E>`, and
`change_coroutine_scheduler<Sch>`.

## Two scope decisions, made explicit here (per advisor review)

1. **`task_scheduler` is a real type-erasing wrapper, not a stub — and it is
   NOT default-constructible.** Verified against the adopted text directly
   (WebFetch of the 2025 papers-directory HTML, not re-derived from memory):
   its only constructor is
   `template <class Sch, ...> explicit task_scheduler(Sch&&, Allocator = {})`,
   constrained on `scheduler<Sch>` — no default constructor exists. This
   corrects my first draft's assumption that a default-constructed
   `task_scheduler` could hold an `inline_scheduler` implicitly. Instead:
   wherever the promise needs a `scheduler_type` value and `Environment`
   doesn't supply one, it must explicitly construct
   `task_scheduler(inline_scheduler{})` — there's no implicit path to that
   value. `task<T, Environment>::scheduler_type` still names `task_scheduler`
   itself (a public-facing typedef, correct regardless); it's the *value*
   the promise defaults to, not the *type*, that needs an explicit
   construction site. `task_scheduler` itself is: `shared_ptr`-held
   type-erased `scheduler` (any type satisfying `execution::scheduler`),
   `equality_comparable`, `copyable`, `.schedule()` returning a type-erased
   sender — small (a vtable of 2-3 functions) since `scheduler`/`schedule_t`
   already exist to build it on top of.
2. **`error_types` is hardcoded to `completion_signatures<set_error_t(exception_ptr)>`
   for this pass — no support for a custom `Environment::error_types` yet.**
   The paper's `promise_type::uncaught_exception()` must `terminate()` when
   `set_error_t(exception_ptr)` isn't in the supported error set; supporting
   an arbitrary custom set means the promise's result-storage variant shape
   depends on `Environment`, which is a materially bigger type. Fixing it to
   the single default case now, and revisiting only if/when a real caller
   needs a custom `Environment::error_types`, avoids building storage
   machinery with no exerciser. This is a documented scope cut on the
   `Environment` customization surface, not a defect.

## File layout

- `libcxx/include/__execution/task.h` — `task<T, Environment>`, `promise_type`,
  `connect`/`state<R>`. Written directly (not dispatched) — this is one
  tightly-coupled unit and this session's worst bugs (backend-tag mismatch,
  counting-iterator double-dereference) came from exactly this kind of
  cross-piece coupling being split across a dispatch with no working sibling
  file to copy from.
- `libcxx/include/__execution/task_scheduler.h` — `task_scheduler` type
  eraser + `inline_scheduler`.
- `libcxx/include/__execution/affine_on.h` — thin wrapper around
  `continues_on` (see above).
- `with_error<E>` and `change_coroutine_scheduler<Sch>` — small enough to
  live in `task.h` directly rather than their own headers.
- Public header: add `task`/`task_scheduler`/`inline_scheduler` exports to
  `libcxx/include/execution` and `libcxx/modules/std/execution.inc` (per the
  standing "always check module exports" rule).
- Tests under `libcxx/test/std/execution/task/`.

## Dispatch plan (once this note is approved)

Claude writes `task.h`'s core (`task`, `promise_type`, `connect`/`state<R>`)
directly. Codex dispatches, sequentially against the same worktree, for the
peripheral/independent pieces: (1) `task_scheduler`/`inline_scheduler`, (2)
`affine_on`, (3) tests. Each gets a real `ninja -C build-libcxx cxx
cxx-test-depends` rebuild + targeted lit run before merging, same discipline
as every prior wave.

## Primary-source check (done)

Fetched the adopted text directly rather than trusting the earlier
paraphrase. Two things confirmed:

- `task_scheduler` is **not** default-constructible (see above) — corrected
  the design accordingly.
- `affine_on(sndr, sch)`'s wording: `start(op)` "will start `sndr` on the
  current execution agent and execute completion operations on `out_rcvr`
  on an execution agent of the execution resource associated with `sch`. If
  the current execution resource is the same as the execution resource
  associated with `sch`, the completion operation on `out_rcvr` **may** be
  called before `start(op)` completes." The "may" confirms the same-resource
  fast path is a permitted optimization, not required observable behavior —
  a thin wrapper around `continues_on(sndr, sch)` (which always takes the
  scheduling hop) is a fully conforming, if unoptimized, implementation.

No remaining open items before implementation.
