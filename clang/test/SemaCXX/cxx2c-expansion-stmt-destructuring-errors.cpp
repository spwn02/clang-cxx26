// RUN: %clang_cc1 -std=c++26 -freflection -fexpansion-statements -verify %s

union U { int value; };
struct Empty {};
struct Private {
private:
  int value; // expected-note 4 {{declared private here}}
};

void invalid() {
  template for (auto x : 5) {} // expected-error {{cannot bind non-class, non-array type 'int'}}
  template for (auto x : U{}) {} // expected-error {{cannot bind union type 'U'}}
  template for (auto x : (void*)0) {} // expected-error {{cannot bind non-class, non-array type 'void *'}}
  template for (auto x : Private{}) {} // expected-error 2 {{cannot bind private member 'value' of 'Private'}}
}

template <class T> void dependent(T value) {
  template for (auto x : value) {} // expected-error {{cannot bind non-class, non-array type 'int'}} expected-error {{cannot bind union type 'U'}} expected-error {{cannot bind non-class, non-array type 'void *'}} expected-error 2 {{cannot bind private member 'value' of 'Private'}}
}
void instantiate() {
  dependent(5); // expected-note {{in instantiation of function template specialization 'dependent<int>' requested here}}
  dependent(U{}); // expected-note {{in instantiation of function template specialization 'dependent<U>' requested here}}
  dependent((void*)0); // expected-note {{in instantiation of function template specialization 'dependent<void *>' requested here}}
  dependent(Private{}); // expected-note {{in instantiation of function template specialization 'dependent<Private>' requested here}}
}

// Zero is a valid structured binding size, including the tuple-like case.
namespace std {
template <class> struct tuple_size;
template <> struct tuple_size<Empty> { static constexpr int value = 0; };
}
struct EmptyMembers {};
constexpr bool zero() {
  template for (auto x : EmptyMembers{}) { return false; }
  template for (auto x : Empty{}) { return false; }
  return true;
}
static_assert(zero());
template <class T> constexpr bool zero_template() {
  template for (auto x : T{}) { return false; }
  return true;
}
static_assert(zero_template<Empty>());
static_assert(zero_template<EmptyMembers>());

// Non-public members can still be decomposed from an authorized access context.
class Accessible {
  int value = 3;
public:
  constexpr int read() const {
    int result = 0;
    template for (auto x : *this) { result += x; }
    return result;
  }
};
static_assert(Accessible{}.read() == 3);
