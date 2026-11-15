// RUN: %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -x c++-module --precompile %s -o %t.pcm
// RUN: %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fmodule-file=Location=%t.pcm -fsyntax-only %S/Inputs/token-sequence-location-use.cpp

module;
#include <__meta/token_locations.h>
export module Location;

export constexpr std::meta::token_sequence locations = ^^{ identifier + 1 };
