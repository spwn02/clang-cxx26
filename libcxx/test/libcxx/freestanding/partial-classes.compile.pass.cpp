//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -ffreestanding
//===----------------------------------------------------------------------===//

#include <array>
#include <expected>
#include <optional>
#include <span>
#include <version>

// A `requires` expression on a concrete (non-dependent) type is evaluated
// immediately rather than treated as a SFINAE context, so an absent member
// is a hard error rather than `false`. Route each check through a template
// so the check itself is properly SFINAE-friendly.
template <class T>
concept has_at = requires(T& value) { value.at(0); };
template <class T>
concept has_value = requires(T& value) { value.value(); };

static_assert(!has_at<std::array<int, 1>>);
static_assert(!has_at<std::span<int>>);
static_assert(!has_value<std::optional<int>>);
static_assert(!has_value<std::expected<int, int>>);
static_assert(__cpp_lib_freestanding_expected == 202311L);
static_assert(__cpp_lib_freestanding_optional == 202311L);

void test_freestanding_partial_classes() {
  std::array<int, 1> array{42};
  std::span<int> span{array};
  (void)array[0];
  (void)span[0];
  std::optional<int> optional{42};
  (void)*optional;
  std::expected<int, int> expected{42};
  (void)*expected;
}
