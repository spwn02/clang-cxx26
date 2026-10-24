// RUN: %clang_cc1 -std=c++20 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++23 -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// RUN: %clang_cc1 -std=c++2c -pedantic-errors %s -verify=expected -fexceptions -fcxx-exceptions
// expected-no-diagnostics

typedef decltype(sizeof(0)) size_t;
inline void *operator new(size_t, void *p) noexcept { return p; }
namespace std {
template<typename T> struct allocator {
  constexpr T *allocate(size_t n) { return static_cast<T *>(::operator new(n * sizeof(T))); }
  constexpr void deallocate(T *p, size_t) { ::operator delete(p); }
};
template<typename T, typename... A> constexpr T *construct_at(T *p, A &&...a) {
  return ::new (static_cast<void *>(p)) T(static_cast<A &&>(a)...);
}
} // namespace std

namespace cwg2750 { // cwg2750: yes
constexpr int f() {
  std::allocator<int> a;
  int *p = a.allocate(1);
  std::construct_at(p);
  int r = *p;
  a.deallocate(p, 1);
  return r;
}
static_assert(f() == 0, "");
} // namespace cwg2750
