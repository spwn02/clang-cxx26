#!/usr/bin/env bash
# Release gate for a C++26 toolchain candidate (issues #168 and #283). Runs every verification step in
# order, gates each on its exit status, and prints one summary table. Nothing is promoted unless the
# whole table is green.
#
# Steps (select with STEPS="build clang clangd libcxx-<std>... modules audit"; default: all):
#   build        ninja: clang clang-tidy clang-query clang-scan-deps clangd clang-extdef-mapping c-index-test
#                (build-nyx) and cxx-tidy, cxx-test-depends (build-libcxx); both must exit 0
#   clang        check-clang (the whole clang test suite)
#   clangd       check-clangd (lit) and the ClangdTests unit binary
#   libcxx-<std> the libc++ suite at that -std, one lit run per test directory
#                (cxx26/dev/sweep-libcxx-std.sh), tallied by sweep-summary.py. <std> is each of
#                STDS (default: c++26 c++23 c++20 c++17 c++14 c++11 c++03)
#   modules      the std / std.compat module export drift tests at c++23 and c++26
#   audit-modes  vocabulary audit at c++23..c++11 against each draft, compared with upstream libc++
#                (cxx26/dev/audit-older-modes.sh); restores DRAFT_DIR to DRAFT_TAG
#   audit        vocabulary audit (#283) against the draft tag in DRAFT_DIR, with and without
#                -fexperimental-library; run last on purpose
#
# Env: JOBS (default nproc), STDS, STEPS, OUT (default $HOME/.local/share/cxx26-release-gate/<HEAD>),
#      DRAFT_DIR (a cplusplus/draft checkout that has the tag DRAFT_TAG, default n5050).
# Never run it while another build or lit run uses the build trees, and do not edit the sources while it
# runs (the lit runs reinstall the headers from the working tree on every call). Needs about 3 GB of
# free disk; the sweep deletes the lit Output directories between test directories.
# `release-gate.sh --list` prints the plan and exits.
set -uo pipefail
ulimit -c 0

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
jobs="${JOBS:-$(nproc)}"
stds="${STDS:-c++26 c++23 c++20 c++17 c++14 c++11 c++03}"
head_rev="$(git rev-parse --short HEAD)"
out="${OUT:-$HOME/.local/share/cxx26-release-gate/$head_rev}"
draft_dir="${DRAFT_DIR:-}"
draft_tag="${DRAFT_TAG:-n5050}"
nyx="$repo_root/build-nyx"
lcx="$repo_root/build-libcxx"

default_steps="build clang clangd"
for s in $stds; do default_steps="$default_steps libcxx-$s"; done
default_steps="$default_steps modules audit-modes audit"
steps="${STEPS:-$default_steps}"

if [[ "${1:-}" == "--list" ]]; then
  echo "revision $head_rev, jobs $jobs, output $out"
  for s in $steps; do echo "  $s"; done
  exit 0
fi

mkdir -p "$out"
if [[ -n "$(git status --porcelain --untracked-files=no)" ]]; then
  echo "refusing to run: the working tree has uncommitted changes (the table must name a revision)" >&2
  exit 2
fi
df_free_gb=$(df --output=avail -BG /home | tail -1 | tr -dc 0-9)
if (( df_free_gb < 3 )); then echo "refusing to run: only ${df_free_gb} GB free on /home" >&2; exit 2; fi

declare -a names results details
record() { names+=("$1"); results+=("$2"); details+=("$3"); printf '%-22s %-6s %s\n' "$1" "$2" "$3"; }

# run <step> <logfile> <command...>: record PASS/FAIL by exit status
run() {
  local step="$1" log="$2"; shift 2
  local t0=$SECONDS
  if "$@" >"$log" 2>&1; then record "$step" PASS "$((SECONDS - t0))s, log $log"; else record "$step" FAIL "exit $?, $((SECONDS - t0))s, log $log"; fi
}

