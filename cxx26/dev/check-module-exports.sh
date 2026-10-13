#!/usr/bin/env bash
# Run libc++'s module-export drift detectors (module_std.gen.py and
# module_std_compat.gen.py) against this fork's own clang-tidy.
#
# libc++'s CMake only builds the clang-tidy plugin these tests need when it finds a
# matching Clang development package, which this fork's build-nyx tree is not as far
# as CMake is concerned (it found a system-installed LLVM instead). This script builds
# the plugin by hand against build-nyx's headers with the exact flags clang-tidy was
# built with, installs it where the lit configuration looks for it for the duration of the
# run and removes it afterwards. It changes nothing in the source tree or in build-libcxx's
# CMake cache.
#
# Usage:
#   cxx26/dev/check-module-exports.sh
# Prerequisites: build-nyx (clang, clang-tidy) and build-libcxx are built and current.
# Afterwards remove the lit output directories (they are large).
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"

nyx="$repo_root/build-nyx"
libcxx_build="$repo_root/build-libcxx"
checks="$repo_root/libcxx/test/tools/clang_tidy_checks"
plugin_dir="$libcxx_build/libcxx/test/tools/clang_tidy_checks"
scratch="$(mktemp -d)"
# The plugin must not stay installed: lit enables every clang-tidy test as soon as the file
# exists, and a plain libcxx-lit run would then pair it with whatever clang-tidy is on PATH
# (an ABI mismatch with a system LLVM) and fail.
trap 'rm -rf "$scratch" "$plugin_dir/libcxx-tidy.plugin"' EXIT

echo "==> refreshing clang and clang-tidy (their embedded revisions must match)"
ninja -C "$nyx" -j"${JOBS:-12}" clang clang-tidy >/dev/null

# The flags clang-tidy itself was compiled with (assertions on, no RTTI/exceptions).
flags=(
  -D_DEBUG -D_GLIBCXX_ASSERTIONS -D_GLIBCXX_USE_CXX11_ABI=1 -D_GNU_SOURCE
  -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS
  -I"$nyx/tools/clang/tools/extra/clang-tidy" -I"$repo_root/clang-tools-extra"
  -I"$repo_root/clang-tools-extra/clang-tidy" -I"$repo_root/clang/include"
  -I"$nyx/tools/clang/include" -I"$nyx/include" -I"$repo_root/llvm/include"
  -fPIC -fno-semantic-interposition -fvisibility-inlines-hidden -O1 -UNDEBUG
  -fno-exceptions -funwind-tables -fno-rtti -std=c++20 -w
  -fno-sanitize=address,hwaddress,undefined,thread,leak
)

echo "==> building the libcxx-tidy plugin against build-nyx"
objects=()
for src in "$checks"/*.cpp; do
  obj="$scratch/$(basename "${src%.cpp}").o"
  /usr/bin/clang++ "${flags[@]}" -c "$src" -o "$obj"
  objects+=("$obj")
done
mkdir -p "$plugin_dir"
/usr/bin/clang++ -shared -o "$plugin_dir/libcxx-tidy.plugin" "${objects[@]}"

echo "==> running the detectors"
status=0
libcxx/utils/libcxx-lit "$libcxx_build" -sv -j"${JOBS:-12}" \
  --param clang-tidy-executable="$nyx/bin/clang-tidy" \
  libcxx/test/libcxx/module_std.gen.py libcxx/test/libcxx/module_std_compat.gen.py || status=$?

find "$libcxx_build/libcxx/test" -type d -name Output -prune -exec rm -rf {} + 2>/dev/null || true
exit "$status"
