// RUN: %clang_cc1 -std=c++26 -freflection -verify %s
// RUN: %clang_cc1 -std=c++26 -freflection -emit-pch -o %t.pch %s
// RUN: %clang_cc1 -std=c++26 -freflection -include-pch %t.pch -verify %s
// expected-no-diagnostics

#ifndef HEADER
#define HEADER
using info = decltype(^^int);

// The raw scope metafunctions back current_function, current_class,
// current_namespace and access_context::current in <meta>.
[[clang::instantiation_dependent]] consteval info scope() { return __metafunction(123); }
[[clang::instantiation_dependent]] consteval info function_scope() { return __metafunction(124); }
[[clang::instantiation_dependent]] consteval info class_scope() { return __metafunction(125); }
[[clang::instantiation_dependent]] consteval info namespace_scope() { return __metafunction(126); }
#define CHECK_SCOPE(S)                                                        \
  static_assert(scope() == S);                                                \
  static_assert(function_scope() == S);                                       \
  static_assert(class_scope() == S);                                          \
  static_assert(namespace_scope() == S)

consteval { CHECK_SCOPE(^^::); }
namespace N {
consteval { CHECK_SCOPE(^^N); }
namespace Inner {
consteval { CHECK_SCOPE(^^Inner); consteval { CHECK_SCOPE(^^Inner); } }
struct C {
  consteval { CHECK_SCOPE(^^C); }
  void member() { consteval { CHECK_SCOPE(^^member); } }
};
void function() { { consteval { CHECK_SCOPE(^^function); } } }
template<class T> struct CT {
  consteval { CHECK_SCOPE(^^CT); }
  void member() { consteval { CHECK_SCOPE(^^member); } }
};
template<class T> void ft() {
  consteval { CHECK_SCOPE(^^ft<T>); }
}
consteval {
  [] {
    static constexpr info ordinary_lambda = function_scope();
    consteval {
      CHECK_SCOPE(ordinary_lambda);
      consteval { CHECK_SCOPE(ordinary_lambda); }
    }
  }();
}
}
}
#else
// Instantiate after reading the PCH: synthesized closure identity must survive
// serialization and template transformation.
N::Inner::CT<int> instance;
template void N::Inner::CT<int>::member();
template void N::Inner::ft<int>();
#endif
