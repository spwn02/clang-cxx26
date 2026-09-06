#!/usr/bin/env bash
# Idempotent configure for the fork's two persistent dev build trees.
# NEVER deletes an existing tree (unlike cxx26/toolchain/build-linux-x86_64.sh,
# which rm -rf's its arguments — do not point that script at these paths).
#
# Usage:
#   cxx26/dev/configure-build-trees.sh nyx      # configure/reconfigure build-nyx
#   cxx26/dev/configure-build-trees.sh libcxx   # configure/reconfigure build-libcxx (needs build-nyx/bin/clang)
#   cxx26/dev/configure-build-trees.sh libcxx-asan  # configure/reconfigure build-libcxx-asan (needs build-nyx's compiler-rt)
#   cxx26/dev/configure-build-trees.sh all      # nyx, libcxx, and libcxx-asan, in order
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"

JOBS="${JOBS:-$(nproc)}"
CCACHE_BIN="$(command -v ccache || true)"

configure_nyx() {
  echo "==> configuring build-nyx"
  local launcher_args=()
  if [[ -n "$CCACHE_BIN" ]]; then
    launcher_args=(-DCMAKE_C_COMPILER_LAUNCHER="$CCACHE_BIN" -DCMAKE_CXX_COMPILER_LAUNCHER="$CCACHE_BIN")
  fi
  # LLVM_ENABLE_ASSERTIONS=ON and LLVM_ENABLE_RUNTIMES=compiler-rt since the
  # 2026-09 Contracts hardening epic (docs/CONTRACTS_HARDENING.md M1) --
  # assertions catch the ICE/segfault class during development, and
  # compiler-rt is a hard prerequisite for any -fsanitize=* use against this
  # tree. Neither applies to the packaged release toolchain, which pins its
  # own flags in cxx26/toolchain/build-linux-x86_64.sh.
  cmake -S llvm -B build-nyx -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" \
    -DLLVM_ENABLE_RUNTIMES="compiler-rt" \
    -DLLVM_TARGETS_TO_BUILD=X86 \
    -DLLVM_ENABLE_ASSERTIONS=ON \
    -DLLVM_INCLUDE_TESTS=ON \
    -DCLANG_INCLUDE_TESTS=ON \
    -DCMAKE_C_COMPILER=/usr/bin/clang \
    -DCMAKE_CXX_COMPILER=/usr/bin/clang++ \
    "${launcher_args[@]}"
}

configure_libcxx() {
  echo "==> configuring build-libcxx"
  if [[ ! -x build-nyx/bin/clang ]]; then
    echo "error: build-nyx/bin/clang not built yet; run 'ninja -C build-nyx clang' first" >&2
    exit 1
  fi
  local launcher_args=()
  if [[ -n "$CCACHE_BIN" ]]; then
    launcher_args=(-DCMAKE_C_COMPILER_LAUNCHER="$CCACHE_BIN" -DCMAKE_CXX_COMPILER_LAUNCHER="$CCACHE_BIN")
  fi
  cmake -S runtimes -B build-libcxx -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi;libunwind" \
    -DCMAKE_C_COMPILER="$repo_root/build-nyx/bin/clang" \
    -DCMAKE_CXX_COMPILER="$repo_root/build-nyx/bin/clang++" \
    "${launcher_args[@]}"
}

configure_libcxx_asan() {
  echo "==> configuring build-libcxx-asan"
  if [[ ! -x build-nyx/bin/clang ]]; then
    echo "error: build-nyx/bin/clang not built yet; run 'ninja -C build-nyx clang' first" >&2
    exit 1
  fi
  if ! find build-nyx -maxdepth 6 -iname "libclang_rt.asan.so" | grep -q .; then
    echo "error: build-nyx's compiler-rt ASan runtime isn't built yet;" \
         "run 'ninja -C build-nyx compiler-rt' first" >&2
    exit 1
  fi
  local launcher_args=()
  if [[ -n "$CCACHE_BIN" ]]; then
    launcher_args=(-DCMAKE_C_COMPILER_LAUNCHER="$CCACHE_BIN" -DCMAKE_CXX_COMPILER_LAUNCHER="$CCACHE_BIN")
  fi
  # Separate tree, not a reconfigure of build-libcxx: LLVM_USE_SANITIZER
  # changes the ABI of the built libc++/libc++abi (sanitizer interceptors,
  # container-annotations), so it cannot share a build directory with the
  # plain instrumented-free tree used for ordinary check-cxx runs.
  cmake -S runtimes -B build-libcxx-asan -G Ninja \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi;libunwind" \
    -DLLVM_USE_SANITIZER="Address;Undefined" \
    -DCMAKE_C_COMPILER="$repo_root/build-nyx/bin/clang" \
    -DCMAKE_CXX_COMPILER="$repo_root/build-nyx/bin/clang++" \
    "${launcher_args[@]}"
}

case "${1:-all}" in
  nyx) configure_nyx ;;
  libcxx) configure_libcxx ;;
  libcxx-asan) configure_libcxx_asan ;;
  all) configure_nyx; configure_libcxx; configure_libcxx_asan ;;
  *) echo "usage: $0 {nyx|libcxx|libcxx-asan|all}" >&2; exit 1 ;;
esac
