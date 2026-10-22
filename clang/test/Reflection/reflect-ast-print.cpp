// RUN: %clang_cc1 -std=c++26 -ast-print %s > %t
// RUN: FileCheck %s < %t
// RUN: %clang_cc1 -std=c++26 -x c++ -fsyntax-only %t

namespace n { int value; template<class> struct Long; }
constexpr auto type = ^^int;
constexpr auto declaration = ^^n::value;
constexpr auto templ = ^^n::Long;

// CHECK: constexpr auto type = ^^int;
// CHECK: constexpr auto declaration = ^^n::value;
// CHECK: constexpr auto templ = ^^n::Long;
