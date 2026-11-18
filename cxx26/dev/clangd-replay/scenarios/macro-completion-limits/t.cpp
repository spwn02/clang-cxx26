#include <__meta/token_locations.h>
#include <array>
#include <string>
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

__macro candidate_limit(token_sequence ts) {
  if (mx::__is_completion(ts[0])) {
    for (unsigned i = 0; i < 130; ++i) {
      if (i == 127) {
        std::string label = std::string("hx_") + "127";
        (void)mx::__complete(ts[0], label, "insert");
      } else {
        char label[] = {'h', 'x', '_', '0', '0', '0', '\0'};
        label[3] = char('0' + (i / 100));
        label[4] = char('0' + ((i / 10) % 10));
        label[5] = char('0' + (i % 10));
        (void)mx::__complete(ts[0], std::string_view(label, 6), "insert");
      }
    }
  }
  return ^^{ };
}
__macro truncation(token_sequence ts) {
  if (mx::__is_completion(ts[0]))
    (void)mx::__complete(ts[0], std::string_view(long_label.data(), 300), "long");
  return ^^{ };
}
void site() {
  candidate_limit!(hx_12);
  truncation!(hx_);
}
