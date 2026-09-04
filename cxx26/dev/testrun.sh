#!/usr/bin/env bash
# Run a named test suite and archive its stamped lit JSON result to
# ~/.local/share/cxx26-contracts/results/. Never runs inside /tmp (parallel bg
# jobs clobber it) and never inside build-*/ (deleted-tree risk).
#
# Usage: cxx26/dev/testrun.sh <suite> [-- extra llvm-lit / lit-args]
#   suites:
#     check-clang         ninja target: full check-clang
#     check-cxx            ninja target: full check-cxx
#     contracts             clang/test/Contracts + clang/test/Parser contract tests
#     contracts-lib          libcxx contracts suite (--param use-contracts=True)
#     reflection             clang/test/Reflection
#     reflection-lib          libcxx reflection suite
#     semacxx                clang/test/SemaCXX
#     serialization           clang/test/AST/ByteCode + Modules + PCH + Import
#     regression-clusters     the 5 named open-regression test files
#
# Env:
#   ARCHIVE_DIR   override archive root (default ~/.local/share/cxx26-contracts)
#   LABEL         extra label appended to the archive filename
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"

ARCHIVE_DIR="${ARCHIVE_DIR:-$HOME/.local/share/cxx26-contracts}"
RESULTS_DIR="$ARCHIVE_DIR/results"
LITTIMES_DIR="$ARCHIVE_DIR/lit-times"
mkdir -p "$RESULTS_DIR" "$LITTIMES_DIR"

suite="${1:?usage: testrun.sh <suite> [-- extra args]}"
shift || true
extra_args=("$@")

sha="$(git rev-parse HEAD)"
short_sha="$(git rev-parse --short=12 HEAD)"
stamp="$(date -u +%Y%m%dT%H%M%SZ)"
label="${LABEL:-}"
[[ -n "$label" ]] && label="-$label"

fingerprint() {
  # A short, stable fingerprint of the CMake config that matters for
  # comparability: build type, assertions, tests-included, targets.
  local cache="$1"
  if [[ ! -f "$cache" ]]; then echo "no-cache"; return; fi
  grep -E '^(CMAKE_BUILD_TYPE|LLVM_ENABLE_ASSERTIONS|LLVM_INCLUDE_TESTS|CLANG_INCLUDE_TESTS|LLVM_TARGETS_TO_BUILD|LLVM_ENABLE_PROJECTS|LLVM_ENABLE_RUNTIMES):' "$cache" \
    | sort | sha256sum | cut -c1-16
}

nyx_fp="$(fingerprint build-nyx/CMakeCache.txt)"
libcxx_fp="$(fingerprint build-libcxx/CMakeCache.txt)"
clang_version="unknown"
[[ -x build-nyx/bin/clang ]] && clang_version="$(build-nyx/bin/clang --version | head -1)"

out_json="$RESULTS_DIR/${suite}-${stamp}-${short_sha}${label}.json"
meta_json="$RESULTS_DIR/${suite}-${stamp}-${short_sha}${label}.meta.json"
latest_link="$RESULTS_DIR/${suite}-latest.json"
latest_meta_link="$RESULTS_DIR/${suite}-latest.meta.json"

run_lit() {
  # $1 = path/target-list description (for logging), remaining = llvm-lit args
  local desc="$1"; shift
  echo "==> $desc"
  ./build-nyx/bin/llvm-lit -q -o "$out_json" --xunit-xml-output "${out_json%.json}.xunit.xml" --time-tests "$@" "${extra_args[@]}"
}

run_ninja_target() {
  local tree="$1" target="$2"
  echo "==> ninja -C $tree $target"
  LIT_OPTS="-o $out_json --xunit-xml-output ${out_json%.json}.xunit.xml --time-tests ${extra_args[*]:-}" \
    ninja -C "$tree" -j"$(nproc)" "$target"
}

