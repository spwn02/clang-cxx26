#!/usr/bin/env bash
# compile a probe against the build tree's libc++ (syntax-only unless -o is given): cx.sh [flags] file.cpp
R=/home/spawn/dev/toolchains/clang-cxx26
exec $R/build-nyx/bin/clang++ -std=c++26 -nostdinc++ -isystem $R/build-libcxx/libcxx/test-suite-install/include/c++/v1 "$@"
