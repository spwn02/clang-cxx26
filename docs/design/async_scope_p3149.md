# Design note: `async_scope` (P3149R11, GitHub issue #11's remaining greenfield item)

Status: draft. Staged implementation — see "Staging" below. Written per this
project's standing policy that greenfield facilities get a design note
before any Codex dispatch, following the same process used for
`execution::task` (docs/design/execution_task_p3552.md).

## What the paper actually specifies (verified via WebFetch of the adopted
text, not re-derived from memory)

- **`scope_token` concept**: `copyable`, plus `try_associate() -> bool`,
  `disassociate() noexcept -> void`, `wrap(sender) -> sender`. Tokens are
  non-owning handles; `disassociate()` must be called exactly once per
  successful `try_associate()`.
- **`simple_counting_scope`**: an atomic-counter-based scope with a state
  machine (unused → open → closed; open-and-joining; closed-and-joining;
  joined). `get_token()`, `close()` (rejects future associations),
  `join()` (a sender completing when the outstanding count reaches zero —
  synchronously if already zero, asynchronously otherwise). Destructor
  `terminate()`s unless in unused, unused-and-closed, or joined state.
  `token::wrap()` is the identity here (returns the sender unchanged).
- **`counting_scope`**: `simple_counting_scope` plus `request_stop()` and a
  real `token::wrap()` that composes the input sender with a "stop-when"
  mechanism — forwards a stop request from either the connected receiver's
  own stop token or the scope's internal `inplace_stop_source` into the
  wrapped sender.
- **`associate(sender, token)`**: wraps + `try_associate()`s; on failure
  completes with `set_stopped()` instead of running the input sender; the
  association is released (via `disassociate()`) from the resulting
  operation state's destructor. Basis operation, not built from `spawn`.
- **`spawn(sender, token, env = {})`**: fire-and-forget-but-observable —
  wraps, `try_associate()`s, connects with a dynamically-allocated,
  self-owning operation state (allocator from `env`/the sender's own
  environment/`allocator<void>`), starts it. On completion: destroy state,
  deallocate, *then* `disassociate()` — that exact order, because the
  allocator must outlive its own last use. Input sender must never
  complete with an error (Mandates), matching "no detached work, but also
  no silently-swallowed failures."
- **`spawn_future(sender, token, env = {})`**: like `spawn` but returns a
  sender exposing the child's actual result (including errors); supports
  abandonment (destroying the returned sender before it's started sends a
  stop request to the still-running child) and race-with-completion
  (starting the returned sender then losing a stop race). Materially
  harder than everything else in the paper — its shared state must survive
  being torn down from either side.
- **Dependencies**: only P2300R10 machinery (senders, `connect`/`start`,
  `get_stop_token`, `get_allocator`, `write_env`-style env composition) —
  no dependency on a real scheduler, so this does NOT block on P2079R10 the
  way it might have seemed to at first glance.

## Reusable substrate already in this fork

- `libcxx/include/__execution/get_allocator.h` (P3433R1, already landed) —
  `get_allocator_t`, usable directly for `spawn`'s allocator selection.
- `libcxx/include/__stop_token/inplace_stop_source.h` etc. — usable for
  `counting_scope`'s internal stop source and for building the `stop_when`
  composition `token::wrap()` needs (via `inplace_stop_callback`).
- `libcxx/include/__execution/write_env.h` — the env-composition pattern
  `wrap()`'s "stop-when" sender needs to expose a combined stop token.
- **The `task_scheduler` type-erasure pattern from #12** (abstract
  concept/model + `unique_ptr`-owned opstate, bridging an arbitrary
  connected sender/receiver pair behind a fixed interface) is the *exact*
  shape `spawn`'s self-owning, dynamically-allocated operation state needs
  — same problem (erase an operation state's concrete type, own it
  independently of any caller-held handle), same solution.
- `libcxx/include/__execution/run_loop.h` — **this is the key piece for
  testing**, not for implementation. See below.

