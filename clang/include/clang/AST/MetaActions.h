//===--- MetaActions.h - Interface for metafunction actions -----*- C++ -*-===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief Provides an interface for actions requiring semantic analysis from
///        C++26 reflection functions (i.e., metafunctions).
///
/// DESIGN NOTE: the "Sema problem" and the evaluation-callback model
/// ===================================================================
/// This file exists to solve a specific architectural tension, folded in
/// here from this fork's former docs/REFLECTION.md (deleted once reflection
/// support was judged complete -- see docs/CXX26_GAPS.md's history and
/// AGENTS.md's Trackers section for context) so the reasoning survives.
///
/// Most metafunctions (`is_type`, `is_public`, `parent_of`, ...) are purely
/// observational: they query AST properties without changing anything, and
/// fit naturally into `clang`'s existing constant-evaluation machinery
/// (`AST/ExprConstant.cpp`), which has no access to `Sema`. But several of
/// the most powerful metafunctions -- `define_class`, `reflect_invoke`,
/// `substitute` -- are *generative*: evaluating them must modify AST state
/// (declare a class, instantiate a template) with the same semantic
/// checking `Sema` performs for ordinary declarations. Constructing AST
/// nodes directly via `ASTContext` alone, without that checking, is a
/// recipe for compiler crashes and assertion failures once later stages
/// (e.g. CodeGen) rely on invariants only `Sema` enforces. So these
/// metafunctions cannot be coherently implemented without `Sema` access --
/// but `AST/ExprConstant.cpp`, where all constant evaluation lives, is
/// deliberately architected to have none.
///
/// Two approaches have been tried across this feature's history:
///
/// - **Lock3/P2320's `EvalContext` model** (the historical fork this
///   implementation's initial roadmap came from): thread an
///   `<ASTContext, ReflectionCallback*>` pair through the `Expr::Evaluate*`
///   family in place of a bare `ASTContext`, with `ReflectionCallback` a
///   polymorphic interface to a narrow slice of `Sema`. Invasive (touches
///   most of `ExprConstant.cpp`'s call sites), and allows evaluation to
///   proceed even when no callback is available (`ReflectionCallback*` can
///   be null), which can crash if a metafunction is evaluated somewhere
///   with no route back to `Sema`.
/// - **This fork's "evaluation callback" model** (what `MetaActions` here
///   and `CXXMetafunctionExpr`'s callback implement): bind a reference to
///   `Sema` into the callback owned by `CXXMetafunctionExpr` itself, which
///   `AST/ExprConstant.cpp`'s `VisitCXXMetafunctionExpr` invokes, along
///   with a separate "Evaluator" callback letting a `Metafunction::evaluate`
///   implementation (in `Sema`) evaluate a sub-expression using the
///   existing constant-evaluation context, without needing the `EvalInfo`
///   object. Less invasive than `EvalContext`, but introduces a real,
///   still-open cost: **the callback captured by `CXXMetafunctionExpr`
///   cannot be serialized.** Since `clang` serializes ASTs for C++20
///   modules and precompiled headers, this breaks both for any
///   `CXXMetafunctionExpr` node -- and even if the callback state itself
///   were serialized, the `Sema` object it closes over almost certainly
///   won't exist in the deserializing process (module-consuming and
///   module-producing compilations are typically different processes).
///   `ASTReader` and `Sema` share a `CompilerInstance`, so constructing
///   `ASTReader` with a `Sema` reference could in principle let it
///   reconstruct callbacks bound to the *new* process's `Sema` on
///   deserialization -- this has not been attempted, and it isn't
///   obviously the right direction even if it works.
///
/// **Neither approach is a full solution.** `EvalContext` avoids storing an
/// unserializable callback in the AST but narrows where an expression can
/// safely be evaluated at all; this fork's model widens that but pushes the
/// unserializability problem into PCH/modules instead. The most defensible
/// summary: once a constant-evaluated expression can have side effects on
/// the AST (as static reflection requires), constant folding can no longer
/// be performed without `Sema` in the loop, full stop -- a third design
/// that more fundamentally reconsiders the `Sema`/AST boundary might do
/// better than either of the two tried so far, but neither this fork nor
/// P2320 has attempted one.
///
/// A smaller, related design note: metafunctions are implemented in a
/// separate `Sema/Metafunctions.cpp` rather than inline in
/// `ExprConstant.cpp` (where nearly all other constant-evaluation logic
/// lives), to keep this experimental surface isolated and preserve the
/// `Sema`/`AST` layering. The cost is that metafunction implementations
/// lack direct access to constant-evaluation state like the call stack;
/// where that state is needed (e.g. `value_of`, `is_accessible`), this
/// fork synthesizes small `Expr` nodes (`LValueValueOfExpr`,
/// `StackLocationExpr`) purely as a communication channel back from the
/// evaluator, at the cost of extra AST-node allocation.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_METAACTIONS_H
#define LLVM_CLANG_AST_METAACTIONS_H

#include <clang/AST/TemplateBase.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceLocation.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/SmallVector.h>

namespace clang {

class AttributeCommonInfo;
class ConceptDecl;
class CXXBasePath;
class CXXRecordDecl;
class Decl;
class DeclContext;
class DeclRefExpr;
class EnumeratorSpec;
class Expr;
class FunctionDecl;
class FunctionTemplateDecl;
class NamedDecl;
struct TagDataMemberSpec;
class TemplateDecl;
class TypeAliasTemplateDecl;
class VarDecl;
class VarTemplateDecl;
class ParsedAttributesView;

// Interface for actions requiring semantic analysis from C++26 reflection
// functions (i.e., metafunctions).
class MetaActions {
public:
  virtual ~MetaActions() {}

