// RUN: mkdir -p %t
// RUN: %clang_cc1 -std=c++26 -emit-module-interface -o %t/lib.pcm %S/Inputs/p4466-lib.cppm
// RUN: %clang_cc1 -std=c++26 -fmodule-file=lib=%t/lib.pcm %s -verify

import lib;

static_assert(^^lib::type == ^^lib::v1::type);
static_assert(^^lib::bs == ^^lib::v1::bs);

// expected-no-diagnostics
