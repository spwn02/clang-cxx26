#!/usr/bin/env bash
cd /home/spawn/dev/toolchains/clang-p2996
D="libcxx/test/std/utilities libcxx/test/std/containers libcxx/test/std/iterators libcxx/test/std/ranges libcxx/test/std/re libcxx/test/std/language.support libcxx/test/std/diagnostics libcxx/test/std/algorithms libcxx/test/std/strings libcxx/test/std/numerics"
for std in c++20 c++17 c++23; do
  libcxx/utils/libcxx-lit build-libcxx -sv -j10 --param std=$std $D > /tmp/cxx26-scratch/older-$std.txt 2>&1
  echo "OLDERDONE $std" >> /tmp/cxx26-scratch/older-$std.txt
  find build-libcxx/libcxx/test -type d -name Output -prune -exec rm -rf {} +
done
echo ALLOLDERDONE > /tmp/cxx26-scratch/older-all.txt
