//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03
// RUN: %{cxx} %{flags} %{compile_flags} -ffreestanding -fsyntax-only %s
//===----------------------------------------------------------------------===//

#include <array>
#include <span>

static_assert(!requires(std::array<int, 1>& value) { value.at(0); });
static_assert(!requires(std::span<int>& value) { value.at(0); });

void test_freestanding_partial_classes() {
  std::array<int, 1> array{42};
  std::span<int> span{array};
  (void)array[0];
  (void)span[0];
}
