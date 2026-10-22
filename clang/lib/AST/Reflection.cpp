//===--- Reflection.cpp - Classes for representing reflection ---*- C++ -*-===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "clang/AST/APValue.h"
#include "clang/AST/Reflection.h"
#include "llvm/ADT/FoldingSet.h"

namespace clang {

bool TagDataMemberSpec::operator==(const TagDataMemberSpec &Rhs) const {
  if (!(Ty == Rhs.Ty && Alignment == Rhs.Alignment &&
        BitWidth == Rhs.BitWidth && Name == Rhs.Name &&
        NoUniqueAddress == Rhs.NoUniqueAddress &&
        Annotations.size() == Rhs.Annotations.size()))
    return false;
  for (size_t I = 0; I != Annotations.size(); ++I) {
    llvm::FoldingSetNodeID L, R;
    Annotations[I]->Profile(L);
    Rhs.Annotations[I]->Profile(R);
    if (L != R)
      return false;
  }
  return true;
}

bool TagDataMemberSpec::operator!=(const TagDataMemberSpec &Rhs) const {
  return !(*this == Rhs);
}

} // namespace clang
