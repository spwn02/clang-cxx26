// RUN: %clang_cc1 -std=c++26 -freflection -verify %s
// [expr.prim.id.qual] example.
template<int V> struct TCls { static constexpr int s = V; using type = int; };
int v1 = [:^^TCls<1>:]::s;
int v2 = template [:^^TCls:]<2>::s;
typename [:^^TCls:]<3>::type v3 = 3;
template [:^^TCls:]<3>::type v4 = 4;
typename template [:^^TCls:]<3>::type v5 = 5;
[:^^TCls:]<3>::type v6 = 6; // expected-error {{expected template}} expected-error {{expected ';' after top level declarator}}
