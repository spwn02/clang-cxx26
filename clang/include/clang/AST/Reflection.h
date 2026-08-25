//===--- Reflection.h - Classes for representing reflection -----*- C++ -*-===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_REFLECTION_H
#define LLVM_CLANG_AST_REFLECTION_H

#include "clang/AST/Type.h"
#include "llvm/ADT/SmallVector.h"
#include <optional>
#include <string>

namespace clang {

class APValue;
class CXXBaseSpecifier;
class ParsedAttr;

/// The kind of construct represented by a reflection value.
enum class ReflectionKind {
  Null = 0,
  Type,
  Object,
  Value,
  Declaration,
  Template,
  Namespace,
  EntityProxy,
  Parameter,
  BaseSpecifier,
  DataMemberSpec,
  Annotation,
  EnumeratorSpec,
  Attribute,
};

/// Description of a hypothetical data member used by `define_class`.
struct TagDataMemberSpec {
  QualType Ty;
  std::optional<std::string> Name;
  std::optional<size_t> Alignment;
  std::optional<size_t> BitWidth;
  bool NoUniqueAddress;
  llvm::SmallVector<ParsedAttr *, 2> Attributes;

  bool operator==(const TagDataMemberSpec &Rhs) const;
  bool operator!=(const TagDataMemberSpec &Rhs) const;
};

/// Description of an enumerator used by `define_enum`.
struct EnumeratorSpec {
  std::string name;
  bool hasValue;
  int64_t val;
  llvm::SmallVector<APValue *, 2> annotations;
  llvm::SmallVector<APValue *, 2> attributes;
};

} // namespace clang

#endif
