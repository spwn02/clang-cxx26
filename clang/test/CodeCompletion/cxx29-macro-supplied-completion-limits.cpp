// RUN: %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -Rmacro-role -Xclang -code-completion-at=%s:46:22 %s -o - 2>&1 | FileCheck %s --check-prefix=DISPLAY
// RUN: %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -Rmacro-role -Xclang -code-completion-at=%s:47:23 %s -o - 2>&1 | FileCheck %s --check-prefix=COUNT

#include <__meta/token_locations.h>
#include <array>
#include <string_view>
namespace mx = std::meta::__x;
using std::meta::token_sequence;

constexpr std::array<char, 301> long_label = [] {
  std::array<char, 301> text{};
  text[0] = 'h'; text[1] = 'x'; text[2] = '_';
  for (unsigned i = 3; i < 256; ++i) text[i] = 'a';
  text[256] = 'Z';
  return text;
}();
constexpr std::array<char, 4098> long_insert = [] {
  std::array<char, 4098> text{};
  for (unsigned i = 0; i < text.size(); ++i) text[i] = 'i';
  return text;
}();

__macro display_limits(token_sequence ts) {
  if (mx::__is_completion(ts[0])) {
    (void)mx::__complete(ts[0], std::string_view(long_label.data(), 300), "ok");
    (void)mx::__complete(ts[0], "hx_dropped",
                         std::string_view(long_insert.data(), long_insert.size()));
    (void)mx::__complete(ts[0], "hx_invalid_kind", "insert",
                         static_cast<mx::__completion_kind>(255));
  }
  return ^^{ };
}
__macro candidate_limit(token_sequence ts) {
  if (mx::__is_completion(ts[0])) {
    for (unsigned i = 0; i < 130; ++i) {
      char label[] = {'h', 'x', '_', '0', '0', '0', '\0'};
      label[3] = char('0' + (i / 100));
      label[4] = char('0' + ((i / 10) % 10));
      label[5] = char('0' + (i % 10));
      (void)mx::__complete(ts[0], std::string_view(label, 6), "insert");
    }
  }
  return ^^{ };
}
void test() {
  display_limits!(hx_);
  candidate_limit!(hx_);
}

// DISPLAY: remark: macro completion kind=0 label=hx_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa insert=ok detail= documentation=
// DISPLAY: remark: macro completion kind=255 label= insert= detail= documentation=
// DISPLAY-NOT: label=hx_dropped
// DISPLAY-NOT: label=hx_invalid_kind
// DISPLAY-NOT: Z
// COUNT-COUNT-128: remark: macro completion kind=0 label=hx_{{[0-9][0-9][0-9]}} insert=insert detail= documentation=
// COUNT-NOT: remark: macro completion kind=0 label=hx_
// COUNT: remark: macro completion kind=255 label= insert= detail= documentation=
