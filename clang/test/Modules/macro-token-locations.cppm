// RUN: %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fmodule-name=MacroTokens -x c++-module --precompile %s -o %t.pcm
// RUN: not %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fmodule-file=MacroTokens=%t.pcm -fsyntax-only -Wmacro-located-diagnostics -fdiagnostics-parseable-fixits %S/Inputs/macro-token-locations-use.cpp 2>&1 | FileCheck %S/Inputs/macro-token-locations-use.cpp --check-prefix=DIAG
// RUN: %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fmodule-file=MacroTokens=%t.pcm -fsyntax-only -Rmacro-role -DTEST_ROLES %S/Inputs/macro-token-locations-use.cpp 2>&1 | FileCheck %S/Inputs/macro-token-locations-use.cpp --check-prefix=ROLE
module;
#include <__meta/token_locations.h>
export module MacroTokens;

export constexpr std::meta::token_sequence imported_diagnostic_tokens = ^^{
  imported_error
  imported_warning
};
export constexpr std::meta::token_sequence imported_role_tokens = ^^{
  imported_role
};
