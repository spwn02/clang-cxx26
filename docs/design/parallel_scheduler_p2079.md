# Design note: `parallel_scheduler` (P2079R10, issue #11's last remaining item)

Status: draft. Staged implementation — see "Staging" below. Written per this
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

- **Pass 1 (this session): the pool, `parallel_scheduler`,
  `get_parallel_scheduler()`, `schedule()`.** A fixed worker set (sized
  from `hardware_concurrency()`, floored at 1) draining one shared
  mutex/condvar-protected intrusive queue — no work stealing, no
  per-thread queues. This is the first genuinely concurrent code in the
  fork; keep the concurrency surface as small as `run_loop`'s own and
  verify it in isolation before building anything on top of it.
- **Pass 2 (this session, after Pass 1 is verified): `bulk_chunked_t`'s
  completion-scheduler probe and chunked dispatch across workers.**
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
