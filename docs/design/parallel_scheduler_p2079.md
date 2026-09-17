# Design note: `parallel_scheduler` (P2079R10, issue #11's last remaining item)

Status: Pass 1 and Pass 2 complete and merged (2026-09-17). Pass 3
(`system_context_replaceability`) remains deferred — see "Staging" below.
Written per this
project's standing policy that greenfield facilities get a design note
before any Codex dispatch, following the process used for `execution::task`
(#12) and `async_scope` (#11's `P3149R11` sub-item).

## What the paper actually specifies (verified via WebFetch of the adopted
text)

- **`parallel_scheduler`**: the sole user-facing scheduler. Not
  default-constructible; obtained only via a free function
  `get_parallel_scheduler()`. Copyable, movable, equality-comparable
  (`==` tests *backend identity* — multiple calls to
  `get_parallel_scheduler()` typically return handles to the same
  underlying process-wide pool). `query(get_forward_progress_guarantee_t)`
  returns `parallel`. `schedule()` returns an exposition-only sender whose
  completion signatures are `set_value_t()`, `set_stopped_t()`,
  `set_error_t(exception_ptr)`, and whose environment answers
  `get_completion_scheduler<set_value_t>()` with the scheduler itself.
- **`bulk()` customization**: the scheduler is supposed to intercept
  `execution::bulk()` on senders whose completion scheduler is a
  `parallel_scheduler`, dispatching each index in `[0, shape)` as a
  genuinely parallel agent via the backend's `schedule_bulk_chunked`/
  `schedule_bulk_unchunked`.
- **Backend replaceability**: `std::execution::system_context_replaceability`
  defines an ABI (`parallel_scheduler_backend`, `receiver_proxy`,
  `bulk_item_receiver_proxy`) and a weak, link-time-replaceable
  `query_parallel_scheduler_backend()` entry point, so a program can swap
  in its own thread pool implementation at link time.
- **Explicitly implementation-defined / deferred by the paper itself**:
  thread count (no configuration API), OS-scheduler integration ("may"),
  priorities (deferred to a future paper), main-thread work donation
  ("drive-ability", a separate proposal), freestanding support (deferred).

## What's already in this fork

