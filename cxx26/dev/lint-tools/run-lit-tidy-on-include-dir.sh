#!/usr/bin/env bash
# usage: runtidy.sh <include-dir> <outdir>
INC=$1; OUT=$2; R=/home/spawn/dev/toolchains/clang-p2996; mkdir -p $OUT
run() { f=$1; b=$(basename $f .sh.cpp)
  $R/build-nyx/bin/clang-tidy $f --warnings-as-errors='*' -header-filter='.*' --config-file=$R/libcxx/.clang-tidy --load=$R/build-libcxx/libcxx/test/tools/clang_tidy_checks/libcxx-tidy.plugin -- -Wweak-vtables -pthread --target=x86_64-unknown-linux-gnu -nostdinc++ -I $INC -I $INC -I $R/libcxx/test/support -std=c++26 -Werror -Wall -Wctad-maybe-unsupported -Wextra -Wshadow -Wundef -Wunused-template -Wno-unused-command-line-argument -Wno-attributes -Wno-pessimizing-move -Wno-noexcept-type -Wno-atomic-alignment -Wno-reserved-module-identifier -Wdeprecated-copy -Wdeprecated-copy-dtor -Wshift-negative-value -Wno-user-defined-literals -Wno-tautological-compare -Wsign-compare -Wunused-variable -Wunused-parameter -Wunreachable-code -Wno-unused-local-typedef -Wno-local-type-template-args -Wno-c++11-extensions -Wno-unknown-pragmas -Wno-pass-failed -Wno-mismatched-new-delete -Wno-redundant-move -Wno-self-move -Wno-nullability-completeness -flax-vector-conversions=none -D_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER -D_LIBCPP_ENABLE_EXPERIMENTAL -D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_NONE -Wuser-defined-warnings -fno-modules > $OUT/$b.log 2>&1; }
export -f run; export INC OUT R
ls $R/build-libcxx/libcxx/test/libcxx/clang_tidy.gen.py/*.sh.cpp | xargs -P12 -I{} bash -c 'run {}'
echo tidy-done
