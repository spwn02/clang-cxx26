#!/usr/bin/env bash
# compile and run a probe: cxrun.sh file.cpp  (binary goes to ${TMPDIR:-/tmp}/cxrun.out)
R=/home/spawn/dev/toolchains/clang-p2996
L=$R/build-libcxx/libcxx/test-suite-install/lib
O=${TMPDIR:-/tmp}/cxrun.out
$(dirname "$0")/cx.sh "$@" -L $L -Wl,-rpath,$L -lc++ -o $O && $O
