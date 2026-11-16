// RUN: %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -fcxx-exceptions -Xclang -verify -Wno-macro-located-diagnostics %s

#include <__meta/token_locations.h>

using std::meta::token_sequence;

__macro warning_and_note(token_sequence ts) {
  (void)std::meta::__x::__warning(ts[0], "suppressed warning");
  (void)std::meta::__x::__note(ts[1], "retained note");
  return ^^{ 42 };
}

static_assert(warning_and_note!(warning_token note_token) == 42); // expected-note@13 {{retained note}}
