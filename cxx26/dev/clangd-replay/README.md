# clangd replay in the real editor stack

`replay.sh` drives Neovim headless with nvim-lspconfig and blink.cmp against a given
`clangd` binary and prints the completion items blink receives from the LSP source, as
the editor user sees them. Use it for every feature that adds completions (see the
clangd completion contracts in the C++29 program plan, issue #362).

Example (do expressions, C++29):

    cxx26/dev/clangd-replay/replay.sh build-nyx/bin/clangd t.cpp \
      '[{"input":"ggjjOint z = do ","show":true,"report":"after-do","wait":3000}]' \
      -std=c++29 -fdo-expressions

Traps: key input is only processed once the main loop runs, so steps are chained with
timers; type `<lt>` for `<`; wait for the first DiagnosticChanged (done by the script)
before typing, or the preamble does not exist yet; type the opening brace in a separate
step from the prefix that is completed afterwards. Set `CLANGD_DEV=1` in the owner's
own config to try the tree's clangd in the daily editor.
