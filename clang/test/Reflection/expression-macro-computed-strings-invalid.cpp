// RUN: not %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -fcxx-exceptions -Wno-nonnull %s 2>&1 | FileCheck %s

#include <__meta/token_locations.h>
#include <string_view>

namespace mx = std::meta::__x;
using std::meta::token_sequence;

__macro invalid_message(token_sequence ts) {
  (void)mx::__error(ts[0], std::string_view(nullptr, 1));
  return ^^{ 0 };
}
int result = invalid_message!(invalid_token);

// CHECK: {{.*}}token_locations.h:{{[0-9]+}}:{{[0-9]+}}: error: macro diagnostic message argument is not a constant string
