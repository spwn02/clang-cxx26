// UNSUPPORTED: c++03, c++11, c++14
#include <atomic>

struct NonCopy {
  NonCopy() = default;
  NonCopy(const NonCopy&) = delete;
  NonCopy(NonCopy&&) = default;
  NonCopy& operator=(const NonCopy&) = delete;
  NonCopy& operator=(NonCopy&&) = default;
};

std::atomic<NonCopy> value; // expected-error@*:* {{std::atomic<T> requires that 'T' be copy constructible}}
