import std;

static_assert(
    std::meta::reflection_range<std::vector<std::meta::info>&>);
static_assert(std::meta::is_integral_type(^^int));
static_assert(std::meta::is_reference_type(^^int &));
static_assert(std::meta::is_same_type(^^int, ^^int));

// Contracts (P2900R14) are on by default in this toolchain file
// (docs/CONTRACTS_HARDENING.md M8); check that a real consumer sees a
// correct result value in a postcondition -- this is exactly the bug M3
// fixed (the result-name `r` used to read uninitialized memory for any
// scalar return type).
auto square(const int x) -> int
    post(r : r == x * x) {
  return x * x;
}

auto main() -> int {
  const std::vector<int> values{1, 2, 3};
  std::inplace_vector<int, 4> inplace{1, 2, 3};
  std::hive<int> hive;
  hive.insert(42);

  std::println("clang-cxx26 reference toolchain: {} values", values.size());
  return values.size() == 3 &&
                 inplace.size() == 3 &&
                 hive.size() == 1 &&
                 *hive.begin() == 42 &&
                 square(7) == 49
             ? 0
             : 1;
}
