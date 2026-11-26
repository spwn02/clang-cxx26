// RUN: mkdir -p %t
// RUN: %clang_cc1 -std=c++26 -emit-module-interface -o %t/lib26.pcm %S/Inputs/p4466-global-using.cppm
// RUN: %clang_cc1 -std=c++26 -fmodule-file=p4466_global_using=%t/lib26.pcm %s -verify
// RUN: %clang_cc1 -std=c++29 -emit-module-interface -o %t/lib29.pcm %S/Inputs/p4466-global-using.cppm
// RUN: %clang_cc1 -std=c++29 -fmodule-file=p4466_global_using=%t/lib29.pcm %s -verify

import p4466_global_using;

static_assert(^^::size_t == ^^size_t);
static_assert(^^::strlen == ^^strlen);
static_assert(^^global_from_export_block == ^^global_from_export_block);
static_assert(^^global_from_linkage_block == ^^global_from_linkage_block);

// P4466R2 §8, [namespace.udecl] *.2.2: the non-inline namespace named by
// the exported using-declarator is an intervening scope.
constexpr auto rejected_export = ^^from_export_block;
// expected-error@-1 {{cannot take the reflection of a using-declarator}}

// The extern "C" wrapper does not make Named::NonInline transparent.
constexpr auto rejected_linkage = ^^Named::from_linkage_block;
// expected-error@-1 {{cannot take the reflection of a using-declarator}}
