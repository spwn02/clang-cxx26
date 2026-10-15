// RUN: rm -rf %t && mkdir %t
// RUN: touch %t/g %t/h %t/i %t/j
// RUN: not %clang_cc1 -std=c++26 -I %t -dump-tokens %s 2>&1 | FileCheck %s

// [lex.pptoken]/[cpp.import]: a header-name is formed immediately after an import
// preprocessing token that is at the start of a logical source line, with no
// intervening preprocessing token on the logical line.

#define EMPTY

// CHECK-NOT: header_name '<e>'
EMPTY import <e>;

// CHECK-NOT: header_name '<f>'
import
<f>;

// CHECK: header_name '<g>'
import /* c */ <g>;

// CHECK: header_name '<h>'
import /* multi
line */ <h>;

// CHECK: header_name '<i>'
export import <i>;

// CHECK: header_name '<j>'
import \
<j>;

// CHECK-NOT: header_name '<b>'
a import <b>;

// CHECK-NOT: header_name '<x>'
export EMPTY import <x>;
