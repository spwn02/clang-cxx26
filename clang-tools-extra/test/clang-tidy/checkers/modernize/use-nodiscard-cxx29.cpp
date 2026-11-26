// RUN: %check_clang_tidy -std=c++29 %s modernize-use-nodiscard %t

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
// CHECK-MESSAGES-NOT: function inject_members should be marked

struct Result {};
struct Ordinary {
  Result result() const { return Result{}; }
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: function 'result' should be marked {{\[\[nodiscard\]\]}} [modernize-use-nodiscard]
  // CHECK-FIXES: {{\[\[nodiscard\]\]}} Result result() const { return Result{}; }
};