step_build() {
  run build-nyx "$out/build-nyx.log" ninja -C "$nyx" -j"$jobs" clang clang-tidy clang-query clang-scan-deps clangd clang-extdef-mapping c-index-test
  run build-libcxx "$out/build-libcxx.log" ninja -C "$lcx" -j"$jobs" cxx-tidy cxx-test-depends
}
step_clang() { run check-clang "$out/check-clang.log" ninja -C "$nyx" -j"$jobs" check-clang; }
step_clangd() {
  run check-clangd "$out/check-clangd.log" ninja -C "$nyx" -j"$jobs" check-clangd
  run ClangdTests "$out/ClangdTests.log" "$nyx/tools/clang/tools/extra/clangd/unittests/ClangdTests"
}
step_libcxx() {
  local std="$1" log="$out/sweep-$1.log"
  if bash cxx26/dev/sweep-libcxx-std.sh "$std" "$log" >"$out/sweep-$std.out" 2>&1 \
     && python3 cxx26/dev/sweep-summary.py "$log" >"$out/sweep-$std.summary" 2>&1; then
    record "libcxx-$std" PASS "$(head -1 "$out/sweep-$std.summary" | sed 's/^[^:]*: //')"
  else
    record "libcxx-$std" FAIL "$(grep -E 'BAD|INCOMPLETE' "$out/sweep-$std.summary" 2>/dev/null | head -3 | tr '\n' ';') see $log"
  fi
}
step_modules() {
  for std in c++23 c++26; do
    run "modules-$std" "$out/modules-$std.log" libcxx/utils/libcxx-lit "$lcx" -sv -j"$jobs" --param std="$std" \
      libcxx/test/libcxx/module_std.gen.py libcxx/test/libcxx/module_std_compat.gen.py libcxx/test/std/modules
  done
}
step_audit_modes() {
  if [[ -z "$draft_dir" || ! -d "$draft_dir/source" ]]; then record audit-modes FAIL "DRAFT_DIR is not a cplusplus/draft checkout"; return; fi
  run audit-modes "$out/audit-modes.log" bash cxx26/dev/audit-older-modes.sh "$draft_dir" "$out/audit-modes"
}
step_audit() {
  if [[ -z "$draft_dir" || ! -d "$draft_dir/source" ]]; then record audit FAIL "DRAFT_DIR is not a cplusplus/draft checkout"; return; fi
  if [[ "$(git -C "$draft_dir" rev-parse HEAD 2>/dev/null)" != "$(git -C "$draft_dir" rev-parse "$draft_tag^{commit}" 2>/dev/null)" ]]; then
    record audit FAIL "DRAFT_DIR is not checked out at $draft_tag (the audit reads the working tree)"; return
  fi
  local inc="$lcx/libcxx/test-suite-install/include/c++/v1" extra=()
  for d in "$lcx"/libcxx/test-suite-install/include/*/c++/v1; do [[ -d "$d" ]] && extra+=(--extra-include "$d"); done
  local flags
  for flags in "-freflection-latest" "-freflection-latest -fexperimental-library"; do
    run "audit [$flags]" "$out/audit-${flags// /_}.log" python3 -I libcxx/utils/vocabulary_audit.py \
      --draft "$draft_dir" --sd6 libcxx/utils/sd6_lib_macros.txt --allow libcxx/utils/vocabulary_allowlist.txt \
      --clang "$nyx/bin/clang++" --include "$inc" ${extra[@]+"${extra[@]}"} "--flags=$flags"
  done
}

echo "release gate at $head_rev, steps: $steps"
for s in $steps; do
  case "$s" in
    build) step_build ;;
    clang) step_clang ;;
    clangd) step_clangd ;;
    libcxx-*) step_libcxx "${s#libcxx-}" ;;
    modules) step_modules ;;
    audit-modes) step_audit_modes ;;
    audit) step_audit ;;
    *) record "$s" FAIL "unknown step" ;;
  esac
done

echo
echo "=== release gate summary, revision $head_rev ==="
fail=0
for i in "${!names[@]}"; do
  printf '%-22s %-6s %s\n' "${names[$i]}" "${results[$i]}" "${details[$i]}"
  [[ "${results[$i]}" == PASS ]] || fail=1
done | tee "$out/summary.txt"
for r in "${results[@]}"; do [[ "$r" == PASS ]] || fail=1; done
if (( fail )); then echo "GATE: FAIL"; else echo "GATE: PASS"; fi | tee -a "$out/summary.txt"
exit "$fail"