# Test suites are expected to sometimes fail (that's the whole point of
# running them) — don't let set -e abort before metadata/archiving below.
set +e
case "$suite" in
  check-clang)
    run_ninja_target build-nyx check-clang
    ;;
  check-cxx)
    rm -rf build-libcxx/libcxx/test/extensions/clang/Output/clang_modules_include* 2>/dev/null || true
    run_ninja_target build-libcxx check-cxx
    ;;
  contracts)
    run_lit "clang/test/Contracts" clang/test/Contracts
    ;;
  contracts-lib)
    libcxx/utils/libcxx-lit build-libcxx -q -o "$out_json" --time-tests \
      --param use-contracts=True libcxx/test/std/contracts "${extra_args[@]}"
    ;;
  reflection)
    run_lit "clang/test/Reflection" clang/test/Reflection
    ;;
  reflection-lib)
    libcxx/utils/libcxx-lit build-libcxx -q -o "$out_json" --time-tests \
      libcxx/test/std/experimental/reflection "${extra_args[@]}"
    ;;
  semacxx)
    run_lit "clang/test/SemaCXX" clang/test/SemaCXX
    ;;
  serialization)
    run_lit "AST/ByteCode + Modules + PCH + Import" \
      clang/test/AST/ByteCode clang/test/Modules clang/test/PCH clang/test/Import
    ;;
  regression-clusters)
    run_lit "named open-regression clusters" \
      clang/test/Reflection/splice-exprs.cpp \
      clang/test/SemaCXX/builtin-is-within-lifetime.cpp \
      clang/test/SemaCXX/constant-expression-cxx11.cpp \
      clang/test/SemaCXX/cxx2b-consteval-propagate.cpp \
      clang/test/SemaCXX/cxx2a-constexpr-dynalloc.cpp
    clang_rc=$?
    libcxx/utils/libcxx-lit build-libcxx -q -o "${out_json%.json}.lib.json" --time-tests \
      libcxx/test/std/experimental/reflection/reflection-ex-parsing-command-line-options-2.sh.cpp
    lib_rc=$?
    suite_rc=$(( clang_rc != 0 || lib_rc != 0 ))
    ;;
  *)
    echo "unknown suite: $suite" >&2
    exit 1
    ;;
esac
suite_rc="${suite_rc:-$?}"
set -e

python3 - "$out_json" "$meta_json" "$sha" "$stamp" "$nyx_fp" "$libcxx_fp" "$clang_version" "$suite" "$suite_rc" << 'PYEOF'
import json, sys
out_json, meta_json, sha, stamp, nyx_fp, libcxx_fp, clang_version, suite, suite_rc = sys.argv[1:10]
meta = {
    "suite": suite,
    "cxx26_sha": sha,
    "stamp_utc": stamp,
    "nyx_config_fp": nyx_fp,
    "libcxx_config_fp": libcxx_fp,
    "clang_version": clang_version,
    "result_file": out_json,
    "suite_exit_code": int(suite_rc),
}
with open(meta_json, "w") as f:
    json.dump(meta, f, indent=2)
try:
    with open(out_json) as f:
        data = json.load(f)
    codes = {}
    for t in data.get("tests", []):
        codes[t["code"]] = codes.get(t["code"], 0) + 1
    print("==> result summary:", codes)
except FileNotFoundError:
    print("WARNING: no JSON output written by lit for this run", file=sys.stderr)
PYEOF

ln -sf "$(basename "$out_json")" "$latest_link"
ln -sf "$(basename "$meta_json")" "$latest_meta_link"
echo "==> archived: $out_json"
echo "==> metadata: $meta_json"

# Back up .lit_test_times.txt so it survives a tree deletion.
for f in build-nyx/.lit_test_times.txt build-libcxx/.lit_test_times.txt; do
  [[ -f "$f" ]] && cp "$f" "$LITTIMES_DIR/$(basename "$(dirname "$f")").lit_test_times.txt"
done

exit "$suite_rc"
