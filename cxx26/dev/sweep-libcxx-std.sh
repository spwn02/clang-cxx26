#!/usr/bin/env bash
# usage: sweep-libcxx-std.sh <std> <outfile>   -- stops at the first directory with a failure (FAIL_FAST=0 disables); runs libc++ lit per test directory (and per top-level
# libcxx/test/libcxx file), deleting Output between.
# libcxx-lit reinstalls headers from the working tree on every call: do not edit libcxx/include while a
# sweep runs (use a worktree). The clang_tidy.gen.py tests need the plugin, see check-module-exports.sh.
cd /home/spawn/dev/toolchains/clang-p2996
export CCACHE_DIR=/tmp/cxx26-scratch/ccache
std="$1"; out="$2"; : > "$out"
# The main headers are not C++03-clean (upstream neither); C++03 is tested against the frozen C++03 headers.
extra=(); [ "$std" = c++03 ] && extra=(--param test_frozen_cxx03_headers=True)
for d in libcxx/test/std/* libcxx/test/libcxx/* libcxx/test/extensions/gnu libcxx/test/extensions/libcxx libcxx/test/extensions/all libcxx/test/extensions/msvc libcxx/test/extensions/posix libcxx/test/extensions/clang/thread libcxx/test/support; do
  [ -e "$d" ] || continue
  case "$d" in *benchmarks*|*.cfg) continue;; esac
  r=$(libcxx/utils/libcxx-lit build-libcxx -sv -j"${JOBS:-$(nproc)}" --param std="$std" ${extra[@]+"${extra[@]}"} "$d" 2>&1 | grep -E "^  (Passed|Failed|Unsupported|Unresolved|Expectedly|XPass)|^FAIL|^XPASS|^UNRESOLVED|Total Discovered" | tr '\n' '|')
  echo "$d :: $r" >> "$out"
  # Stop at the first failing directory (FAIL_FAST=0 keeps going): investigate before spending more time.
  if [ "${FAIL_FAST:-1}" = 1 ] && grep -qE '\|?  (Failed|Unresolved)|FAIL|UNRESOLVED|XPASS' <<<"$r"; then
    echo "SWEEP-ABORTED at $d" >> "$out"; echo "SWEEP-ABORTED at $d (first failure)" >&2; exit 1
  fi
  find build-libcxx/libcxx/test -type d -name Output -prune -exec rm -rf {} + 2>/dev/null
done
echo SWEEP-DONE >> "$out"
