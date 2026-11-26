// ADDITIONAL_COMPILE_FLAGS: -std=c++26

// P4451R2 provides a placeholder with an unspecified distinct class type.
#include <contracts>
#include <type_traits>

using enforced_type = decltype(std::contracts::controls::enforced);
static_assert(std::is_class_v<enforced_type>);
static_assert(std::is_empty_v<enforced_type>);
static_assert(std::is_const_v<decltype(std::contracts::controls::enforced)>);

int main(int, char**) { return 0; }
