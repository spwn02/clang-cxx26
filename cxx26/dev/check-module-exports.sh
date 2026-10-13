#!/usr/bin/env bash
# Run libc++'s clang-tidy based tests: the module-export drift detectors (module_std.gen.py,
# module_std_compat.gen.py) and, with EXTRA_TESTS, others that need the libcxx-tidy plugin
# (e.g. EXTRA_TESTS="libcxx/test/libcxx/clang_tidy.gen.py").
#
# The plugin is built by libc++'s CMake against build-nyx: build-libcxx has to be configured with
#   -DClang_DIR=<build-nyx>/lib/cmake/clang -DLLVM_DIR=<build-nyx>/lib/cmake/llvm
#   -DLIBCXX_CLANG_TIDY_EXTRA_INCLUDE_DIRS="<src>/clang-tools-extra;<src>/clang-tools-extra/clang-tidy;<build-nyx>/tools/clang/tools/extra/clang-tidy"
# (see libcxx/test/tools/clang_tidy_checks/CMakeLists.txt); the lit configuration then finds the clang-tidy
# next to the compiler under test. clang, clang-tidy and the plugin must come from the same revision, so
# this script rebuilds them first.
#
# Afterwards remove the lit output directories (they are large).
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"
nyx="$repo_root/build-nyx"
libcxx_build="$repo_root/build-libcxx"

echo "==> refreshing clang, clang-tidy and the libcxx-tidy plugin (their embedded revisions must match)"
ninja -C "$nyx" -j"${JOBS:-12}" clang clang-tidy >/dev/null
ninja -C "$libcxx_build" -j"${JOBS:-12}" cxx-tidy >/dev/null

echo "==> running the tests"
status=0
# shellcheck disable=SC2086
libcxx/utils/libcxx-lit "$libcxx_build" -sv -j"${JOBS:-12}" \
  libcxx/test/libcxx/module_std.gen.py libcxx/test/libcxx/module_std_compat.gen.py ${EXTRA_TESTS:-} || status=$?

find "$libcxx_build/libcxx/test" -type d -name Output -prune -exec rm -rf {} + 2>/dev/null || true
exit "$status"
