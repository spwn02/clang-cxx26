import std;

static_assert(
    std::meta::reflection_range<std::vector<std::meta::info>&>);
static_assert(std::meta::is_integral_type(^^int));
static_assert(std::meta::is_reference_type(^^int &));
static_assert(std::meta::is_same_type(^^int, ^^int));

// P1383R2: the transcendental <cmath> functions are constexpr (MPFR-backed
// constant evaluation in the compiler).
static_assert(std::sqrt(4.0) == 2.0);
static_assert(std::sin(0.0) == 0.0);
static_assert(std::exp(0.0) == 1.0);
static_assert(std::sqrtf(9.0f) == 3.0f);
static_assert(std::fabsf(-2.5f) == 2.5f);
static_assert(std::nextup(1.0) > 1.0);

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
