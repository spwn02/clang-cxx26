# Wave 5 exception-handle implementation report

## Scope and implementation

Issue #148 requires constexpr `exception_ptr` operations with stable exception
identity while preserving the exported runtime representation. The evaluator now
tracks dynamically allocated exception objects and retained handle counts through
capture, copy/move, release, nested catches, and rethrow. Catch matching uses the
existing handler logic, including derived-to-base adjustment. Non-trivial
by-value handler copies invoke the copy constructor; exception destruction is
tracked by identity. Escaping handles and references are rejected.

Clang provides evaluator-only capture, retain, release, and rethrow hooks. libc++
uses separate constant-evaluation and runtime paths, keeps runtime layout and
exported entry points, and updates ABI symbol lists for out-of-line helper
functions. `exception_ptr_cast` retains the current-draft
`optional<const E&>` return type.

The exception feature macro is generated only when all four compiler hooks are
available. Its test coverage includes identity, copy/move/reset, nested
handlers, rethrow, base matching, non-trivial by-value copies, destructor counts,
and rejected escapes.

## Validation

The focused Clang constexpr exception-handle Sema test and libc++
`exception_ptr_cast` test pass. Coverage exercises identity, copy/move/reset,
nested catches, rethrow, derived-to-base matching, destructor counts, and
rejected escapes. Following the final AST change, libc++ was explicitly clean
rebuilt before its full suite. The full Clang/libc++ archives show no new
feature failures against baseline; only sandbox-denied lit sockets, GDB ptrace,
and filesystem socket binds differ from baseline. #148 remains open pending
final review and publication. #116 remains deferred.
