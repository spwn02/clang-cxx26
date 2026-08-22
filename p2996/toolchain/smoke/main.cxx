import std;

static_assert(
    std::meta::reflection_range<std::vector<std::meta::info>&>);
static_assert(std::meta::is_integral_type(^^int));
static_assert(std::meta::is_reference_type(^^int &));
static_assert(std::meta::is_same_type(^^int, ^^int));

auto main() -> int {
  const std::vector<int> values{1, 2, 3};
  std::inplace_vector<int, 4> inplace{1, 2, 3};
  std::hive<int> hive;
  hive.insert(42);

  std::println("clang-p2996 reference toolchain: {} values", values.size());
  return values.size() == 3 &&
                 inplace.size() == 3 &&
                 hive.size() == 1 &&
                 *hive.begin() == 42
             ? 0
             : 1;
}
