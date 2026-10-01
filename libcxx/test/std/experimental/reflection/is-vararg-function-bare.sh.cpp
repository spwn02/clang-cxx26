//===----------------------------------------------------------------------===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23

// The standard query does not require parameter reflection.
// RUN: %{cxx} %{flags} %{compile_flags} -std=c++26 -freflection -fsyntax-only %S/is-vararg-function.pass.cpp
