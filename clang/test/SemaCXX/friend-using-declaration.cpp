// RUN: %clang_cc1 -std=c++11 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// CWG386: the lookup for a qualified friend declarator or a template-id finds
// the entity introduced by a using-declaration; its target scope is the scope
// of that entity.
namespace Original {
template<class T> void destroy(T *p) { delete p; }
template<class T> void tf(T); // #tf
void function();
void over(int);
void over(char);
}

namespace Imported {
using Original::destroy;
using Original::tf; // #using-tf
using Original::function;
using Original::over;

class Secret {
  ~Secret() {}
  friend void destroy<>(Secret *);
  friend void Imported::function();
  friend void Imported::over(int);
  // An unqualified declarator is not looked up: it declares a function of the
  // innermost enclosing namespace, which conflicts with the using-declaration.
  template<class T> friend void tf(T); // expected-error {{cannot befriend target of using declaration}}
  // expected-note@#tf {{target of using declaration}}
  // expected-note@#using-tf {{using declaration}}
};

template<class T> class Tmpl {
  friend void Imported::function();
  template<class U> friend void Imported::tf(U);
};

struct Plain {
  // The declarator corresponds to no template found by the lookup.
  template<class U> friend void Imported::tf(U *); // expected-error {{friend declaration of 'tf' does not match any declaration in namespace 'Imported'}}
};

void use(Secret *p) { Original::destroy(p); }
}
