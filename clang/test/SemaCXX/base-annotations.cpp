// RUN: %clang_cc1 -std=c++26 -freflection -fannotation-attributes -fsyntax-only -verify %s

struct B0 {};
struct B1 {};
struct Single : [[=13]] B0 {};
struct Multiple : [[=1, =2, =1]] public virtual B0, [[=3]] private B1 {};
struct Invalid : [[nodiscard]] B0 {}; // expected-error {{'nodiscard' attribute cannot be applied to a base specifier}}

template<int N> struct Dependent : [[=N]] B0 {};
Dependent<3> dependent;
template<class... Bs> struct Expanded : [[=sizeof(Bs)]] Bs... {};
Expanded<B0, B1> expanded;
template<int... Ns> struct AnnotationPack : [[=Ns...]] B0 {};
AnnotationPack<1, 2> annotations;
AnnotationPack<> empty;

template<int... Ns> struct AnnotationOnlyBasePack : [[=Ns]] B0... {};
AnnotationOnlyBasePack<> no_bases;
AnnotationOnlyBasePack<5> one_base;

template<class... Bs> struct Unexpanded : [[=sizeof(Bs)]] B0 {}; // expected-error {{contains unexpanded parameter pack}}

int runtime(); // expected-note {{declared here}}
struct NonConstant : [[=runtime()]] B0 {}; // expected-error {{C++26 annotation attribute requires an expression usable as a template argument}} expected-note {{non-constexpr function}}
template<class T> struct [[=T::value()]] BadDeclaration {}; // expected-error 2 {{C++26 annotation attribute requires an expression usable as a template argument}} expected-note 2 {{non-constexpr function}}
template<class T> struct BadBase : [[=T::value()]] B0 {}; // expected-error {{C++26 annotation attribute requires an expression usable as a template argument}} expected-note {{non-constexpr function}}
struct Runtime { static int value() { return runtime(); } }; // expected-note 3 {{declared here}}
BadDeclaration<Runtime> bad_declaration; // expected-note 2 {{in instantiation of template class}}
BadBase<Runtime> bad_base; // expected-note {{in instantiation of template class}}

class NonStructural {
  int value = 0;
};
template<class T> struct BadStructuralBase : [[=T{}]] B0 {}; // expected-error {{C++26 annotation attribute requires a value of structural type}}
BadStructuralBase<NonStructural> bad_structural_base; // expected-note {{in instantiation of template class}}
