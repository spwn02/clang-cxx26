//===--- ReflectionValue.h - Reflection value types ------------*- C++ -*-===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_REFLECTIONVALUE_H
#define LLVM_CLANG_AST_REFLECTIONVALUE_H

namespace clang {

enum class ReflectionKind {
  Null = 0, Type, Object, Value, Declaration, Template, Namespace,
  EntityProxy, Parameter, BaseSpecifier, DataMemberSpec, Annotation,
  EnumeratorSpec, Attribute,
};

} // namespace clang

#endif
