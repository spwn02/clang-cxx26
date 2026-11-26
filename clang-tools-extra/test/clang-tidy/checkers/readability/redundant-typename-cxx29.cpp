// RUN: %check_clang_tidy -std=c++29 %s readability-redundant-typename %t

template <class T> concept Any = true;

template <template <class> concept... CC> struct Constrained {
  template <CC...[0] T> struct First {};
};

static_assert(requires { typename Constrained<Any>::First<int>; });
// CHECK-MESSAGES-NOT: warning: redundant 'typename'

struct Concrete { using type = int; };
typename Concrete::type value = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: redundant 'typename' [readability-redundant-typename]
// CHECK-FIXES: Concrete::type value = 0;
