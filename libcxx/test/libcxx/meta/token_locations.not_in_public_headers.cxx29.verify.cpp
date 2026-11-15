// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26

#include <debugging>
#include <meta>

consteval void __test() {
  (void)std::meta::__x::__token_begin(std::meta::token{}); // expected-error {{no member named '__x' in namespace 'std::meta'}}
}
