// RUN: %check_clang_tidy -std=c++26 %s misc-unused-using-decls %t
// RUN: %check_clang_tidy -std=c++29 %s misc-unused-using-decls %t

namespace std::contracts::controls {
inline namespace __p4451 {
struct __enforced_t {};
inline constexpr __enforced_t enforced{};
}
}

using std::contracts::controls::enforced;
namespace unused_ns {
int unused;
} // namespace unused_ns
using unused_ns::unused;
// CHECK-MESSAGES: :[[@LINE-1]]:18: warning: using decl 'unused' is unused
// CHECK-MESSAGES-NOT: warning: using decl 'enforced' is unused

void precondition(int x) pre<enforced>(x > 0) {}
void assertion(int x) { contract_assert<enforced>(x > 0); }
int postcondition(int x) post<enforced>(r : r > 0) { return x; }
