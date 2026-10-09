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

# MPFR/GMP back the constexpr evaluation of the P1383R2 transcendental
# <cmath> functions; they are linked statically (see build-mpfr-gmp.sh).
mpfr_prefix="${CXX26_MPFR_PREFIX:-${build_dir}-mpfr-gmp}"
"${repo_root}/cxx26/toolchain/build-mpfr-gmp.sh" "${mpfr_prefix}"

runtime_components="cxx;cxxabi;unwind"
# NOTE: "compiler-rt" itself must NOT appear in runtime_distribution_components:
# it's already a full entry in LLVM_ENABLE_RUNTIMES, which makes the runtimes
# build register its own install-compiler-rt/-stripped targets automatically.
# Listing it again here makes LLVM try to define those same targets a second
# time and fail to configure ("add_custom_target ... already exists"). Only
# genuine sub-components (not full runtime names) belong here.
runtime_distribution_components="cxx-modules;compiler-rt-headers"
distribution_components="clang;clangd;clang-tidy;clang-resource-headers;clang-scan-deps;lld;llvm-ar;${runtime_components};compiler-rt;${runtime_distribution_components}"

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
  -DMPFR_INCLUDE_DIR="${mpfr_prefix}/include" \
  -DMPFR_LIBRARY="${mpfr_prefix}/lib/libmpfr.a" \
  -DGMP_INCLUDE_DIR="${mpfr_prefix}/include" \
  -DGMP_LIBRARY="${mpfr_prefix}/lib/libgmp.a" \
  -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra;lld" \
  -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi;libunwind;compiler-rt" \
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

if ! grep -q '#define CLANG_HAVE_MPFR 1' "${build_dir}/tools/clang/include/clang/Config/config.h"; then
  echo "clang was configured without MPFR/GMP: no constexpr transcendental <cmath>" >&2
  exit 1
fi

"${cmake_bin}" --build "${build_dir}" --target distribution --parallel "${jobs}"
"${cmake_bin}" --build "${build_dir}" --target install-distribution --parallel "${jobs}"

mkdir -p "${install_prefix}/share/clang-cxx26/licenses"
cp -r "${mpfr_prefix}/share/licenses/." "${install_prefix}/share/clang-cxx26/licenses/"

test -x "${install_prefix}/bin/clang"
test -x "${install_prefix}/bin/clang++"
test -x "${install_prefix}/bin/clang-scan-deps"
test -x "${install_prefix}/bin/clang-tidy"

if ! find "${install_prefix}" -name 'libc++.modules.json' -print -quit | grep -q .; then
  echo "reference toolchain install does not contain libc++.modules.json" >&2
  exit 1
fi

if ! find "${install_prefix}" -name 'libclang_rt.ubsan_standalone*' -print -quit | grep -q .; then
  echo "reference toolchain install does not contain the UBSan runtime (libclang_rt.ubsan_standalone)" >&2
  exit 1
fi

# Release gate (#283): nothing that the C++ draft does not mention may be public. Every build that
# can become a published prerelease or release runs the vocabulary audit against the installed
# toolchain and fails on any unexplained header, name or macro. The reference is the draft tag the
# release targets.
draft_tag="${CXX26_DRAFT_TAG:-n5050}"
draft_dir="${CXX26_DRAFT_DIR:-${build_dir}-draft}"
if [[ ! -d "${draft_dir}/source" ]]; then
  git clone --quiet --depth 1 --branch "${draft_tag}" https://github.com/cplusplus/draft.git "${draft_dir}"
fi
libcxx_include="${install_prefix}/include/c++/v1"
extra_includes=()
for dir in "${install_prefix}"/include/*/c++/v1; do
  [[ -d "${dir}" ]] && extra_includes+=(--extra-include "${dir}")
done
# Twice: with the default C++26 feature set and with the experimental library on (-fexperimental-library
# exposes more of the headers). The audit fails closed: it exits 2 when its own setup cannot compile
# <vector>, <algorithm> and <string>.
for audit_flags in "-freflection-latest" "-freflection-latest -fexperimental-library"; do
  python3 -I "${repo_root}/libcxx/utils/vocabulary_audit.py" \
    --draft "${draft_dir}" \
    --sd6 "${repo_root}/libcxx/utils/sd6_lib_macros.txt" \
    --allow "${repo_root}/libcxx/utils/vocabulary_allowlist.txt" \
    --clang "${install_prefix}/bin/clang++" \
    --include "${libcxx_include}" ${extra_includes[@]+"${extra_includes[@]}"} \
    "--flags=${audit_flags}"
done
