// RUN: %check_clang_tidy -std=c++29 %s readability-convert-member-functions-to-static %t

namespace std::meta {
using info = decltype(^^::);
class token_sequence {
public:
  constexpr token_sequence() noexcept = default;
};
}

struct Annotation {
  consteval auto inject_members(std::meta::info) const -> std::meta::token_sequence {
    return ^^{ int added = 1; };
  }
};
struct [[=Annotation{}]] Subject {};
// CHECK-MESSAGES-NOT: method inject_members can be made static

struct Ordinary {
  int f() const { return 1; }
  // CHECK-MESSAGES: :[[@LINE-1]]:7: warning: method 'f' can be made static [readability-convert-member-functions-to-static]
  // CHECK-FIXES: static int f() { return 1; }
};
