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

#include "clang/AST/ReflectionValue.h"
#include "clang/Basic/OperatorKinds.h"
#include "clang/Basic/TokenKinds.h"
#include "clang/AST/TypeBase.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/ADT/SmallVector.h"
#include <optional>
#include <string>

namespace clang {

class APValue;
class CXXBaseSpecifier;
class NamedDecl;
class NamespaceDecl;
class Token;

struct FunctionDeclSpec;

/// \\brief Representation of a captured token sequence from ^^{ ... }.
/// Essentially an ArrayRef<Token> allocated in the ASTContext.
struct TokenSequenceData : public ArrayRef<Token> {
  TokenSequenceData() = default;
  TokenSequenceData(const Token *Data, size_t Length)
      : ArrayRef<Token>(Data, Length) {}
};

/// Allocate token sequence storage in the ASTContext. Empty sequences are
/// represented with an empty array.
/// std::meta::operators enumerates the overloadable operators in a fixed
/// order, with 0 meaning "no operator". These convert between that order and
/// OverloadedOperatorKind.
OverloadedOperatorKind getOverloadedOperatorForMetaIndex(unsigned Index);
unsigned getMetaIndexForOverloadedOperator(OverloadedOperatorKind OO);

/// std::meta::punctuator enumerates C++'s punctuators in a fixed order (see
/// MetaPunctuators.def). These convert between that order and token kinds;
/// a token kind that is not one of them has no index (~0u), and an index out
/// of range has no token kind (tok::unknown).
unsigned getMetaIndexForPunctuator(tok::TokenKind Kind);
tok::TokenKind getPunctuatorForMetaIndex(unsigned Index);

/// The overloadable operator a single token spells, or OO_None. Operators
/// without a one-token spelling ('()', '[]', new, delete) never match.
OverloadedOperatorKind getOverloadedOperatorForTokenKind(tok::TokenKind Kind);

/// Remove deduced placeholder sugar ('auto', 'decltype(auto)', deduced class
/// template names) from \p T so it can be spelled as a concrete type. The
/// deduced type's own sugar is kept when the placeholder is at the top level.
QualType stripDeducedTypeSugar(const ASTContext &C, QualType T);

TokenSequenceData CreateTokenSequenceData(ASTContext &Ctx,
                                          ArrayRef<Token> Tokens);

TokenSequenceData CreateTokenSequenceData(ASTContext &Ctx,
                                          ArrayRef<Token> Tokens1,
                                          ArrayRef<Token> Tokens2);

/// \p LHS followed by \p RHS, for std::meta::token_sequence's += and +.
/// Token arrays are immutable: an array is a view of a prefix of a buffer
/// that is only ever appended to. When \p LHS is a whole buffer's used
/// prefix and the buffer has room, \p RHS is written past it and the
/// result views the longer prefix -- every other view of the buffer still
/// sees exactly what it saw. Otherwise both are copied into a new buffer
/// with spare capacity. So building a sequence by appending is amortized
/// linear, and copies of a sequence never need copying themselves.
TokenSequenceData AppendTokenSequenceData(ASTContext &Ctx,
                                          TokenSequenceData LHS,
                                          ArrayRef<Token> RHS);

/// Allocate an empty token sequence in the ASTContext.
inline TokenSequenceData CreateEmptyTokenSequenceData(ASTContext &Ctx) {
  return CreateTokenSequenceData(Ctx, {});
}

/// The tokens of \p TSD as an array of std::meta::token in static storage
/// (what a std::meta::token_sequence iterates): a pointer to its first element,
/// of type \p PtrTy, or a null pointer for no tokens. The storage for a token
/// array is made once (keyed by its storage), and remembers its tokens (see
/// getMaterializedTokens).
APValue MaterializeTokens(ASTContext &C, TokenSequenceData TSD, QualType PtrTy);

/// The tokens of an array made by MaterializeTokens, or null.
const TokenSequenceData *getMaterializedTokens(const Decl *Storage);

/// Description of a hypothetical data member used by `define_class`.
struct TagDataMemberSpec {
  QualType Ty;
  std::optional<std::string> Name;
  std::optional<size_t> Alignment;
  std::optional<size_t> BitWidth;
  bool NoUniqueAddress;
  llvm::SmallVector<APValue *, 2> Annotations;

  bool operator==(const TagDataMemberSpec &Rhs) const;
  bool operator!=(const TagDataMemberSpec &Rhs) const;
};

/// \brief Description of a function declaration to be introduced into a
/// class, cloned from an existing member function or member function
/// template (std::meta::declaration_of).
///
/// The description retains the *source* declaration and the naming policy;
/// the actual clone is produced at each injection site, so reusing the
/// description creates fresh declarations each time.
struct FunctionDeclSpec {
  /// The source: a CXXMethodDecl or a FunctionTemplateDecl whose templated
  /// declaration is a CXXMethodDecl.
  NamedDecl *Source;

  /// Replacement declaration name; empty means keep the source's name.
  std::optional<std::string> Name;

  /// Prefixes for renaming template parameters (-> T0, T1, ...) and function
  /// parameters (-> p0, p1, ...).
  std::string TemplateParameterPrefix;
  std::string ParameterPrefix;

  /// Transformations applied to the description
  /// (std::meta::make_override / make_noexcept): declare the clone
  /// 'override', and/or declare it noexcept.
  bool MarkOverride = false;
  bool MarkNoexcept = false;

  bool operator==(FunctionDeclSpec const &Rhs) const;
  bool operator!=(FunctionDeclSpec const &Rhs) const;
};

/// \brief Annotation payload for std::meta::template_parameter_list_for.
///
/// Carried by a tok::annot_template_param_spec token inside a token
/// sequence; when the parser reaches it inside a written template<...>
/// parameter list, the description's cloned (renamed) template parameters
/// are materialized at that position.
struct TemplateParamListSpec {
  FunctionDeclSpec *Spec;
  bool KeepDefaults;
};
} // namespace clang

#endif
