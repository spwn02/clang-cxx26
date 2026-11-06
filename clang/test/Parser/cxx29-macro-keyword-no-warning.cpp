// RUN: %clang_cc1 -std=c++29 -pedantic -Werror -I %S/../Reflection/Inputs -fsyntax-only -verify %s
// expected-no-diagnostics

// `__macro` is a keyword of C++29 (P4380), not an extension: no
// -Wlanguage-extension-token under -pedantic.
#include "token-sequence.h"

template <class T> __macro fwd(T &&t) { return ^^{ \(t) }; }
__macro plain(std::meta::token_sequence body) { return body; }
