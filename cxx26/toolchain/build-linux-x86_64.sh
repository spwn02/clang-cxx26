#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 <install-prefix> <build-directory>" >&2
  exit 2
fi

repo_root="$(git rev-parse --show-toplevel)"
install_prefix="$(realpath -m "$1")"
build_dir="$(realpath -m "$2")"
cmake_bin="${CMAKE:-cmake}"
jobs="${CXX26_BUILD_JOBS:-$(nproc)}"
compiler_launcher="${CXX26_COMPILER_LAUNCHER:-}"

rm -rf "${install_prefix}" "${build_dir}"

runtime_components="cxx;cxxabi;unwind"
runtime_distribution_components="cxx-modules"
distribution_components="clang;clangd;clang-resource-headers;clang-scan-deps;lld;llvm-ar;${runtime_components};${runtime_distribution_components}"

launcher_args=()
if [[ -n "${compiler_launcher}" ]]; then
  launcher_args+=(
    "-DCMAKE_C_COMPILER_LAUNCHER=${compiler_launcher}"
    "-DCMAKE_CXX_COMPILER_LAUNCHER=${compiler_launcher}"
  )
fi

"${cmake_bin}" \
  -S "${repo_root}/llvm" \
  -B "${build_dir}" \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="${install_prefix}" \
  -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra;lld" \
  -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi;libunwind" \
  -DLLVM_TARGETS_TO_BUILD="X86" \
  -DLLVM_INSTALL_TOOLCHAIN_ONLY=ON \
  -DLLVM_ENABLE_ASSERTIONS=OFF \
  -DLLVM_ENABLE_BINDINGS=OFF \
  -DLLVM_INCLUDE_BENCHMARKS=OFF \
  -DLLVM_INCLUDE_DOCS=OFF \
  -DLLVM_INCLUDE_EXAMPLES=OFF \
  -DLLVM_INCLUDE_TESTS=OFF \
  -DCLANG_INCLUDE_DOCS=OFF \
  -DCLANG_INCLUDE_TESTS=OFF \
  -DCLANG_ENABLE_ARCMT=OFF \
  -DCLANG_ENABLE_STATIC_ANALYZER=OFF \
  -DLLVM_ENABLE_TERMINFO=OFF \
  -DLLVM_ENABLE_ZLIB=OFF \
  -DLLVM_ENABLE_ZSTD=OFF \
  -DLLVM_ENABLE_LIBXML2=OFF \
  -DLLVM_ENABLE_CURL=OFF \
  -DCLANG_DEFAULT_CXX_STDLIB="libc++" \
  -DCLANG_DEFAULT_LINKER="lld" \
  -DLIBCXX_ENABLE_SHARED=ON \
  -DLIBCXX_ENABLE_STATIC=ON \
  -DLIBCXX_INCLUDE_BENCHMARKS=OFF \
  -DLIBCXX_INCLUDE_TESTS=OFF \
  -DLIBCXX_INSTALL_MODULES=ON \
  -DLIBCXXABI_ENABLE_SHARED=ON \
  -DLIBCXXABI_ENABLE_STATIC=ON \
  -DLIBCXXABI_INCLUDE_TESTS=OFF \
  -DLIBCXXABI_USE_LLVM_UNWINDER=ON \
  -DLIBUNWIND_ENABLE_SHARED=ON \
  -DLIBUNWIND_ENABLE_STATIC=ON \
  -DLIBUNWIND_INCLUDE_TESTS=OFF \
  -DLLVM_RUNTIME_DISTRIBUTION_COMPONENTS="${runtime_distribution_components}" \
  -DLLVM_DISTRIBUTION_COMPONENTS="${distribution_components}" \
  "${launcher_args[@]}"

"${cmake_bin}" --build "${build_dir}" --target distribution --parallel "${jobs}"
"${cmake_bin}" --build "${build_dir}" --target install-distribution --parallel "${jobs}"

test -x "${install_prefix}/bin/clang"
test -x "${install_prefix}/bin/clang++"
test -x "${install_prefix}/bin/clang-scan-deps"

if ! find "${install_prefix}" -name 'libc++.modules.json' -print -quit | grep -q .; then
  echo "reference toolchain install does not contain libc++.modules.json" >&2
  exit 1
fi
