#!/usr/bin/env bash
# Builds the pinned GMP and MPFR releases as static, position-independent
# libraries. clang's constant evaluator uses them for the P1383R2
# transcendental <cmath> functions (CLANG_HAVE_MPFR); linking them statically
# keeps the reference toolchain relocatable and free of system library
# dependencies.
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 <install-prefix>" >&2
  exit 2
fi

prefix="$(realpath -m "$1")"
jobs="${CXX26_BUILD_JOBS:-$(nproc)}"

gmp_version=6.3.0
gmp_sha256=a3c2b80201b89e68616f4ad30bc66aee4927c3ce50e33929ca819d5c43538898
mpfr_version=4.2.1
mpfr_sha256=277807353a6726978996945af13e52829e3abd7a9a5b7fb2793894e18f1fcbb2

if [[ -f "${prefix}/lib/libgmp.a" && -f "${prefix}/lib/libmpfr.a" &&
      -f "${prefix}/.versions-${gmp_version}-${mpfr_version}" ]]; then
  exit 0
fi

work="$(mktemp -d)"
trap 'rm -rf "${work}"' EXIT
rm -rf "${prefix}"
mkdir -p "${prefix}"

fetch() {
  local url="$1" sha256="$2" file="${work}/$(basename "$1")"
  curl --fail --silent --show-error --location --retry 3 -o "${file}" "${url}"
  echo "${sha256}  ${file}" | sha256sum --check --quiet -
  tar -C "${work}" -xf "${file}"
}

fetch "https://ftp.gnu.org/gnu/gmp/gmp-${gmp_version}.tar.xz" "${gmp_sha256}"
fetch "https://ftp.gnu.org/gnu/mpfr/mpfr-${mpfr_version}.tar.xz" "${mpfr_sha256}"

export CFLAGS="-O2 -fPIC -std=gnu17"
(
  cd "${work}/gmp-${gmp_version}"
  # GMP's assembly needs m4. --enable-fat selects the CPU-specific code at run time instead of tuning it to the
  # build host (which could otherwise fault on an older CPU); without m4 (or with CXX26_GMP_NO_ASM=1) fall back to
  # the portable C implementation.
  if [[ -z "${CXX26_GMP_NO_ASM:-}" ]] && command -v m4 >/dev/null; then
    gmp_cpu_flags=(--enable-fat)
  else
    gmp_cpu_flags=(--disable-assembly)
  fi
  ./configure --prefix="${prefix}" --enable-static --disable-shared --with-pic "${gmp_cpu_flags[@]}"
  make -j"${jobs}"
  make install
)
(
  cd "${work}/mpfr-${mpfr_version}"
  ./configure --prefix="${prefix}" --enable-static --disable-shared --with-pic \
    --with-gmp="${prefix}"
  make -j"${jobs}"
  make install
)

# The license texts that accompany the statically linked libraries.
mkdir -p "${prefix}/share/licenses/gmp" "${prefix}/share/licenses/mpfr"
cp "${work}/gmp-${gmp_version}/COPYING.LESSERv3" "${work}/gmp-${gmp_version}/COPYINGv2" \
   "${work}/gmp-${gmp_version}/COPYINGv3" "${prefix}/share/licenses/gmp/"
cp "${work}/mpfr-${mpfr_version}/COPYING.LESSER" "${work}/mpfr-${mpfr_version}/COPYING" "${prefix}/share/licenses/mpfr/"

touch "${prefix}/.versions-${gmp_version}-${mpfr_version}"
