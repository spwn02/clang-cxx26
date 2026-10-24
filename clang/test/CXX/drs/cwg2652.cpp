// RUN: %clang_cc1 -std=c++23 -triple x86_64-unknown-unknown -fsyntax-only -fstdcpp-float-macros -verify %s
// expected-no-diagnostics

namespace cwg2652 { // cwg2652: yes
// [cpp.predefined]: __STDCPP_BFLOAT16_T__ is defined if and only if
// std::bfloat16_t is supported with the properties of [basic.extended.fp].
#ifndef __STDCPP_BFLOAT16_T__
#error __STDCPP_BFLOAT16_T__ is not defined
#endif
static_assert(sizeof(__bf16) == 2, "");
} // namespace cwg2652
