#include <meta>
#include <utility>
void f() { template for (constexpr auto index : std::integer_sequence<int, 1, 5>{}) { (void)index; } }
