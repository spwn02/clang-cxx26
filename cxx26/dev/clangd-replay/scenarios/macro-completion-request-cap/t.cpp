#include <__meta/token_locations.h>
#include <array>
#include <string_view>
namespace mx = std::meta::__x;
using std::meta::token_sequence;
constexpr auto make_labels(char m) {
  std::array<char, 128 * 8> a{};
  for (int i = 0; i < 128; ++i) { a[i*8]='h'; a[i*8+1]='x'; a[i*8+2]=m; a[i*8+3]=char('0'+(i/100)%10); a[i*8+4]=char('0'+(i/10)%10); a[i*8+5]=char('0'+i%10); a[i*8+6]='_'; a[i*8+7]='x'; }
  return a;
}
constexpr auto L0 = make_labels('a');
constexpr auto L1 = make_labels('b');
constexpr auto L2 = make_labels('c');
constexpr auto L3 = make_labels('d');
constexpr auto L4 = make_labels('e');
__macro m0(token_sequence raw) {
  for (std::meta::token t : raw)
    if (mx::__is_completion(t))
      for (int i = 0; i < 128; ++i)
        (void)mx::__complete(t, std::string_view(L0.data() + i * 8, 8), "x");
  return ^^{ m1!(\(raw)) };
}
__macro m1(token_sequence raw) {
  for (std::meta::token t : raw)
    if (mx::__is_completion(t))
      for (int i = 0; i < 128; ++i)
        (void)mx::__complete(t, std::string_view(L1.data() + i * 8, 8), "x");
  return ^^{ m2!(\(raw)) };
}
__macro m2(token_sequence raw) {
  for (std::meta::token t : raw)
    if (mx::__is_completion(t))
      for (int i = 0; i < 128; ++i)
        (void)mx::__complete(t, std::string_view(L2.data() + i * 8, 8), "x");
  return ^^{ m3!(\(raw)) };
}
__macro m3(token_sequence raw) {
  for (std::meta::token t : raw)
    if (mx::__is_completion(t))
      for (int i = 0; i < 128; ++i)
        (void)mx::__complete(t, std::string_view(L3.data() + i * 8, 8), "x");
  return ^^{ m4!(\(raw)) };
}
__macro m4(token_sequence raw) {
  for (std::meta::token t : raw)
    if (mx::__is_completion(t))
      for (int i = 0; i < 128; ++i)
        (void)mx::__complete(t, std::string_view(L4.data() + i * 8, 8), "x");
  return ^^{ 0 };
}
int f() { return m0!(<div x>); }
