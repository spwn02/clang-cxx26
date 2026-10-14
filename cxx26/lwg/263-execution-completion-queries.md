# LWG issue drafts (tracker spwn02/clang-cxx26#263) - five separate issues

All five were found while implementing the completion queries (`get_completion_scheduler`, `get_completion_domain`, `inline-attrs`) in the clang-cxx26 fork. Each is meant to be submitted as its own LWG issue. **Wording is relative to:** N5050 (draft of 2026-06-01, commit 8ab1a9b of cplusplus/draft), `exec.tex`. Status: New (to be submitted). None of them changes behavior of the fork: the implementation follows the most specific text and documents it.

---

## 263-A `basic-sender::get_env` uses `impls-for<Tag>::get-attrs`, which `default-impls` does not declare

**Section:** [exec.snd.expos] (`default-impls`, `basic-sender`), [exec.just], [exec.read.env]   **Priority:** 2

### Discussion

The `basic-sender` class template in [exec.snd.expos] is specified as

```cpp
decltype(auto) get_env() const noexcept {
  auto& [_, data, ...child] = *this;
  return impls-for<Tag>::get-attrs(data, child...);
}
```

but the members of `default-impls` are `get-env`, `get-state`, `start`, `complete` and the function template `check-types`; there is no `get-attrs`, and the name occurs nowhere else in the standard. No specialization of `impls-for` in the draft defines it either, so `basic-sender::get_env()` is ill-formed for every sender made with `make-sender`: `just`, `just_error`, `just_stopped`, `read_env`, `then`, `let_value`, `bulk`, `when_all`, ... .

