#include <meta>
template<class T> consteval auto f() { return std::meta::display_string_of(std::meta::type_of(^^T)); }
static_assert(f<int>() == "int");
