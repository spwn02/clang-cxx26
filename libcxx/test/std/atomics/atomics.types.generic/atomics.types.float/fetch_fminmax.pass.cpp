// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
#include <atomic>
#include <cassert>
#include <cmath>
#include <limits>

template <class A>
void test(A& a) {
  using T = typename A::value_type;
  const T nan = std::numeric_limits<T>::quiet_NaN();
  assert(a.fetch_fmaximum(T(2)) == T(1)); assert(a.load() == T(2));
  assert(a.fetch_fminimum(T(1)) == T(2)); assert(a.load() == T(1));
  a.store(T(1)); assert(a.fetch_fmaximum(nan) == T(1)); assert(std::isnan(a.load()));
  a.store(nan); assert(std::isnan(a.fetch_fminimum(T(1)))); assert(std::isnan(a.load()));
  a.store(nan); assert(std::isnan(a.fetch_fmaximum_num(nan))); assert(std::isnan(a.load()));
  a.store(nan); assert(std::isnan(a.fetch_fminimum_num(nan))); assert(std::isnan(a.load()));
  a.store(nan); assert(std::isnan(a.fetch_fmaximum_num(T(2)))); assert(a.load() == T(2));
  a.store(nan); assert(std::isnan(a.fetch_fminimum_num(T(2)))); assert(a.load() == T(2));
  a.store(T(1)); a.store_fmaximum(T(3)); assert(a.load() == T(3));
  a.store_fminimum(T(2)); assert(a.load() == T(2));
  a.store_fmaximum_num(nan); assert(a.load() == T(2));
  a.store_fminimum_num(nan); assert(a.load() == T(2));
}
int main(int, char**) { std::atomic<float> a(1); test(a); float x = 1; std::atomic_ref<float> r(x); test(r); }
