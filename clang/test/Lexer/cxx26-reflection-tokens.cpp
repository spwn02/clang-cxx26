// RUN: %clang_cc1 -std=c++26 -freflection -dump-tokens %s 2>&1 | FileCheck %s --check-prefix=REFLECTION
// RUN: %clang_cc1 -std=c++26 -dump-tokens %s 2>&1 | FileCheck %s --check-prefix=DISABLED

[:x:] ^^ __metafunction

// REFLECTION: l_splice '[:'
// REFLECTION: r_splice ':]'
// REFLECTION: caretcaret '^^'
// REFLECTION: __metafunction '__metafunction'

// DISABLED-NOT: warning:
// DISABLED: l_square '['
// DISABLED: colon ':'
// DISABLED: identifier 'x'
// DISABLED: colon ':'
// DISABLED: r_square ']'
