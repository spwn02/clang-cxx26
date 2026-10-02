//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

#ifdef BASE_ANNOTATIONS_IMPORT_STD
import std;
#else
#include <meta>
#endif

using namespace std::meta;

struct B0 {};
struct B1 {};
struct B2 {};
struct Tag { int value; };

struct Single : [[=13]] B0 {};
struct Multiple : [[=1, =Tag{2}]] [[=3, =1]] B0 {};
struct Access : [[=4]] virtual public B0, [[=5]] protected virtual B1,
                [[=6]] private B2 {};
struct Unannotated : B0 {};

consteval info base(info derived, unsigned index = 0) {
  return bases_of(derived, access_context::unchecked())[index];
}

static_assert(annotations_of(base(^^Single)).size() == 1);
static_assert(is_annotation(annotations_of(base(^^Single))[0]));
static_assert([:constant_of(annotations_of(base(^^Single))[0]):] == 13);
static_assert(annotations_of(base(^^Unannotated)).empty());

consteval bool check_order() {
  auto annotations = annotations_of(base(^^Multiple));
  if (annotations.size() != 4)
    return false;
  if (extract<int>(annotations[0]) != 1 || extract<Tag>(annotations[1]).value != 2 ||
      extract<int>(annotations[2]) != 3 || extract<int>(annotations[3]) != 1)
    return false;
  // Each annotation produces a unique reflection even with equal constants.
  if (annotations[0] == annotations[3] || constant_of(annotations[0]) != constant_of(annotations[3]))
    return false;
  auto integers = annotations_of_with_type(base(^^Multiple), ^^const int);
  return integers.size() == 3 && integers[0] == annotations[0] &&
         integers[1] == annotations[2] && integers[2] == annotations[3] &&
         annotations_of_with_type(base(^^Multiple), ^^Tag).size() == 1 &&
         annotations_of_with_type(base(^^Multiple), ^^float).empty();
}
static_assert(check_order());
static_assert(extract<int>(annotations_of(base(^^Access, 0))[0]) == 4);
static_assert(extract<int>(annotations_of(base(^^Access, 1))[0]) == 5);
static_assert(extract<int>(annotations_of(base(^^Access, 2))[0]) == 6);

template<int N> struct Dependent : [[=N]] B0 {};
static_assert(extract<int>(annotations_of(base(^^Dependent<3>))[0]) == 3);
static_assert(extract<int>(annotations_of(base(^^Dependent<7>))[0]) == 7);
static_assert(annotations_of(base(^^Dependent<3>))[0] != annotations_of(base(^^Dependent<7>))[0]);

template<class T> struct SameConstant : [[=13]] B0 {};
static_assert(annotations_of(base(^^SameConstant<int>))[0] !=
              annotations_of(base(^^SameConstant<char>))[0]);
static_assert(constant_of(annotations_of(base(^^SameConstant<int>))[0]) ==
              constant_of(annotations_of(base(^^SameConstant<char>))[0]));

template<class... Bs> struct Expanded : [[=1]] Bs... {};
static_assert(bases_of(^^Expanded<>, access_context::unchecked()).empty());
static_assert(annotations_of(base(^^Expanded<B0, B1>, 0)).size() == 1);
static_assert(annotations_of(base(^^Expanded<B0, B1>, 1)).size() == 1);
static_assert(extract<int>(annotations_of(base(^^Expanded<B0, B1>, 0))[0]) == 1);
static_assert(extract<int>(annotations_of(base(^^Expanded<B0, B1>, 1))[0]) == 1);
static_assert(annotations_of(base(^^Expanded<B0, B1>, 0))[0] !=
              annotations_of(base(^^Expanded<B0, B1>, 1))[0]);

template<class... Bs> struct DependentExpanded : [[=sizeof(Bs)]] Bs... {};
static_assert(extract<decltype(sizeof(B0))>(annotations_of(base(^^DependentExpanded<B0, B1>, 0))[0]) == sizeof(B0));
static_assert(extract<decltype(sizeof(B1))>(annotations_of(base(^^DependentExpanded<B0, B1>, 1))[0]) == sizeof(B1));

