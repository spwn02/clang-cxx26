#include <meta>
#include <utility>
#include <vector>
constexpr auto arr_0 = std::define_static_array(std::vector<std::pair<int, std::meta::info>>{{42, ^^int}});
constexpr auto arr_1 = std::define_static_array(std::vector<std::pair<int, std::meta::info>>{{42, ^^int}});
int main() {}
