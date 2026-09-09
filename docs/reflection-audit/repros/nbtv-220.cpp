#include <meta>
struct s_undeducible { auto operator()(); };
consteval { static_assert(false, std::meta::display_string_of(std::meta::type_of(^^s_undeducible::operator()))); }
