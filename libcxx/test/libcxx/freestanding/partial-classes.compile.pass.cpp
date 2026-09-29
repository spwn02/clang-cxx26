//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -ffreestanding -fno-exceptions -D_LIBCPP_HAS_NO_THREADS
//===----------------------------------------------------------------------===//

#include <array>
#include <expected>
#include <optional>
#include <span>
#include <string_view>
#include <version>

// A `requires` expression on a concrete (non-dependent) type is evaluated
// immediately rather than treated as a SFINAE context, so an absent member
// is a hard error rather than `false`. Route each check through a template
// so the check itself is properly SFINAE-friendly.
template <class T>
concept has_at = requires(T& value) { value.at(0); };
template <class T>
concept has_value = requires(T& value) { value.value(); };
template <class T>
concept has_string_view_at = requires(T& value) { value.at(0); };
template <class T>
concept has_string_view_copy = requires(T& value, char* output) { value.copy(output, 1); };
template <class T>
concept has_string_view_substr = requires(T& value) { value.substr(0, 1); };
template <class T>
concept has_string_view_pos_compare = requires(T& value) { value.compare(0, 1, std::string_view{}); };
template <class T>
concept has_string_view_pos_compare_with_range =
    requires(T& value) { value.compare(0, 1, std::string_view{}, 0, 1); };
template <class T>
concept has_string_view_pos_compare_c_string = requires(T& value) { value.compare(0, 1, "x"); };
template <class T>
concept has_string_view_pos_compare_c_string_with_length = requires(T& value) { value.compare(0, 1, "x", 1); };

static_assert(!has_at<std::array<int, 1>>);
static_assert(!has_at<std::span<int>>);
static_assert(!has_value<std::optional<int>>);
static_assert(!has_value<std::expected<int, int>>);
static_assert(!has_string_view_at<std::string_view>);
static_assert(!has_string_view_copy<std::string_view>);
static_assert(!has_string_view_substr<std::string_view>);
static_assert(!has_string_view_pos_compare<std::string_view>);
static_assert(!has_string_view_pos_compare_with_range<std::string_view>);
static_assert(!has_string_view_pos_compare_c_string<std::string_view>);
static_assert(!has_string_view_pos_compare_c_string_with_length<std::string_view>);
static_assert(__cpp_lib_freestanding_expected == 202311L);
static_assert(__cpp_lib_freestanding_optional == 202311L);

void test_freestanding_partial_classes() {
  std::array<int, 1> array{42};
  std::span<int> span{array};
  (void)array[0];
  (void)span[0];
  (void)std::string_view("ok").starts_with("o");
  (void)std::string_view("ok").ends_with("k");
  (void)std::string_view("ok").compare("ok");
  std::optional<int> optional{42};
  (void)*optional;
  std::expected<int, int> expected{42};
  (void)*expected;
}