template<int... Ns> struct AnnotationPack : [[=0, =Ns..., =9]] B0 {};
static_assert(annotations_of(base(^^AnnotationPack<>)).size() == 2);
static_assert(annotations_of(base(^^AnnotationPack<2, 4>)).size() == 4);
static_assert(extract<int>(annotations_of(base(^^AnnotationPack<2, 4>))[0]) == 0);
static_assert(extract<int>(annotations_of(base(^^AnnotationPack<2, 4>))[1]) == 2);
static_assert(extract<int>(annotations_of(base(^^AnnotationPack<2, 4>))[2]) == 4);
static_assert(extract<int>(annotations_of(base(^^AnnotationPack<2, 4>))[3]) == 9);

// The base expansion's unexpanded pack can occur solely in its annotation.
template<int... Ns> struct AnnotationOnlyBasePack : [[=Ns]] B0... {};
static_assert(bases_of(^^AnnotationOnlyBasePack<>, access_context::unchecked()).empty());
static_assert(bases_of(^^AnnotationOnlyBasePack<5>, access_context::unchecked()).size() == 1);
static_assert(extract<int>(annotations_of(base(^^AnnotationOnlyBasePack<5>))[0]) == 5);

// Instantiating the outer template must preserve annotations that still
// depend on the inner template's parameter and the base expansion's pack.
template<class... Bs> struct Outer {
  template<int N> struct Inner : [[=N, =sizeof(Bs)]] Bs... {};
};
static_assert(annotations_of(base(^^Outer<B0, B1>::Inner<3>, 0)).size() == 2);
static_assert(annotations_of(base(^^Outer<B0, B1>::Inner<3>, 1)).size() == 2);
static_assert(extract<int>(annotations_of(base(^^Outer<B0, B1>::Inner<3>, 0))[0]) == 3);
static_assert(extract<int>(annotations_of(base(^^Outer<B0, B1>::Inner<3>, 1))[0]) == 3);
static_assert(extract<decltype(sizeof(B0))>(annotations_of(base(^^Outer<B0, B1>::Inner<3>, 0))[1]) == sizeof(B0));
static_assert(extract<decltype(sizeof(B1))>(annotations_of(base(^^Outer<B0, B1>::Inner<3>, 1))[1]) == sizeof(B1));
static_assert(annotations_of(base(^^Outer<B0, B1>::Inner<3>, 0))[0] !=
              annotations_of(base(^^Outer<B0, B1>::Inner<3>, 1))[0]);
static_assert(bases_of(^^Outer<>::Inner<3>, access_context::unchecked()).empty());

// The annotation-list expansion and base-list expansion have independent
// pack lengths and substitution indices.
template<int... Ns> struct OuterAnnotationPack {
  template<class... Bs> struct Inner : [[=Ns...]] Bs... {};
};
static_assert(bases_of(^^OuterAnnotationPack<2, 4>::Inner<B0>, access_context::unchecked()).size() == 1);
static_assert(annotations_of(base(^^OuterAnnotationPack<2, 4>::Inner<B0>)).size() == 2);
static_assert(extract<int>(annotations_of(base(^^OuterAnnotationPack<2, 4>::Inner<B0>))[0]) == 2);
static_assert(extract<int>(annotations_of(base(^^OuterAnnotationPack<2, 4>::Inner<B0>))[1]) == 4);
static_assert(bases_of(^^OuterAnnotationPack<>::Inner<B0, B1>, access_context::unchecked()).size() == 2);
static_assert(annotations_of(base(^^OuterAnnotationPack<>::Inner<B0, B1>, 0)).empty());
static_assert(annotations_of(base(^^OuterAnnotationPack<>::Inner<B0, B1>, 1)).empty());

int main(int, char**) { return 0; }
