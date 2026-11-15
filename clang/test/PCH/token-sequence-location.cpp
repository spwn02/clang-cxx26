// RUN: %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -x c++-header -Xclang -emit-pch %S/Inputs/token-sequence-location.h -o %t.pch
// RUN: %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -include-pch %t.pch -fsyntax-only %s

static_assert(std::meta::__x::__token_begin(locations[0]).line() == 3);
static_assert(std::meta::__x::__token_end(locations[0]).column() ==
              std::meta::__x::__token_begin(locations[0]).column() + 10);
static_assert(std::meta::__x::__token_end(locations[1]).column() ==
              std::meta::__x::__token_begin(locations[1]).column() + 1);
