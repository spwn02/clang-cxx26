#!/usr/bin/env bash
cd /home/spawn/dev/toolchains/clang-p2996
I=build-libcxx/libcxx/test-suite-install/include/c++/v1
T=$CLAUDE_JOB_DIR/tmp/hs; mkdir -p $T
one() { std=$1; h=$2; f=$T/$h.$std.cpp; echo "#include <$h>" > $f
  build-nyx/bin/clang++ -std=c++$std -nostdinc++ -I $I -fsyntax-only $f >$T/$h.$std.log 2>&1 || echo "FAIL c++$std $h"; }
export -f one; export I T
for std in 26 20; do for h in $(cd libcxx/include; ls | grep -vE '^(_|CMake|module|libcxx|__)' | grep -vE '\.(h|inc|in|txt|modulemap)$'); do echo "$std $h"; done; done | xargs -P12 -L1 bash -c 'one $0 $1'
echo sweep-done
