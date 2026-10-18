// A module file that cannot be read is a fatal error of the import. The tokens
// after it are still parsed (and looked ahead at): the level of the lexer was
// left raised by the early return of Preprocessor::Lex and the parser asserted
// ("cannot use lookahead while lexing").

// RUN: rm -rf %t && mkdir %t
// RUN: echo "not a module file" > %t/bad.pcm
// RUN: not %clang_cc1 -std=c++20 -fmodule-file=M=%t/bad.pcm -fsyntax-only %s 2>&1 | FileCheck %s

import M
f @
// CHECK: fatal error: file '{{.*}}bad.pcm' is not a valid module file
// CHECK-NOT: Assertion
