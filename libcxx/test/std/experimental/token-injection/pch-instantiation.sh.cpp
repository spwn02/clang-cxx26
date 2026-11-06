//===----------------------------------------------------------------------===//
//
// Copyright 2026 Jump Trading, LLC
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23 || c++26

// FILE_DEPENDENCIES: pch-instantiation.h
//
// A token sequence literal instantiated from a template in a file whose
// <meta> comes from a precompiled header (the preamble of an editor): the class
// std::meta::token_sequence loads its fields lazily, and its hidden member is
// what makes it consteval-only (the compiler asserted).
//
// RUN: %{cxx} %{compile_flags} -x c++-header pch-instantiation.h -o %t.pch
// RUN: %{cxx} %{compile_flags} -include-pch %t.pch -fsyntax-only %s

template <class T> struct Forwarder {
  consteval {
    for (std::meta::info m : members_of(^^std::vector<T>,
                                        std::meta::access_context::unprivileged())) {
      if (!is_function(m) || !has_identifier(m) || is_special_member_function(m) ||
          is_constructor(m) || is_operator_function(m))
        continue;
      auto d = std::meta::declaration_of(m);
      auto call = std::meta::forwarding_call_for(d, ^^{ impl });
      (void)call;
    }
  }
  std::vector<T> impl;
};

Forwarder<int> f;
