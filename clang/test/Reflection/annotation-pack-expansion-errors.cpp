// RUN: %clang_cc1 -std=c++26 -freflection -fannotation-attributes -verify %s

struct [[=42 ...]] NoPack {}; // expected-error {{pack expansion does not contain any unexpanded parameter packs}}
template<int... Is> struct [[=Is]] Unexpanded {}; // expected-error {{contains unexpanded parameter pack 'Is'}}
