// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++29

#include <__meta/token_locations.h>
#include <meta>

void __test() {
  (void)std::meta::__x::__error(0, "not in C++26"); // expected-error {{no member named '__x' in namespace 'std::meta'}}
}
