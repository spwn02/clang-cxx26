// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

namespace std::contracts::controls {
inline namespace __p4451 {
struct __enforced_t {};
inline constexpr __enforced_t enforced{};
}
}

namespace controls = std::contracts::controls;
namespace c = std::contracts::controls;
using namespace std::contracts::controls;

void qualified(int x) pre<std::contracts::controls::enforced>(x > 0);
void aliased(int x) pre<controls::enforced>(x > 0);
void namespace_alias(int x) pre<c::enforced>(x > 0);
void unqualified(int x) pre<enforced>(x > 0);
int post_result(const int x) post<controls::enforced>(r : r > x) { return x + 1; }
void assertion(int x) { contract_assert<controls::enforced>(x > 0); }

namespace using_declaration {
using std::contracts::controls::enforced;
void accepted() pre<enforced>(true);
}

namespace copied_entities {
constexpr auto& enforced_ref = std::contracts::controls::enforced;
constexpr auto enforced_copy = std::contracts::controls::enforced;
void reference_is_not_the_entity() pre<enforced_ref>(true); // expected-error {{assertion-control id-expression must denote std::contracts::controls::enforced}}
void copy_is_not_the_entity() pre<enforced_copy>(true); // expected-error {{assertion-control id-expression must denote std::contracts::controls::enforced}}
}

template <class T> void dependent() pre<T::label>(true); // expected-error {{assertion-control id-expression must not be type- or value-dependent}}

namespace wrong {
inline constexpr int enforced = 0;
}
void wrong_entity() pre<wrong::enforced>(true); // expected-error {{assertion-control id-expression must denote std::contracts::controls::enforced}}

namespace user_defined {
inline constexpr int enforced = 0;
}
void different_type_in_another_namespace() pre<user_defined::enforced>(true); // expected-error {{assertion-control id-expression must denote std::contracts::controls::enforced}}

void changed(bool) pre<std::contracts::controls::enforced>(true); // expected-note {{contract previously specified with a different assertion-control-specifier}}
void changed(bool) pre(true); // expected-error {{differs in contract specifier sequence}} expected-note {{in contract specified here}}
void changed_back(bool) pre(true); // expected-note {{contract previously specified with a different assertion-control-specifier}}
void changed_back(bool) pre<std::contracts::controls::enforced>(true); // expected-error {{differs in contract specifier sequence}} expected-note {{in contract specified here}}

template <class T> struct Member {
  void f(int x) pre<std::contracts::controls::enforced>(x > 0);
};

struct Redeclared {
  void f(int x) pre<std::contracts::controls::enforced>(x > 0); // expected-note {{contract previously specified with a different assertion-control-specifier}}
};
void Redeclared::f(int x) pre(x > 0) {} // expected-error {{differs in contract specifier sequence}} expected-note {{in contract specified here}}
