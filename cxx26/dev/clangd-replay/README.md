# clangd replay in the real editor stack

`replay.sh` drives Neovim headless with nvim-lspconfig and blink.cmp against a given
`clangd` binary. It reports completion items blink receives from the LSP source and
can invoke Neovim's hover handler and report the returned contents. Use it for every
feature that adds completions (see the clangd completion contracts in the C++29
program plan, issue #362).

Example (do expressions, C++29):

    cxx26/dev/clangd-replay/replay.sh build-nyx/bin/clangd t.cpp \
      '[{"input":"ggjjOint z = do ","show":true,"report":"after-do","wait":3000}]' \
      -std=c++29

Traps: key input is only processed once the main loop runs, so steps are chained with
timers; type `<lt>` for `<`; wait for the first DiagnosticChanged (done by the script)
before typing, or the preamble does not exist yet; type the opening brace in a separate
step from the prefix that is completed afterwards. Set `CLANGD_DEV=1` in the owner's
own config to try the tree's clangd in the daily editor.

## Scenarios

`run-all.sh [clangd]` runs every `scenarios/<name>/` (`t.cpp`, `flags`, `steps.json`,
`expect`) and checks that, at the step with a given `report` label, an item is (`must`)
or is not (`never`) among the LSP items blink received. Hover steps use
`{"hover":true,"line":N,"col":N,"label":"name"}`; line is 0-based, column is a
0-based byte offset as in Neovim, and the captured response is checked with
`must-contain`/`never-contain`. The step moves with normal-mode `G` and `|`, calls
`vim.lsp.buf.hover()`, and captures its response callback. Every C++29
feature that adds completions ships one scenario directory (do expressions:
`scenarios/do-expr`); the close-out gate runs the script against `build-nyx/bin/clangd`
(done-checklist item 10, issue #359).
