#include <meta>
template<typename ...Types> struct S {
  struct inside_S;
  consteval { std::meta::define_aggregate(^^inside_S, { std::meta::data_member_spec(^^int, {.name="a"}), std::meta::data_member_spec(^^char, {.name="b"}), std::meta::data_member_spec(^^double, {.name="c"}) }); }
};
using for_template = S<int,float,double>;
template<for_template V> struct X {};
X<for_template{}> x;
