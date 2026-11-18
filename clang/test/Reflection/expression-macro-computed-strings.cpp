// RUN: %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -fcxx-exceptions -Xclang -verify -Wmacro-located-diagnostics %s
// RUN: not %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -fcxx-exceptions -Wmacro-located-diagnostics -fdiagnostics-parseable-fixits %s 2>&1 | FileCheck %s --check-prefix=FIXIT
// RUN: not %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -fcxx-exceptions -Wmacro-located-diagnostics %s 2>&1 | FileCheck %s --check-prefix=DIAG
// RUN: not %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -fcxx-exceptions -Rmacro-role -Xclang -code-completion-at=%s:72:39 %s -o - 2>&1 | FileCheck %s --check-prefix=COMPLETE

#include <__meta/token_locations.h>
#include <array>
#include <string>
#include <string_view>

namespace mx = std::meta::__x;
using std::meta::token_sequence;

constexpr std::array<char, 6> messages{'t', 'a', 'b', 'l', 'e', '\0'};

__macro heap_error(token_sequence ts) {
  std::string message = "heap " + std::string("computed message");
  (void)mx::__error(ts[0], message);
  return ^^{ 0 };
}
int e1 = heap_error!(heap_token); // expected-error {{heap computed message}}

__macro array_warning(token_sequence ts) {
  std::string_view message(messages.data(), 4);
  (void)mx::__warning(ts[0], message);
  return ^^{ 0 };
}
int e2 = array_warning!(array_token); // expected-warning {{tabl}}

__macro local_note(token_sequence ts) {
  char message[] = {'l', 'o', 'c', 'a', 'l', '\0'};
  (void)mx::__note(ts[0], std::string_view(message, 5));
  return ^^{ 0 };
}
int e3 = local_note!(local_token); // expected-note {{local}}

__macro concatenated_error(token_sequence ts) {
  std::string message = std::string("left") + " right";
  (void)mx::__error(ts[0], message);
  return ^^{ 0 };
}
int e4 = concatenated_error!(concat_token); // expected-error {{left right}}

__macro replacement(token_sequence ts) {
  std::string text = std::string("computed") + "_replacement";
  (void)mx::__warning(ts[0], "computed fixit",
                      mx::__replace(ts[0], ts[0], text));
  return ^^{ 0 };
}
int e5 = replacement!(replace_token); // expected-warning {{computed fixit}}

__macro insertion(token_sequence ts) {
  std::string text = std::string("computed") + "_insertion";
  (void)mx::__warning(ts[0], "computed insertion",
                      mx::__insert_after(ts[0], text));
  return ^^{ 0 };
}
int e6 = insertion!(insert_token); // expected-warning {{computed insertion}}

__macro complete(token_sequence ts) {
  if (mx::__is_completion(ts[0])) {
    std::string label = std::string("hx_") + "computed";
    char insert[] = {'i', 'n', 's', 'e', 'r', 't', '\0'};
    std::array<char, 7> detail{'d', 'e', 't', 'a', 'i', 'l', '\0'};
    std::string documentation = std::string("doc") + "umentation";
    (void)mx::__complete(ts[0], label, std::string_view(insert, 6),
                         mx::__completion_kind::property,
                         std::string_view(detail.data(), 6), documentation);
  }
  return ^^{ };
}
void completion_site() { complete!(hx_); }

// FIXIT: fix-it:"{{.*}}expression-macro-computed-strings.cpp":{50:{{[0-9]+}}-50:{{[0-9]+}}}:"computed_replacement"
// FIXIT: fix-it:"{{.*}}expression-macro-computed-strings.cpp":{58:{{[0-9]+}}-58:{{[0-9]+}}}:"computed_insertion"
// DIAG: {{.*}}expression-macro-computed-strings.cpp:21:22: error: heap computed message
// COMPLETE: remark: macro completion kind=2 label=hx_computed insert=insert detail=detail documentation=documentation
