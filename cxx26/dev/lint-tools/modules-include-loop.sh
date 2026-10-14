#!/usr/bin/env bash
cd /home/spawn/dev/toolchains/clang-p2996
: > $CLAUDE_JOB_DIR/tmp/modloop.out
while read t; do
  libcxx/utils/libcxx-lit build-libcxx -sv -j1 --filter "clang_modules_include.gen.py/${t%.compile.pass.cpp}.compile" libcxx/test/extensions/clang/clang_modules_include.gen.py 2>&1 | grep -E "^FAIL|Passed|Failed|error:" | head -3 >> $CLAUDE_JOB_DIR/tmp/modloop.out
  echo "== $t" >> $CLAUDE_JOB_DIR/tmp/modloop.out
  find build-libcxx/libcxx/test -type d -name Output -prune -exec rm -rf {} +
done < $CLAUDE_JOB_DIR/tmp/modfail.txt
echo LOOPDONE >> $CLAUDE_JOB_DIR/tmp/modloop.out
