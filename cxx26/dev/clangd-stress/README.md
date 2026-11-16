# clangd macro completion stress matrix

`lsp_probe.py` exercises every character boundary from each macro name through
its invocation semicolon, every character boundary in each `^^{ ... }` quote
body, and advisor control cases for dependent explicit-object hints, lambda
trailing-return completion, a raw macro argument's first token, a
mem-initializer macro, and macro-supplied completion in a DSL token sequence.
The 15 macro cases cover expression, statement,
declaration, typed and raw parameters, nesting, deferred templates, explicit
object members, injected calls, mem-initializers, and quote navigation.

Each case/repeat starts a fresh clangd process and sends
`textDocument/completion` with `triggerKind: Invoked` at every collected
position. At each quote-body position it also sends hover, definition,
references, and rename (`newName: stress_renamed`); rename edits are discarded.
The default is three repeats. Death, timeout, malformed JSON-RPC, unexpected
protocol errors, missing control completions, and the missing dependent-call
hint are failures. JSON-RPC errors with ignored codes remain ignored, as do
clangd's expected `Cannot rename symbol` refusals. Per-run and total summaries
report `completion_requests` and `navigation_requests` separately.

The `cc1` mode runs every collected position 25 times in fresh compiler
processes by default, grouped by invocation, quote-body, and control class. It
uses `-cc1 -std=c++29 -I clang/test/Reflection/Inputs -fsyntax-only
-code-completion-at=...` and reports assertion/crash output and the control
case's expected completion behavior.

Run from the repository root:

```sh
python3 cxx26/dev/clangd-stress/lsp_probe.py /path/to/clangd \
  --clang /path/to/clang --mode all
```

Run only selected LSP or cc1 cases or change repeat counts:

```sh
python3 cxx26/dev/clangd-stress/lsp_probe.py /path/to/clangd \
  --mode lsp --case nested --case control-lambda-trailing --repeats 3
python3 cxx26/dev/clangd-stress/lsp_probe.py /path/to/clangd \
  --clang /path/to/clang --mode cc1 --cc1-repeats 25
python3 cxx26/dev/clangd-stress/lsp_probe.py /path/to/clangd \
  --clang /path/to/clang --mode cc1 --case control-macro-supplied-completion \
  --case control-raw-first-token --case control-meminit --cc1-repeats 40
```

The compiler executable is passed separately so the same corpus can compare
different clangd/clang builds. Set `TMPDIR` to a writable location for the
temporary source workspaces when the system default is unavailable.
