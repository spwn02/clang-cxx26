#include <meta>
enum E { e };
struct S { static E value; };
E S::value = e;
static_assert(std::meta::type_of(^^S::value) == ^^E);
