// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -I %S/Inputs -emit-pch -o %t.pch -x c++-header %S/Inputs/macro-invocation-dependent.h
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -I %S/Inputs -include-pch %t.pch -fsyntax-only -verify %s

// A dependent expression-macro invocation read back from a PCH keeps the
// environment its unparsed arguments are parsed in: the instantiation here
// crashed in ParseDeferredMacroArguments because the context, the template
// parameters and the explicit template arguments were not serialized (#502).
// expected-no-diagnostics

static_assert(use_member(Holder{10}) == 11);
static_assert(use_member_tmpl(Holder{10}) == 18);
static_assert(use_free(Holder{10}) == 11);
constexpr int via_pointer() { Holder h{5}; return use_arrow(&h); }
static_assert(via_pointer() == 8);
constexpr int via_class_template() { Box<Holder> b{Holder{1}}; return b.use(); }
static_assert(via_class_template() == 1 + sizeof(Holder));
