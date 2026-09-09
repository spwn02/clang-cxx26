#include <algorithm>
#include <meta>
#include <ranges>
#include <string>
std::string s{};
constexpr auto a11 = std::meta::display_string_of(^^decltype(std::ranges::max_element(s)));
constexpr auto a12 = std::meta::display_string_of(std::meta::dealias(^^decltype(std::ranges::max_element(s))));
std::ranges::owning_view ov(std::move(s));
constexpr auto a21 = std::meta::display_string_of(^^decltype(std::ranges::max_element(ov)));
constexpr auto a22 = std::meta::display_string_of(std::meta::dealias(^^decltype(std::ranges::max_element(ov))));
constexpr auto a23 = std::meta::display_string_of(std::meta::dealias(std::meta::dealias(^^decltype(std::ranges::max_element(ov)))));
using T1 = decltype(std::ranges::max_element(s));
constexpr auto a31 = std::meta::display_string_of(^^T1);
constexpr auto a32 = std::meta::display_string_of(std::meta::dealias(^^T1));
constexpr auto a33 = std::meta::display_string_of(std::meta::dealias(std::meta::dealias(^^T1)));
using T2 = decltype(std::ranges::max_element(ov));
constexpr auto a41 = std::meta::display_string_of(^^T2);
constexpr auto a42 = std::meta::display_string_of(std::meta::dealias(^^T2));
constexpr auto a43 = std::meta::display_string_of(std::meta::dealias(std::meta::dealias(^^T2)));
