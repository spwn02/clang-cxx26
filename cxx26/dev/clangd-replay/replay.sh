#!/usr/bin/env bash
# usage: replay.sh <clangd> <source-file> '<steps-json>' [compile flags...]
# Runs the steps against a scratch copy of <source-file> in the real editor stack
# (Neovim + nvim-lspconfig + blink.cmp) and prints what blink shows from the LSP source.
# Steps (JSON list): {"input": "<keys>", "line": N, "col": N, "show": bool, "report": "<label>", "wait": ms, "pre": ms}
#   - "input" goes through nvim_input (send "<lt>" for "<"); every step is chained with timers,
#     because inputs are only processed once the main loop runs.
#   - "show" asks blink to show the menu; "report" prints the LSP items at the end of the step.
set -euo pipefail
clangd="$1"; src="$2"; steps="$3"; shift 3
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
work="$(mktemp -d)"; trap 'rm -rf "$work"' EXIT
cp "$src" "$work/t.cpp"
flags="${*:--std=c++26}"
cat > "$work/compile_commands.json" <<JSON
[{"directory":"$work","command":"$(dirname "$clangd")/clang++ $flags -fsyntax-only $work/t.cpp","file":"$work/t.cpp"}]
JSON
cd "$work"
REPLAY_CLANGD="$clangd" REPLAY_FILE="$work/t.cpp" REPLAY_OUT="$work/out.txt" REPLAY_STEPS="$steps" \
  timeout 180 nvim --headless -u "$here/init.lua" -c "luafile $here/run.lua" >/dev/null 2>&1 || true
cat "$work/out.txt"
