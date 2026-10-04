// RUN: %clang_cc1 -std=c++26 -fcontracts -freflection -fsyntax-only -verify %s

// [expr.prim.id.unqual]: "a variable declared outside of C of object type T"
// ... "then the type of the expression is const T".
int global;
thread_local int thread;
namespace N { int qualified; }
void storage() pre(++global) // expected-error {{considered 'const'}}
  pre(++thread) // expected-error {{considered 'const'}}
  // [expr.prim.id.qual]: "If Q appears in the predicate of a contract
  // assertion C" ... "a variable declared outside of C of object type T"
  // ... "then the type of the expression is const T".
  pre(++N::qualified) {} // expected-error {{considered 'const'}}
void static_storage() {
  static int local;
  // [expr.prim.id.unqual]: "a variable declared outside of C of object type T".
  contract_assert(++local); // expected-error {{considered 'const'}}
  contract_assert([] { static int inside; return ++inside; }()); // OK: inside C
  contract_assert([] { return ++global; }()); // expected-error {{considered 'const'}}
}

// [expr.prim.id.unqual]: "a variable or template parameter declared outside
// of C of type 'reference to T'" ... "the type of the expression is const T".
int &reference = global;
void global_reference() pre(++reference); // expected-error {{considered 'const'}}
template<int &R> void reference_parameter() pre(++R); // expected-error {{considered 'const'}}

struct Aggregate { int value; };
struct Tuple { int value; template<unsigned> int &get() { return value; } };
namespace std {
template<class> struct tuple_size;
template<> struct tuple_size<Tuple> { static constexpr unsigned value = 1; };
template<unsigned, class> struct tuple_element;
template<> struct tuple_element<0, Tuple> { using type = int; };
}
void bindings() {
  int array[1]{};
  Aggregate aggregate{};
  Tuple tuple{};
  auto &[a] = array;
  auto &[b] = aggregate;
  auto &[c] = tuple;
  // [expr.prim.id.unqual]: "a structured binding of type T whose
  // corresponding variable is declared outside of C" ... "const T".
  contract_assert(++a); // expected-error {{considered 'const'}}
  contract_assert(++b); // expected-error {{considered 'const'}}
  contract_assert(++c); // expected-error {{considered 'const'}}
  contract_assert([] { int a[1]{}; auto &[inside] = a; return ++inside; }()); // OK: inside C
}

// [expr.prim.id.unqual]: "a structured binding of type T whose
// corresponding variable is declared outside of C" (static storage too).
Aggregate global_aggregate{};
auto &[global_binding] = global_aggregate;
void global_binding_test() pre(++global_binding); // expected-error {{considered 'const'}}

void splices() {
  int local = 0;
  // [expr.prim.splice]: "If E appears in the predicate of a contract assertion
  // C and S is ... a variable declared outside of C of object type T ...
  // then the type of E is const T".
  contract_assert(++[:^^local:]); // expected-error {{}} expected-note {{within contract context introduced here}}
  contract_assert(++[:^^global:]); // expected-error {{}} expected-note {{within contract context introduced here}}
  contract_assert(++[:^^reference:]); // expected-error {{}} expected-note {{within contract context introduced here}}
  // [expr.prim.splice]: "a structured binding of type T whose corresponding
  // variable is declared outside of C" ... "the type of E is const T".
  contract_assert(++[:^^global_binding:]); // expected-error {{}} expected-note {{within contract context introduced here}}
  contract_assert([] { int inside = 0; return ++[:^^inside:]; }()); // OK: inside C
}

// [expr.prim.id.unqual] Y example: "++i; // error: attempting to modify const
// lvalue", "++j; // OK", "++k; // OK" in the nested reference capture.
void nested(int i) pre([&i] {
  int j = 17;
  [&] { int k = 34; ++i; ++j; ++k; }(); // expected-error {{}}
  return true;
}());

// [expr.prim.id.unqual] Y example: "++this->z; // OK, captured *this"
// and "++z; // OK, captured *this".
struct Copy {
  int z;
  void f() pre([*this]() mutable { ++this->z; ++z; return true; }());
  // [expr.prim.id.unqual] Y example: the mutable copied entity is inside C;
  // reference capture by a further lambda still denotes that copied entity.
  void nested_copy() pre([*this]() mutable {
    [&] { ++this->z; ++z; }();
    return true;
  }());
  // [expr.prim.this]: "including in the bodies of nested lambda-expressions".
  // Without a copy, the original object remains const.
  void original() pre([this] { return ++z; }()); // expected-error {{considered 'const'}}
};