The intended attributes are described only indirectly: [exec.adapt.general] says "A parent sender with a single child sender `sndr` has an associated attribute object equal to `FWD-ENV(get_env(sndr))` except that the `get_completion_scheduler<completion-tag>` and `get_completion_domain<completion-tag>` queries are handled as described in [exec.snd.general]", and "a parent sender with more than one child sender has an associated attributes object equal to `env<>{}` except ...". The sender factories ([exec.just], [exec.read.env]) have no attributes specification at all (they have no child; by the second sentence's logic they have no attributes, but that is not stated).

### Proposed resolution

In [exec.snd.expos], add to `default-impls`

```cpp
static constexpr auto get-attrs = see below;           // exposition only
```

and add a paragraph after the one for `default-impls::get-env`:

> The member `default-impls::get-attrs` is initialized with a callable object equivalent to the following lambda:
> ```cpp
> [](const auto&, const auto&... child) noexcept -> decltype(auto) {
>   if constexpr (sizeof...(child) == 1)
>     return (FWD-ENV(get_env(child)), ...);
>   else
>     return env<>();
> }
> ```

This reproduces [exec.adapt.general]: the completion-scheduler/domain queries are layered on top as described in [exec.snd.general] by the senders that provide them. (If the editors prefer to keep the behavior in [exec.adapt.general], the declaration of `get-attrs` in `default-impls` and the lambda above are still needed because `basic-sender::get_env` refers to the member.)

---

## 263-B The `get_completion_scheduler` requirement of [exec.sched] cannot hold for `run_loop`'s scheduler with tag `set_error_t`

**Section:** [exec.sched] (the paragraph on `get_completion_scheduler<T>(sch, envs...)`), [exec.get.compl.sched], [exec.run.loop.types]   **Priority:** 3

### Discussion

[exec.sched] requires, for a scheduler expression `sch`, type `T` and pack `envs`, that `get_completion_scheduler<T>(sch, envs...)` and `get_completion_scheduler<T>(get_env(schedule(sch)), envs...)` "are either both ill-formed, or both well-formed with the same type and value".

[exec.get.compl.sched] makes the first expression well-formed for every `T` in `{set_value_t, set_error_t, set_stopped_t}` as soon as `envs` is not empty and the query on `q` is otherwise ill-formed: "Otherwise, `auto(q)` if the type of `q` satisfies `scheduler` and `envs` is not an empty pack". For the scheduler of `run_loop` and `T = set_error_t`, the first expression is therefore `sch` (well-formed), while [exec.run.loop.types] only provides the completion scheduler for tags "other than `set_error_t`" for `get_env(schedule(sch))` (the run-loop sender never completes with `set_error`), so the second expression is ill-formed. The requirement of [exec.sched] is violated by the draft's own scheduler.

### Proposed resolution (one possible)

In [exec.sched], restrict the requirement to the tags the schedule sender can complete with:

> ... the following expressions are either both ill-formed, or both well-formed with the same type and value, for every `T` such that `completion_signatures_of_t<schedule_result_t<decltype((sch))>, decltype((envs))...>` has at least one completion signature whose completion tag is `T`:

(and the analogous change for `get_completion_domain`). Alternative: make bullet 2 of [exec.get.compl.sched] apply only when the schedule sender of `q` can complete with the tag.

---

## 263-C `inline_scheduler` violates [exec.sched] for environments without a scheduler

**Section:** [exec.snd.expos] (`inline-attrs`), [exec.sched], [exec.get.compl.sched]   **Priority:** 3

### Discussion

`inline-attrs<Tag>{}.query(get_completion_scheduler<Tag>, env)` is specified as `get_scheduler(env)`, which is ill-formed for an `env` without a scheduler. For `inline_scheduler sch`:

* `get_completion_scheduler<set_value_t>(get_env(schedule(sch)), env)`: bullet 1 of [exec.get.compl.sched] queries the sender's attributes, i.e. `get_scheduler(env)`, ill-formed without a scheduler in `env`; bullet 2 does not apply (the attributes are not a scheduler); the expression is ill-formed.
* `get_completion_scheduler<set_value_t>(sch, env)`: bullet 1 (query on a scheduler object) is ill-formed, bullet 2 applies (`sch` is a scheduler, `envs` not empty): well-formed, yielding `sch` itself.

[exec.sched] requires both to be ill-formed or both well-formed with the same value. The design intent of `inline_scheduler` is that it completes on the scheduler of the receiver's environment, not on itself, so bullet 2 produces the wrong answer.

### Proposed resolution (options)

(i) Specify `inline_scheduler` as an exception to the equivalence in [exec.sched]; or (ii) give `inline_scheduler` a `query(get_completion_scheduler<Tag>, env)` member with the same constraint as `inline-attrs` so that bullet 1 handles it, and let the scheduler-level query be ill-formed exactly when the sender-level one is.

---

## 263-D RECURSE-QUERY is not expressible as "expression-equivalent to"

**Section:** [exec.get.compl.sched]   **Priority:** 3

### Discussion

[exec.get.compl.sched] defines `RECURSE-QUERY(sch1, envs...)` as "expression-equivalent to `sch1` if `sch2` is ill-formed or if `sch1` and `sch2` have the same type and compare equal; otherwise, `RECURSE-QUERY(sch2, envs...)`", where `sch2` is `TRY-QUERY(sch1, get_completion_scheduler<set_value_t>, envs...)`. "Compare equal" is a run-time property of values; an expression-equivalence relation (and therefore the *type* of the expression) cannot depend on it. If `sch1` and `sch2` have the same type but are unequal, the result is the type of `RECURSE-QUERY(sch2, envs...)`, which may be a different scheduler type (`sch3`); if they are equal, the result has the type of `sch1`.

### Proposed resolution

Define `RECURSE-QUERY` operationally (it *returns* a value) and constrain the types:

> `RECURSE-QUERY(sch1, envs...)` returns `sch1` if `sch2` is ill-formed or if `sch1` and `sch2` have the same type and compare equal; otherwise it returns `RECURSE-QUERY(sch2, envs...)`. *Mandates:* if `sch1` and `sch2` have the same type, `RECURSE-QUERY(sch2, envs...)` also has that type.

(the fork's implementation requires exactly this: the chain must stay in one type once two consecutive schedulers have the same type.)

---

## 263-E The ill-formedness rule for completion queries is stated on a sender, not on the query

**Section:** [exec.get.compl.sched] p7 (the paragraph beginning "For a type `Tag`, subexpression `sndr`, and pack `envs`"), [exec.get.compl.domain]   **Priority:** 4

### Discussion

The paragraph says: "If both `get_completion_scheduler<Tag>(get_env(sndr), envs...)` and `CS` are well-formed and `CS().count-of(Tag()) == 0` is `true`, the program is ill-formed" (with `CS = completion_signatures_of_t<decay_t<decltype((sndr))>, decltype((envs))...>`); [exec.get.compl.domain] has the same rule. The query object receives `get_env(sndr)` and `envs...`, not `sndr`, so it cannot check the condition; the rule can only be enforced by whoever combines a sender with its attributes. In effect it is a requirement on authors of senders, but it is written as if it were a property of the query (the program is ill-formed whenever the pair exists, even if the query is never called, and no diagnostic is required or even possible in general).

### Proposed resolution

State it as a requirement on sender types, for example as an *Expects*/*Mandates* of `basic-sender::get_env` and as a note for user-defined senders: "For a sender type `Sndr` ..., if `get_completion_scheduler<Tag>(get_env(sndr), envs...)` is well-formed then `completion_signatures_of_t<Sndr, decltype((envs))...>` shall contain a completion signature with tag `Tag`."
