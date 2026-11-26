// RUN: %check_clang_tidy -std=c++29 %s readability-identifier-naming %t -- \
// RUN:   -config='{CheckOptions: {readability-identifier-naming.FunctionCase: camelBack}}'

namespace std::meta {
using info = decltype(^^::);
class token_sequence {
public:
  constexpr token_sequence() noexcept = default;
};
}

struct Annotation {
  consteval auto inject_members(std::meta::info) const -> std::meta::token_sequence {
    return ^^{ int injected_name = 1; };
  }
};
struct [[=Annotation{}]] Subject {};

int useInjectedMember(Subject value) { return value.injected_name; }
// CHECK-MESSAGES-NOT: warning: invalid case style

int ordinary_name() { return 0; }
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: invalid case style for function 'ordinary_name' [readability-identifier-naming]
// CHECK-FIXES: int ordinaryName() { return 0; }
