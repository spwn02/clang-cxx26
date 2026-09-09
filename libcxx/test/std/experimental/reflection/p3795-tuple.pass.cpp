// UNSUPPORTED: no-reflection
// ADDITIONAL_COMPILE_FLAGS: -freflection

#include <meta>
#include <tuple>

using namespace std::meta;

int add(int, long);
int add_noexcept(int, long) noexcept;

static_assert(is_applicable_type(^^decltype(&add), ^^std::tuple<int, long>));
static_assert(!is_applicable_type(^^decltype(&add), ^^std::tuple<int>));
static_assert(is_nothrow_applicable_type(^^decltype(&add_noexcept), ^^std::tuple<int, long>));
static_assert(!is_nothrow_applicable_type(^^decltype(&add), ^^std::tuple<int, long>));
static_assert(apply_result(^^decltype(&add), ^^std::tuple<int, long>) == ^^int);

int main() {}
