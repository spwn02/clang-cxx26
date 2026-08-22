import std;

auto main() -> int {
  const std::vector<int> values{1, 2, 3};
  std::println("clang-p2996 reference toolchain: {} values", values.size());
  return values.size() == 3 ? 0 : 1;
}
