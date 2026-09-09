#include <meta>
struct A { protected: static void protected_virtual_function(); };
struct B : A { static_assert(std::meta::is_protected(^^A::protected_virtual_function)); };
