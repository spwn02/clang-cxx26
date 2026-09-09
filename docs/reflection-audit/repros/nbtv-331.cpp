#include <meta>
constexpr struct A { std::meta::info _; A() = default; A(const A&) = default; } a {};
consteval { std::meta::reflect_object(a); }
int main() {}
