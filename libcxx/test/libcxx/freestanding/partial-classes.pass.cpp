//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03
// RUN: %{cxx} %{flags} %{compile_flags} -ffreestanding -fsyntax-only %s
//===----------------------------------------------------------------------===//

#include <array>
#include <expected>
#include <optional>
#include <span>
#include <version>

static_assert(!requires(std::array<int, 1>& value) { value.at(0); });
static_assert(!requires(std::span<int>& value) { value.at(0); });
static_assert(!requires(std::optional<int>& value) { value.value(); });
template<class T> concept has_expected_value = requires(T& value) { value.value(); };
static_assert(!has_expected_value<std::expected<int, int>>);
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
