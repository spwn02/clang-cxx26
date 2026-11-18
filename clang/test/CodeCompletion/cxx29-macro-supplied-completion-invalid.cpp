// RUN: not %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -Xclang -code-completion-at=%s:17:29 %s 2>&1 | FileCheck %s

#include <__meta/token_locations.h>
#include <string_view>

namespace mx = std::meta::__x;
using std::meta::token_sequence;

// An unreadable completion string is diagnosed at the completion token and
// names the argument.
__macro bad_label(token_sequence raw) {
  for (std::meta::token t : raw)
    if (mx::__is_completion(t))
      (void)mx::__complete(t, std::string_view(nullptr, 1), "ins");
  return ^^{ };
}
void f() { bad_label!(<div x>); }

// CHECK: cxx29-macro-supplied-completion-invalid.cpp:17:28: error: macro completion label argument is not a constant string
