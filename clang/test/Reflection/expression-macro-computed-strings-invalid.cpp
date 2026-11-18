// RUN: not %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -fcxx-exceptions -Wno-nonnull %s 2>&1 | FileCheck %s

#include <__meta/token_locations.h>
#include <string_view>

namespace mx = std::meta::__x;
using std::meta::token_sequence;

// A string that cannot be read is diagnosed at the macro author's token and
// names the argument, instead of the generic "did not produce a token sequence".
__macro invalid_message(token_sequence ts) {
  (void)mx::__error(ts[0], std::string_view(nullptr, 1));
  return ^^{ 0 };
}
int result = invalid_message!(invalid_token);

__macro invalid_replacement(token_sequence ts) {
  (void)mx::__error(ts[0], "bad token",
                    mx::__replace(ts[0], ts[0], std::string_view(nullptr, 1)));
  return ^^{ 0 };
}
int result2 = invalid_replacement!(other_token);

// CHECK: expression-macro-computed-strings-invalid.cpp:15:31: error: macro diagnostic message argument is not a constant string
// CHECK: expression-macro-computed-strings-invalid.cpp:22:36: error: macro diagnostic replacement text argument is not a constant string
