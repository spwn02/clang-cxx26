# Design note: `async_scope` (P3149R11, GitHub issue #11's remaining greenfield item)

Status: Pass 1, Pass 2, and Pass 3 all complete and merged (2026-09-17,
2026-09-18, 2026-09-19) — issue #114 fully closed. Written per this
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
- **Pass 2 (done, 2026-09-18, issue #114): `counting_scope`.** The paper's
  own [exec.stop.when] specifies `token::wrap()` in terms of an
  exposition-only `stop-when(sndr, token)` sender algorithm, *not* the
  mechanism originally guessed above (a single `inplace_stop_callback`
  forwarding the connected receiver's own stop token into the scope's
  source). The real spec: `stop-when` fuses the given `token` with
  whatever `get_stop_token(get_env(r))` the eventual receiver `r` answers,
  building a *combined* token when both are genuinely stoppable (installed
  via `write_env(sndr, prop(get_stop_token, combined))`), or degrading to
  installing just `token` alone when `r`'s own token is `unstoppable_token`
  (and to the identity when `token` itself is `unstoppable_token`).
  Implemented as its own reusable header, `<__execution/stop_when.h>`
  (needed again by `spawn_future` per the paper's own section ordering),
  realizing the paper's abstract "OR of two tokens" `stoken-t` concretely
  as an `inplace_stop_source` that either side's callback can request stop
  on. `counting_scope` itself is exactly the paper's own "as if
  implemented like so" reference: a private `simple_counting_scope` member
  plus an `inplace_stop_source`, with `get_token()`/`close()`/`join()` as
  thin delegates and `request_stop()` a single call into the source —
  composition, not a parallel reimplementation of Pass 1's state machine.
  Verified via `<__execution/stop_when.h>`'s two `if constexpr` branches
  each having direct test coverage (a receiver with no stop token, and one
  with a real `inplace_stop_source` of its own, requesting stop from both
  the scope's side and the external side), a real `import std;` round
  trip (confirming `counting_scope` is exported and `__stop_when` is not —
  this caught a real stale-install trap, see
  [[feedback_build_libcxx_test_suite_install_stale]]: the module partition
  `.inc` files have their *own* separate install target,
  `libcxx-test-suite-install-cxx-modules`, distinct from
  `libcxx-test-suite-install-cxx-headers`), a standalone ThreadSanitizer
  run, and the full `libcxx/test/std/execution` suite (54/54).
- **Pass 3 (done, 2026-09-19, issue #114): `spawn_future`.** Implemented
  exactly per [exec.spawn.future]'s exposition-only reference: a
  `spawn-future-state` owning the eagerly-connected-and-started child
  operation (composed through `token.wrap(sndr)` **and** a second,
  independent `stop-when` layer using the state's own private
  `inplace_stop_source` -- abandoning *this* future requests stop without
  affecting any other operation associated with the scope), a
  `spawn-future-receiver` that stores the result into a `variant` and
  signals completion, and three operations --
  `complete()`/`consume()`/`abandon()` -- that the spec requires to
  "behave as atomic operations" appearing "to occur in a single total
  order." Implemented with a plain `mutex` (matching `simple_counting_scope`'s
  own Pass 1 idiom for a structurally similar race), not lock-free atomics.

  **Two real bugs found only by testing every ordering the race actually
  has, not just the ones synchronous test senders exercise by default:**
  1. `complete()`'s "the receiver was already registered via `consume()`"
     branch dispatched the result but never transitioned `__phase_` away
     from `__consumed`. Once the *outer* opstate (and the `unique_ptr` it
     owns) was later destroyed, `abandon()` -- called from the
     `unique_ptr`'s own deleter, never a direct delete -- saw a phase its
     `switch` had no case for, silently did nothing, and the state (and
     its scope association) leaked forever, `std::terminate()`-ing the
     *next* time that scope's own destructor ran. Fixed by transitioning
     to `__completed` (the same "nothing left to wait for, just tear
     down" terminal state `abandon()` already knows how to handle) before
     dispatching.
  2. The original "defer the destroy decision to `abandon()`'s own stack"
     fix for the advisor-flagged synchronous-`request_stop()` reentrancy
     risk only handles `complete()` firing *synchronously inside*
     `request_stop()`'s own call. It does not handle the much more common
     case in this fork -- `complete()` firing *later*, fully
     asynchronously (a `run_loop`-scheduled operation, draining only once
     something eventually calls `run()`) -- where `abandon()`'s own stack
     frame is long gone by the time `complete()` runs, so nothing was ever
     going to re-check anything. Fixed with an explicit
     `__request_stop_in_progress_` flag: `complete()`'s `__abandoned`
     branch destroys directly, itself, whenever that flag is false (the
     ordinary, asynchronous case, where nothing depends on
     `request_stop()`'s stack frame surviving); it only defers when the
     flag is true (genuinely nested inside `request_stop()`'s own
     still-unwinding call).

  Both were caught by a standalone scratch reproduction *before* they
  reached the committed test suite -- built specifically because the
  design note's own testability section already flagged that this fork's
  senders complete synchronously by default, so a test built around that
  assumption would never exercise the deferred-completion code at all.
  The lesson generalizes past this one facility: **for any state machine
  whose correctness depends on *which* of several async completion paths
  happens first, a passing synchronous-only test proves nothing about the
  paths that only fire when something is genuinely still outstanding --
  build the `run_loop`-deferred version of every such test before trusting
  any of them.**

  Also discovered while composing tests: `run_loop`'s own `schedule()`
  opstate checks its stop token *before* dispatching
  ([exec.run.loop.types]p10.2) and completes with `set_stopped()`
  directly if already requested -- correct, standard-mandated behavior,
  but it means nothing chained after `schedule(loop.get_scheduler())` via
  `then`/`let_value` ever runs once abandonment has requested stop first.
  A test built to observe a stop token mid-chain this way will silently
  never reach its own observer; the actually-meaningful observable proof
  for "did abandonment's `request_stop()` reach a still-outstanding
  operation" is that the scope's own association count returns to zero
  cleanly afterward (which is exactly what bug 1 above broke).

  Verified via the full paper wording extracted directly from the primary
  source (`curl` + manual HTML-strip, after `WebFetch` truncated before
  reaching [exec.spawn.future]'s own section -- see
  [[project_p3149_async_scope]] for why), tests covering every
  `complete`/`consume`/`abandon` ordering (synchronous value/error/stopped
  completion, closed-scope association failure, abandonment before and
  after completion, consume-then-complete via `run_loop`), a standalone
  ThreadSanitizer run (4 reps, clean), a real `import std;` round trip,
  and the full `libcxx/test/std/execution` suite (54/54).

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
