// Smoke test of the cxx29 reference toolchain: libc++ in C++29 mode and a do expression (P2806).
#include <print>
#include <vector>

int main() {
  std::vector<int> values{1, 2, 3};
  int sum = do {
    int total = 0;
    for (int value : values)
      total += value;
    do_return total;
  };
  std::println("clang-cxx29 reference toolchain: {} values, sum {}", values.size(), sum);
  return sum == 6 ? 0 : 1;
}
