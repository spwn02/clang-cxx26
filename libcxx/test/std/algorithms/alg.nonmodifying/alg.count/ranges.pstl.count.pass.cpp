// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: libcpp-has-no-incomplete-pstl
#include <algorithm>
#include <array>
#include <cassert>
#include <execution>
struct X { int v; };
template<class P> void test(P p) { std::array<X, 4> a{{{1},{2},{2},{4}}}; assert(std::ranges::count(p,a.begin(),a.end(),2,&X::v)==2); assert(std::ranges::count(p,a,4,&X::v)==1); }
int main(int,char**) { test(std::execution::seq); test(std::execution::par); test(std::execution::par_unseq); test(std::execution::unseq); }
