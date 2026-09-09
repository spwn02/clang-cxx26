#include <meta>
constexpr struct { std::meta::info _; consteval auto f() const {} } a {};
consteval { std::meta::reflect_constant(&a); }
int main() {}
