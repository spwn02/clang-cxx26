// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

extern "C" {
  template<class T> struct ClassTemplate {};
  template<class T> int functionTemplate(T value) { return value; }
  template<class T> int variableTemplate;
  template<class T> using AliasTemplate = T;

  template<> struct ClassTemplate<int> { int member; };
  template<class T> struct ClassTemplate<T *> {};
  template<> int functionTemplate<int>(int value) { return value; }
  template<> int variableTemplate<int>;
}

extern "C" template<class T> int singleDeclaration(T value) {
  return value;
}

extern "C" {
  template<class T> int blockDeclaration(T value) { return value; }
}

template<class T> extern "C" void invalidSingle(T); // expected-error {{a linkage-specification cannot appear within a template declaration}}
template<class T> extern "C" { void invalidBlock(T); } // expected-error {{a linkage-specification cannot appear within a template declaration}}

extern "C" {
  template<class T> int distinct(T) { return 0; }
}
template<> int distinct<int>(int) { return 0; }
template<> int distinct<char>(char) { return 0; }


// Redeclarations and partial specializations of class templates in the block, a function template that overloads a
// non-template function of the same name (a template is not a function with C language linkage that is identified
// by name), and no -Wreturn-type-c-linkage warning for a template returning a dependent type.
extern "C" {
  template<class T> struct Redeclared;
  template<class T> struct Redeclared { T value; };
  template<class T> struct Redeclared<T *> { int pointer; };
  template<class T> T returnsDependent(T value);
  template<class T> T returnsDependent(T value) { return value; }
  extern int overloaded();
  template<class T> int overloaded(T);
  template<class T> int variableTemplate2 = 1;
}
namespace Different {
  extern "C" { template<class T> struct SameName { int member; }; }
}
namespace Other {
  extern "C" { template<class T> struct SameName { int member; }; }
}
int use() {
  Redeclared<int *> pointer;
  Redeclared<char> value;
  return returnsDependent(1) + overloaded(2) + variableTemplate2<int> + sizeof(pointer) + sizeof(value) +
         sizeof(Different::SameName<int>) + sizeof(Other::SameName<int>);
}
