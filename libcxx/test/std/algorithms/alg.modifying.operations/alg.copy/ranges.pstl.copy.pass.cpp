// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: libcpp-has-no-incomplete-pstl
#include <algorithm>
#include <array>
#include <cassert>
#include <execution>
template<class P> void test(P p) { std::array<int,4> a{1,2,3,4}, b{}; auto r=std::ranges::copy(p,a.begin(),a.end(),b.begin()); assert(r.in==a.end() && r.out==b.end()); auto q=std::ranges::copy(p,a,b.begin()); assert(q.in==a.end() && q.out==b.end()); }
int main(int,char**) { test(std::execution::seq); test(std::execution::par); test(std::execution::par_unseq); test(std::execution::unseq); }