## The testability problem this facility has that `task` didn't (per
advisor review)

Every sender currently buildable in this fork's tests completes
synchronously, inline, inside `start()` (there is no real async scheduler
until P2079R10). For `execution::task`, that was fine — inline completion
still exercises every real code path. For `async_scope`, it is NOT fine:
the entire point of the counter/state-machine is coordinating genuinely
*outstanding* work. If every spawned sender is already complete by the time
`join()` is called, the count is always already zero, `join()` always takes
its synchronous fast path, and the actual deferred-completion code (the
opstate that stores a receiver and completes it later, from whichever
`disassociate()` call happens to bring the count to zero) is written but
never executed by any test — tests passing would prove nothing about it.

**Fix: use `run_loop` as the test harness's source of genuinely-deferred
completion**, not as anything `async_scope` itself depends on.
`__run_loop_opstate::start()` only enqueues (`loop->__push_back(this)`); the
receiver only completes once something calls `loop.run()`. So:
`spawn(schedule(loop.get_scheduler()), token)` leaves the count at 1 with
nothing completed; `join()` genuinely has to wait; asserting it hasn't
completed yet, then calling `loop.run()`, then asserting `join()` completes,
exercises the real deferred path deterministically, single-threaded, with no
new scheduler infrastructure needed. Tests must be built around this from
the start, not retrofitted — it affects the design (the join opstate must
genuinely store its receiver and complete it later, not poll).

## Staging (per advisor review — do not build all three passes at once)

- **Pass 1 (this session): `simple_counting_scope`, `scope_token` concept,
  `associate`, `spawn`, `join`.** Fully self-contained and testable via
  `run_loop` as described above. This is what gets implemented and
  committed now.
- **Pass 2 (explicit follow-up, not this session): `counting_scope`** =
  Pass 1 plus the real `token::wrap()` "stop-when" composition (an
  `inplace_stop_callback` on the connected receiver's stop token,
  forwarding into the scope's own `inplace_stop_source`) and
  `request_stop()`. Incremental on top of Pass 1.
- **Pass 3 (explicit follow-up, not this session): `spawn_future`.** Its
  shared state must survive being completed from one side while abandoned
  or stopped from the other — materially harder than Pass 1 and 2
  combined, and the piece least verifiable without genuine concurrency.
  Design it here, land it separately, the same way `execution::task`
  deferred a custom `Environment::error_types` rather than build unexercised
  machinery for it.

## Implementation notes for Pass 1

- `spawn` implements its own `wrap → try_associate → connect/start`
  directly — it is a basis operation, not `associate` followed by a
  separate start. Teardown order on completion: destroy the connected
  child operation state, deallocate its storage, *then* call
  `disassociate()` — in that exact order, since the allocator used to free
  the storage must itself still be valid at the point of deallocation, and
  nothing guarantees the scope (and whatever holds the allocator) outlives
  an out-of-order `disassociate()` that let a racing `close()`/destructor
  reason the scope was safe to tear down.
- `join()`'s sender: on `connect`, an opstate stores the receiver; on
  `start()`, if the scope's counter is already zero, completes immediately
  (matching "synchronously ... if conditions are met"); otherwise registers
  itself as the pending joiner, and whichever `disassociate()` call brings
  the count to zero completes it.
- File layout: `libcxx/include/__execution/async_scope.h` (or split
  `scope_token.h` + `async_scope.h` if the concept + `simple_counting_scope`
  + `associate`/`spawn`/`join` end up sizable enough to warrant it — decide
  once Pass 1 is actually written).

## Open item to verify before implementation starts

`spawn`'s exact allocator-selection order ("selected from env, the sender's
own environment, or `allocator<void>`") — confirm the precise fallback
chain against the paper's Effects clause, not the WebFetch paraphrase,
before writing the allocator-extraction code (mirrors the `task_scheduler`
default-constructibility lesson from #12: verify exact mechanics against
adopted text, not a summarized extraction, before locking in behavior).