                            // ====================
                            // Access-Check Support
                            // ====================

  // Returns the current declaration context.
  virtual Decl *CurrentCtx() const = 0;

  // Returns whether the declaration 'D' is accessible from 'Ctx'.
  virtual bool IsAccessible(NamedDecl *Target, DeclContext *Ctx,
                            CXXRecordDecl *NamingCls) = 0;

  // Returns whether the base class 'B' is accessible from 'Ctx'.
  virtual bool IsAccessibleBase(QualType BaseTy, QualType DerivedTy,
                                const CXXBasePath &Path,
                                DeclContext *Ctx, SourceLocation AccessLoc) = 0;

                            // ====================
                            // Substitution Support
                            // ====================

  // Returns 'true' if 'TArgs' are allowed template arguments for 'TD'.
  // Otherwise, 'false'.
  virtual bool
  CheckTemplateArgumentList(TemplateDecl *TD,
                            SmallVectorImpl<TemplateArgument> &TArgs,
                            bool SuppressDiagnostics,
                            SourceLocation InstantiateLoc) = 0;

  // Returns the specialization 'TD<TArgs...>'. The template arguments are
  // assumed to be valid for the specialization, as a precondition; substitution
  // into the declaration can still fail, in which case a null result is
  // returned. When 'SuppressDiagnostics' is true, no diagnostics leak from a
  // failed substitution.
  virtual QualType Substitute(TypeAliasTemplateDecl *TD,
                              ArrayRef<TemplateArgument> TArgs,
                              bool SuppressDiagnostics,
                              SourceLocation InstantiateLoc) = 0;
  virtual FunctionDecl *Substitute(FunctionTemplateDecl *TD,
                                   ArrayRef<TemplateArgument> TArgs,
                                   bool SuppressDiagnostics,
                                   SourceLocation InstantiationLoc) = 0;
  virtual VarDecl *Substitute(VarTemplateDecl *TD,
                              ArrayRef<TemplateArgument> TArgs,
                              bool SuppressDiagnostics,
                              SourceLocation InstantiateLoc) = 0;
  virtual Expr *Substitute(ConceptDecl *TD, ArrayRef<TemplateArgument> TArgs,
                           bool SuppressDiagnostics,
                           SourceLocation InstantiateLoc) = 0;

                          // ========================
                          // Member Iteration Support
                          // ========================

  // If 'D' is a template specialization, then ensures that 'D' is instantiated.
  // Returns 'false' if 'D' could not be instantiated (e.g., failed
  // constraints). Otherwise, 'true'.
  virtual bool EnsureInstantiated(Decl *D, SourceRange Range) = 0;

  // Ensures that any implicit members of 'RD' have been declared.
  virtual void EnsureDeclarationOfImplicitMembers(CXXRecordDecl *RD) = 0;

  // Ensures instantiation of the exception specification of 'FD'.
  virtual void EnsureInstantiationOfExceptionSpec(SourceLocation Loc,
                                                  FunctionDecl *FD) = 0;

  // Returns 'true' if the constraints of 'FD' are satisfied.
  // Otherwise, 'false'.
  virtual bool HasSatisfiedConstraints(FunctionDecl *FD) = 0;

                             // ==================
                             // Invocation Support
                             // ==================

  // Returns the specialization of 'FD' deduced from the explicit template
  // arguments 'TArgs' and the function arguments 'Args'.
  virtual FunctionDecl *DeduceSpecialization(FunctionTemplateDecl *FTD,
                                             ArrayRef<TemplateArgument> TArgs,
                                             ArrayRef<Expr *> Args,
                                             SourceLocation InstantiateLoc) = 0;

  // Synthesizes a member-access expression for 'Obj.Mem', eliding member
  // lookup.
  virtual Expr *
  SynthesizeDirectMemberAccess(Expr *Obj, CXXReflectExpr *Mem,
                               SourceLocation PlaceholderLoc) = 0;

  // Synthesizes a call expression for 'Fn(Args...)'.
  virtual Expr *SynthesizeCallExpr(Expr *Fn, MutableArrayRef<Expr *> Args) = 0;

                           // =======================
                           // Class Synthesis Support
                           // =======================

  // Returns a new definition of 'D' having the members specified by 'Mems'.
  virtual
  CXXRecordDecl *DefineAggregate(CXXRecordDecl *IncompleteDecl,
                                 ArrayRef<TagDataMemberSpec *> MemberSpecs,
                                 Decl *ContainingDecl,
                                 SourceLocation DefinitionLoc) = 0;

  // Appertains the value represented by 'Value' as an annotation of 'Decl'.
  virtual CXX26AnnotationAttr *Annotate(Decl *TargetDecl, const APValue &Value,
                                        Decl *ContainingDecl,
                                        SourceLocation DefinitionLoc) = 0;

  virtual EnumDecl *
  DefineEnum(EnumDecl *ED,
             SmallVector<EnumeratorSpec *, 8> EnumSpecs,
             Decl *ContainingDecl,
             QualType TargetEnumType,
             const ParsedAttributesView *Attrs,
             SourceLocation DefinitionLoc) = 0;

  // ============================
  // Annotation Synthesis Support
  // ============================

  virtual AttributeCommonInfo *SynthesizeAnnotation(Expr *CE,
                                                    SourceLocation Loc) = 0;
};
} // namespace clang

#endif