Nothing — confirmed again via grep, same as when `async_scope`'s design
note checked: no thread-pool/task-queue primitive exists anywhere except
`<__execution/run_loop.h>`'s explicitly single-threaded, hand-driven queue
(`__pstl/backends/std_thread.h` is self-documented "for testing purposes
only" and is not a general-purpose scheduler). This is the first genuinely
concurrent facility this fork has needed to build from scratch — everything
built this session (`execution::task`, `async_scope`) was deliberately
scoped to stay correct under purely inline/single-threaded completion.

**`run_loop.h` is still the right structural template**, just generalized
from one draining thread to N: its intrusive-linked-list-under-a-mutex,
`condition_variable`-gated queue (`__push_back`/`__pop_front`) is exactly
the shape a fixed worker pool needs, just with multiple threads calling the
pop-and-execute loop instead of one. `__run_loop_sndr_env`'s pattern for
answering `get_completion_scheduler<set_value_t>` is reused directly for
`parallel_scheduler`'s own schedule-sender environment.

## A real architectural finding: `bulk()` cannot be customized the way the
paper's own wording describes on this fork

`libcxx/include/__execution/bulk.h`'s own top-of-file comment documents
that `bulk_t`'s "expression-equivalent to `bulk_chunked(sndr, policy,
shape, new_f)`" wording is implemented by directly returning that
concrete sender type, **not** through `[exec.bulk]p4`'s actual specified
mechanism (domain-based `transform_sender` customization, dispatched via
`tag_of_t<Sndr>().transform_sender(...)`) — because that dispatch path is
the exact one `<__execution/domain.h>`'s own documented M2 deviation
permanently disables on this fork (a `tag_of_t` structured-binding probe
hard-errors instead of SFINAE-ing away, a confirmed Clang limitation, not
a design choice). This is the *same* underlying limitation already
recorded for `default_domain::transform_sender`'s "if well-formed" branch.

Per-scheduler `bulk()` customization is specified through exactly this
domain/transform_sender mechanism, so it inherits the same limitation:
**there is no way to make `parallel_scheduler` customize `bulk()` through
the standard's own specified path on this fork.** Reworking
`domain.h`/`transform_sender` to route around the compiler limitation is
a much larger undertaking than this facility — out of scope here, not
attempted.

**Pragmatic, documented deviation (confirmed via advisor review): probe
the completion scheduler directly inside `bulk_chunked_t::operator()`.**
Since `parallel_scheduler`'s schedule-sender environment already answers
`get_completion_scheduler<set_value_t>` (see above), `bulk_chunked_t` can
use the same `__try_query`-style probe `<__execution/get_scheduler.h>`
already has precedent for, check whether the input sender's completion
scheduler is a `parallel_scheduler`, and dispatch through the pool
directly when it is — falling back to the existing single-chunk behavior
otherwise. This produces the paper's *observable* behavior (bulk actually
runs in parallel when chained after `parallel_scheduler.schedule()`)
without needing the exposition-only domain machinery at all. This is a
real, permanent deviation from the specified mechanism, not a TODO to
later fix via `domain.h` — recorded here the same way `bulk.h`'s own
existing deviations are recorded.

## Staging (per advisor review)

- **Pass 1 (done, 2026-09-17): the pool, `parallel_scheduler`,
  `get_parallel_scheduler()`, `schedule()`.** A fixed worker set (sized
  from `hardware_concurrency()`, floored at 1) draining one shared
  mutex/condvar-protected intrusive queue — no work stealing, no
  per-thread queues. Verified via a real rebuild, a dedicated lit test,
  50 reps of a concurrent stress test through `async_scope`'s
  `spawn`/`join`, and a standalone ThreadSanitizer run — which caught a
  genuine pre-existing race in `<__execution/run_loop.h>` (see "Bugs
  found" below), fixed as part of this pass.
- **Pass 2 (done, 2026-09-17): `bulk_chunked_t`'s completion-scheduler
  probe and chunked dispatch across workers.** Implemented in
  `<__execution/bulk.h>`: `__bulk_sndr::connect()` probes the child
  sender's completion scheduler via the same `__try_query` primitive
  `get_completion_scheduler_t` itself uses internally (calling the full
  CPO directly would hard-fail via a `static_assert` instead of
  SFINAE-ing away when the query isn't answered — the same "immediate
  context" pitfall recorded elsewhere in this fork's M1/M2 deviations).
  When the probe finds `parallel_scheduler`, `[0, shape)` is split into
  up to 32 contiguous chunks (capped by `hardware_concurrency()`), each
  dispatched via its own `schedule(sch)` and completed asynchronously —
  **not** via a blocking wait, which would consume a pool worker and
  deadlock the moment concurrent bulk operations reach or exceed the
  worker count (this fork's Pass-1 pool has no work-stealing/helping to
  rescue a blocked worker). Instead each chunk decrements a shared
  atomic counter (`acq_rel`, not `relaxed`, so the completing chunk
  observes every other chunk's writes); the chunk that reaches zero
  completes the outer receiver. `bulk_t` (which composes over
  `bulk_chunked_t`) becomes parallel for free. `bulk_unchunked_t` is
  deliberately **not** customized this pass — always the existing
  single-threaded fallback, regardless of scheduler.

  **A real scope limit, found empirically, not assumed:** the probe
  only ever succeeds for a *direct* `schedule(get_parallel_scheduler())
  | bulk_chunked(...)` chain. `FWD-ENV` (`<__execution/fwd_env.h>`),
  the same shared utility every adaptor in this fork's `get_env()` uses
  (including `bulk`'s own and `then`'s), does not forward
  `get_completion_scheduler` through — confirmed by actually testing
  `schedule(sch) | then(f) | bulk_chunked(...)`, which silently and
  correctly falls back to the sequential path rather than customizing.
  This matches every other adaptor's existing behavior in this fork; it
  is a narrower reach than "wherever a parallel_scheduler's completion
  is reachable," and is recorded here rather than left to be
  rediscovered as a surprise later.

  Verified via a dedicated lit test (997-shape chunk-boundary
  correctness, `bulk()`-composes-parallel-too, TRY-EVAL exception
  propagation via `set_error` rather than `std::terminate` — unlike the
  *parallel range algorithms'* different exception rule — and the
  non-`parallel_scheduler` regression case), a real multi-thread-id
  observation confirming genuine parallelism (not just a
  sequential-fallback that happens to be correct), a standalone
  ThreadSanitizer run (4 repeats, clean), and — the specific check this
  design's self-deadlock risk demanded — a manual, temporary
  forced-single-worker-pool run of the whole suite, which completed
  without hanging (the only failure was the trivially-expected
  "observed more than one thread id" assertion, since a 1-worker pool
  can only ever produce one).

- **Pass 3 (explicit follow-up, not this session): the
  `system_context_replaceability` ABI** (`parallel_scheduler_backend`,
  `receiver_proxy`, `bulk_item_receiver_proxy`, the weak-symbol
  `query_parallel_scheduler_backend()`). The paper mandates link-time
  replaceability, but nothing in this fork provides a second
  implementation to replace it with, and a weak-symbol ABI contract is
  only meaningful with a real second implementation to validate it
  against. Deferred explicitly, the same way `execution::task` deferred
  custom `Environment::error_types` and `async_scope` deferred
  `spawn_future`.

## Bugs found during this work (not present before Pass 1/2, or latent
and newly exposed)

- **`run_loop.h`'s `finish()`/`__push_back()` notified their condition
  variable *after* releasing the mutex.** A waiter could wake, return
  from `run()`, and let its caller destroy the `run_loop` (including
  the condition variable) while the notifying thread's `notify_one()`
  call was still in flight — a genuine use-after-free race, caught by
  ThreadSanitizer on the very first cross-thread exercise of
  `run_loop` this fork has ever had (`this_thread::sync_wait` uses
  `run_loop` internally, and `parallel_scheduler`'s worker threads are
  the first real second thread to ever call `finish()` on one). Fixed
  by notifying while still holding the lock in both functions. Every
  prior `run_loop` user drove it single-threadedly, so this was latent,
  not a regression.
- **`__parallel_opstate`'s defaulted move constructor triggered
  `-Wdeprecated-copy-with-dtor` under `-Werror`.** Moving the
  `__parallel_task_base` base subobject fell back to its (deprecated)
  implicit copy constructor, since a user-declared destructor suppresses
  the implicit move constructor. Fixed by declaring one explicitly on
  the base.
- **`receiver_of`'s check required `__bulk_chunk_rcvr` to declare
  `set_stopped()`** even though its environment never answers
  `get_stop_token` (expected, by this file's own reasoning applied
  elsewhere, to narrow the sender's completion signatures down to just
  `set_value_t()`) — a real build demanded it regardless. Added as a
  safe no-op (decrements the same completion counter, invokes nothing);
  genuinely unreachable in practice, since no stop token is ever
  propagated into that receiver's environment.

## Two specifics that would otherwise cause real bugs

- **Lifetime/destruction.** The paper allows the scheduler to outlive
  `main()` and suggests a Phoenix-singleton pattern. Joining worker
  threads from a static destructor is a classic deadlock/UB source (join
  ordering against other static destructors, including the C++ runtime's
  own, is unspecified). **The pool is heap-allocated once, on first call
  to `get_parallel_scheduler()`, and deliberately never destroyed** —
  worker threads are detached and run an infinite loop for the process's
  entire lifetime. `parallel_scheduler` holds a raw, non-owning pointer to
  this leaked singleton (no `shared_ptr`, no refcounting needed — the
  pointee never goes away). This is a real, deliberate scope decision
  (not an oversight): no shutdown/join support exists in Pass 1.
- **Handle semantics.** `parallel_scheduler() = delete` plus copyable
  means this is a handle, not an owner. `operator==` compares the
  underlying pool pointer. It must satisfy `scheduler` (copyable,
  equality_comparable, `schedule()` returns a sender, answers
  `get_forward_progress_guarantee`) exactly the way `task_scheduler`
  already does, so `task_scheduler(get_parallel_scheduler())` works
  out of the box once Pass 1 lands.

## Verification plan (matches the concurrency-specific caution this
facility needs beyond what `task`/`async_scope` required)

Beyond a real `ninja -C build-libcxx` rebuild + `libcxx-lit`: a standalone
stress test exercising genuine concurrency, not just single-shot
correctness — many concurrent `schedule()`s incrementing a shared atomic,
joined via `async_scope`'s `spawn`+`join` (built last pass, and this is
its first real exercise against genuinely-outstanding, genuinely-parallel
work rather than `run_loop`'s single-threaded stand-in), run repeatedly to
surface any race rather than trusting one clean pass. Compile the
standalone stress test with `-fsanitize=thread` if the toolchain supports
it, given this is the fork's first real multi-threaded code and a data
race here would be silent until it wasn't.
