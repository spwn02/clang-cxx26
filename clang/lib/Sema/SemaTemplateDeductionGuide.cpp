//===- SemaTemplateDeductionGude.cpp - Template Argument Deduction---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements deduction guides for C++ class template argument
// deduction.
//
//===----------------------------------------------------------------------===//

#include "TreeTransform.h"
#include "TypeLocBuilder.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclBase.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclFriend.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/AST/DeclarationName.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/OperationKinds.h"
#include "clang/AST/TemplateBase.h"
#include "clang/AST/TemplateName.h"
#include "clang/AST/Type.h"
#include "clang/AST/TypeLoc.h"
#include "clang/Basic/LLVM.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/Specifiers.h"
#include "clang/Basic/TypeTraits.h"
#include "clang/Sema/DeclSpec.h"
#include "clang/Sema/Initialization.h"
#include "clang/Sema/Lookup.h"
#include "clang/Sema/Overload.h"
#include "clang/Sema/Ownership.h"
#include "clang/Sema/Scope.h"
#include "clang/Sema/SemaInternal.h"
#include "clang/Sema/Template.h"
#include "clang/Sema/TemplateDeduction.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include <cassert>
#include <optional>
#include <utility>

using namespace clang;
using namespace sema;

namespace {

/// Return true if two associated-constraint sets are semantically equal.
static bool HaveSameAssociatedConstraints(
    Sema &SemaRef, const NamedDecl *Old, ArrayRef<AssociatedConstraint> OldACs,
    const NamedDecl *New, ArrayRef<AssociatedConstraint> NewACs) {
  if (OldACs.size() != NewACs.size())
    return false;
  if (OldACs.empty())
    return true;

  // General case: pairwise compare each associated constraint expression.
  Sema::TemplateCompareNewDeclInfo NewInfo(New);
  for (size_t I = 0, E = OldACs.size(); I != E; ++I)
    if (!SemaRef.AreConstraintExpressionsEqual(
            Old, OldACs[I].ConstraintExpr, NewInfo, NewACs[I].ConstraintExpr))
      return false;

  return true;
}

/// Tree transform to "extract" a transformed type from a class template's
/// constructor to a deduction guide.
class ExtractTypeForDeductionGuide
    : public TreeTransform<ExtractTypeForDeductionGuide> {
  llvm::SmallVectorImpl<TypedefNameDecl *> &MaterializedTypedefs;
  ClassTemplateDecl *NestedPattern;
  const MultiLevelTemplateArgumentList *OuterInstantiationArgs;
  std::optional<TemplateDeclInstantiator> TypedefNameInstantiator;

public:
  typedef TreeTransform<ExtractTypeForDeductionGuide> Base;
  ExtractTypeForDeductionGuide(
      Sema &SemaRef,
      llvm::SmallVectorImpl<TypedefNameDecl *> &MaterializedTypedefs,
      ClassTemplateDecl *NestedPattern = nullptr,
      const MultiLevelTemplateArgumentList *OuterInstantiationArgs = nullptr)
      : Base(SemaRef), MaterializedTypedefs(MaterializedTypedefs),
        NestedPattern(NestedPattern),
        OuterInstantiationArgs(OuterInstantiationArgs) {
    if (OuterInstantiationArgs)
      TypedefNameInstantiator.emplace(
          SemaRef, SemaRef.getASTContext().getTranslationUnitDecl(),
          *OuterInstantiationArgs);
  }

  TypeSourceInfo *transform(TypeSourceInfo *TSI) { return TransformType(TSI); }

  /// Returns true if it's safe to substitute \p Typedef with
  /// \p OuterInstantiationArgs.
  bool mightReferToOuterTemplateParameters(TypedefNameDecl *Typedef) {
    if (!NestedPattern)
      return false;

    static auto WalkUp = [](DeclContext *DC, DeclContext *TargetDC) {
      if (DC->Equals(TargetDC))
        return true;
      while (DC->isRecord()) {
        if (DC->Equals(TargetDC))
          return true;
        DC = DC->getParent();
      }
      return false;
    };

    if (WalkUp(Typedef->getDeclContext(), NestedPattern->getTemplatedDecl()))
      return true;
    if (WalkUp(NestedPattern->getTemplatedDecl(), Typedef->getDeclContext()))
      return true;
    return false;
  }

  QualType RebuildTemplateSpecializationType(
      ElaboratedTypeKeyword Keyword, TemplateName Template,
      SourceLocation TemplateNameLoc, TemplateArgumentListInfo &TemplateArgs) {
    if (!OuterInstantiationArgs ||
        !isa_and_present<TypeAliasTemplateDecl>(Template.getAsTemplateDecl()))
      return Base::RebuildTemplateSpecializationType(
          Keyword, Template, TemplateNameLoc, TemplateArgs);

    auto *TATD = cast<TypeAliasTemplateDecl>(Template.getAsTemplateDecl());
    auto *Pattern = TATD;
    while (Pattern->getInstantiatedFromMemberTemplate())
      Pattern = Pattern->getInstantiatedFromMemberTemplate();
    if (!mightReferToOuterTemplateParameters(Pattern->getTemplatedDecl()))
      return Base::RebuildTemplateSpecializationType(
          Keyword, Template, TemplateNameLoc, TemplateArgs);

    Decl *NewD =
        TypedefNameInstantiator->InstantiateTypeAliasTemplateDecl(TATD);
    if (!NewD)
      return QualType();

    auto *NewTATD = cast<TypeAliasTemplateDecl>(NewD);
    MaterializedTypedefs.push_back(NewTATD->getTemplatedDecl());

    return Base::RebuildTemplateSpecializationType(
        Keyword, TemplateName(NewTATD), TemplateNameLoc, TemplateArgs);
  }

  QualType TransformTypedefType(TypeLocBuilder &TLB, TypedefTypeLoc TL) {
    ASTContext &Context = SemaRef.getASTContext();
    TypedefNameDecl *OrigDecl = TL.getDecl();
    TypedefNameDecl *Decl = OrigDecl;
    const TypedefType *T = TL.getTypePtr();
    // Transform the underlying type of the typedef and clone the Decl only if
    // the typedef has a dependent context.
    bool InDependentContext = OrigDecl->getDeclContext()->isDependentContext();

    // A typedef/alias Decl within the NestedPattern may reference the outer
    // template parameters. They're substituted with corresponding instantiation
    // arguments here and in RebuildTemplateSpecializationType() above.
    // Otherwise, we would have a CTAD guide with "dangling" template
    // parameters.
    // For example,
    //   template <class T> struct Outer {
    //     using Alias = S<T>;
    //     template <class U> struct Inner {
    //       Inner(Alias);
    //     };
    //   };
    if (OuterInstantiationArgs && InDependentContext &&
        T->isInstantiationDependentType()) {
      Decl = cast_if_present<TypedefNameDecl>(
          TypedefNameInstantiator->InstantiateTypedefNameDecl(
              OrigDecl, /*IsTypeAlias=*/isa<TypeAliasDecl>(OrigDecl)));
      if (!Decl)
        return QualType();
      MaterializedTypedefs.push_back(Decl);
    } else if (InDependentContext) {
      TypeLocBuilder InnerTLB;
      QualType Transformed =
          TransformType(InnerTLB, OrigDecl->getTypeSourceInfo()->getTypeLoc());
      TypeSourceInfo *TSI = InnerTLB.getTypeSourceInfo(Context, Transformed);
      if (isa<TypeAliasDecl>(OrigDecl))
        Decl = TypeAliasDecl::Create(
            Context, Context.getTranslationUnitDecl(), OrigDecl->getBeginLoc(),
            OrigDecl->getLocation(), OrigDecl->getIdentifier(), TSI);
      else {
        assert(isa<TypedefDecl>(OrigDecl) && "Not a Type alias or typedef");
        Decl = TypedefDecl::Create(
            Context, Context.getTranslationUnitDecl(), OrigDecl->getBeginLoc(),
            OrigDecl->getLocation(), OrigDecl->getIdentifier(), TSI);
      }
      MaterializedTypedefs.push_back(Decl);
    }

    NestedNameSpecifierLoc QualifierLoc = TL.getQualifierLoc();
    if (QualifierLoc) {
      QualifierLoc = getDerived().TransformNestedNameSpecifierLoc(QualifierLoc);
      if (!QualifierLoc)
        return QualType();
    }

    QualType TDTy = Context.getTypedefType(
        T->getKeyword(), QualifierLoc.getNestedNameSpecifier(), Decl);
    TLB.push<TypedefTypeLoc>(TDTy).set(TL.getElaboratedKeywordLoc(),
                                       QualifierLoc, TL.getNameLoc());
    return TDTy;
  }
};

// Build a deduction guide using the provided information.
//
// A deduction guide can be either a template or a non-template function
// declaration. If \p TemplateParams is null, a non-template function
// declaration will be created.
NamedDecl *
buildDeductionGuide(Sema &SemaRef, TemplateDecl *OriginalTemplate,
                    TemplateParameterList *TemplateParams,
                    CXXConstructorDecl *Ctor, ExplicitSpecifier ES,
                    TypeSourceInfo *TInfo, SourceLocation LocStart,
                    SourceLocation Loc, SourceLocation LocEnd, bool IsImplicit,
                    llvm::ArrayRef<TypedefNameDecl *> MaterializedTypedefs = {},
                    const AssociatedConstraint &FunctionTrailingRC = {}) {
  DeclContext *DC = OriginalTemplate->getDeclContext();
  auto DeductionGuideName =
      SemaRef.Context.DeclarationNames.getCXXDeductionGuideName(
          OriginalTemplate);

  DeclarationNameInfo Name(DeductionGuideName, Loc);
  ArrayRef<ParmVarDecl *> Params =
      TInfo->getTypeLoc().castAs<FunctionProtoTypeLoc>().getParams();

  // Build the implicit deduction guide template.
  QualType GuideType = TInfo->getType();

  // In CUDA/HIP mode, avoid duplicate implicit guides that differ only in CUDA
  // target attributes (same constructor signature and constraints).
  if (IsImplicit && Ctor && SemaRef.getLangOpts().CUDA) {
    SmallVector<AssociatedConstraint, 4> NewACs;
    Ctor->getAssociatedConstraints(NewACs);

    for (NamedDecl *Existing : DC->lookup(DeductionGuideName)) {
      auto *ExistingFT = dyn_cast<FunctionTemplateDecl>(Existing);
      auto *ExistingGuide =
          ExistingFT
              ? dyn_cast<CXXDeductionGuideDecl>(ExistingFT->getTemplatedDecl())
              : dyn_cast<CXXDeductionGuideDecl>(Existing);
      if (!ExistingGuide)
        continue;

      // Only consider guides that were also synthesized from a constructor.
      auto *ExistingCtor = ExistingGuide->getCorrespondingConstructor();
      if (!ExistingCtor)
        continue;

      // If the underlying constructors are overloads (different signatures once
      // CUDA attributes are ignored), they should each get their own guides.
      if (SemaRef.IsOverload(Ctor, ExistingCtor,
                             /*UseMemberUsingDeclRules=*/false,
                             /*ConsiderCudaAttrs=*/false))
        continue;

      // At this point, the constructors have the same signature ignoring CUDA
      // attributes. Decide whether their associated constraints are also the
      // same; only in that case do we treat one guide as a duplicate of the
      // other.
      SmallVector<AssociatedConstraint, 4> ExistingACs;
      ExistingCtor->getAssociatedConstraints(ExistingACs);

      if (HaveSameAssociatedConstraints(SemaRef, ExistingCtor, ExistingACs,
                                        Ctor, NewACs))
        return Existing;
    }
  }

  auto *Guide = CXXDeductionGuideDecl::Create(
      SemaRef.Context, DC, LocStart, ES, Name, GuideType, TInfo, LocEnd, Ctor,
      DeductionCandidate::Normal, FunctionTrailingRC);
  Guide->setImplicit(IsImplicit);
  Guide->setParams(Params);

  for (auto *Param : Params)
    Param->setDeclContext(Guide);
  for (auto *TD : MaterializedTypedefs)
    TD->setDeclContext(Guide);
  if (isa<CXXRecordDecl>(DC))
    Guide->setAccess(AS_public);

  if (!TemplateParams) {
    DC->addDecl(Guide);
    return Guide;
  }

  auto *GuideTemplate = FunctionTemplateDecl::Create(
      SemaRef.Context, DC, Loc, DeductionGuideName, TemplateParams, Guide);
  GuideTemplate->setImplicit(IsImplicit);
  Guide->setDescribedFunctionTemplate(GuideTemplate);

  if (isa<CXXRecordDecl>(DC))
    GuideTemplate->setAccess(AS_public);

  DC->addDecl(GuideTemplate);
  return GuideTemplate;
}

// Transform a given template type parameter `TTP`.
TemplateTypeParmDecl *transformTemplateTypeParam(
    Sema &SemaRef, DeclContext *DC, TemplateTypeParmDecl *TTP,
    MultiLevelTemplateArgumentList &Args, unsigned NewDepth, unsigned NewIndex,
    bool EvaluateConstraint) {
  // TemplateTypeParmDecl's index cannot be changed after creation, so
  // substitute it directly.
  auto *NewTTP = TemplateTypeParmDecl::Create(
      SemaRef.Context, DC, TTP->getBeginLoc(), TTP->getLocation(), NewDepth,
      NewIndex, TTP->getIdentifier(), TTP->wasDeclaredWithTypename(),
      TTP->isParameterPack(), TTP->hasTypeConstraint(),
      TTP->getNumExpansionParameters());
  if (const auto *TC = TTP->getTypeConstraint())
    SemaRef.SubstTypeConstraint(NewTTP, TC, Args,
                                /*EvaluateConstraint=*/EvaluateConstraint);
  if (TTP->hasDefaultArgument()) {
    TemplateArgumentLoc InstantiatedDefaultArg;
    if (!SemaRef.SubstTemplateArgument(
            TTP->getDefaultArgument(), Args, InstantiatedDefaultArg,
            TTP->getDefaultArgumentLoc(), TTP->getDeclName()))
      NewTTP->setDefaultArgument(SemaRef.Context, InstantiatedDefaultArg);
  }
  SemaRef.CurrentInstantiationScope->InstantiatedLocal(TTP, NewTTP);
  return NewTTP;
}
// Similar to above, but for non-type template or template template parameters.
template <typename NonTypeTemplateOrTemplateTemplateParmDecl>
NonTypeTemplateOrTemplateTemplateParmDecl *
transformTemplateParam(Sema &SemaRef, DeclContext *DC,
                       NonTypeTemplateOrTemplateTemplateParmDecl *OldParam,
                       MultiLevelTemplateArgumentList &Args, unsigned NewIndex,
                       unsigned NewDepth) {
  // Ask the template instantiator to do the heavy lifting for us, then adjust
  // the index of the parameter once it's done.
  auto *NewParam = cast<NonTypeTemplateOrTemplateTemplateParmDecl>(
      SemaRef.SubstDecl(OldParam, DC, Args));
  NewParam->setPosition(NewIndex);
  NewParam->setDepth(NewDepth);
  return NewParam;
}

NamedDecl *transformTemplateParameter(Sema &SemaRef, DeclContext *DC,
                                      NamedDecl *TemplateParam,
                                      MultiLevelTemplateArgumentList &Args,
                                      unsigned NewIndex, unsigned NewDepth,
                                      bool EvaluateConstraint = true) {
  if (auto *TTP = dyn_cast<TemplateTypeParmDecl>(TemplateParam))
    return transformTemplateTypeParam(
        SemaRef, DC, TTP, Args, NewDepth, NewIndex,
        /*EvaluateConstraint=*/EvaluateConstraint);
  if (auto *TTP = dyn_cast<TemplateTemplateParmDecl>(TemplateParam))
    return transformTemplateParam(SemaRef, DC, TTP, Args, NewIndex, NewDepth);
  if (auto *NTTP = dyn_cast<NonTypeTemplateParmDecl>(TemplateParam))
    return transformTemplateParam(SemaRef, DC, NTTP, Args, NewIndex, NewDepth);
  llvm_unreachable("Unhandled template parameter types");
}

/// Transform to convert portions of a constructor declaration into the
/// corresponding deduction guide, per C++1z [over.match.class.deduct]p1.
struct ConvertConstructorToDeductionGuideTransform {
  ConvertConstructorToDeductionGuideTransform(Sema &S,
                                              ClassTemplateDecl *Template)
      : SemaRef(S), Template(Template) {
    // If the template is nested, then we need to use the original
    // pattern to iterate over the constructors.
    ClassTemplateDecl *Pattern = Template;
    while (Pattern->getInstantiatedFromMemberTemplate()) {
      if (Pattern->isMemberSpecialization())
        break;
      Pattern = Pattern->getInstantiatedFromMemberTemplate();
      NestedPattern = Pattern;
    }

    if (NestedPattern)
      OuterInstantiationArgs = SemaRef.getTemplateInstantiationArgs(Template);
  }

  Sema &SemaRef;
  ClassTemplateDecl *Template;
  ClassTemplateDecl *NestedPattern = nullptr;

  DeclContext *DC = Template->getDeclContext();
  CXXRecordDecl *Primary = Template->getTemplatedDecl();
  DeclarationName DeductionGuideName =
      SemaRef.Context.DeclarationNames.getCXXDeductionGuideName(Template);

  QualType DeducedType = SemaRef.Context.getCanonicalTagType(Primary);

  // Index adjustment to apply to convert depth-1 template parameters into
  // depth-0 template parameters.
  unsigned Depth1IndexAdjustment = Template->getTemplateParameters()->size();

  // Instantiation arguments for the outermost depth-1 templates
  // when the template is nested
  MultiLevelTemplateArgumentList OuterInstantiationArgs;

  /// Transform a constructor declaration into a deduction guide.
  NamedDecl *transformConstructor(FunctionTemplateDecl *FTD,
                                  CXXConstructorDecl *CD) {
    SmallVector<TemplateArgument, 16> SubstArgs;

    LocalInstantiationScope Scope(SemaRef);

    // C++ [over.match.class.deduct]p1:
    // -- For each constructor of the class template designated by the
    //    template-name, a function template with the following properties:

    //    -- The template parameters are the template parameters of the class
    //       template followed by the template parameters (including default
    //       template arguments) of the constructor, if any.
    TemplateParameterList *TemplateParams =
        SemaRef.GetTemplateParameterList(Template);
    SmallVector<TemplateArgument, 16> Depth1Args;
    AssociatedConstraint OuterRC(TemplateParams->getRequiresClause());
    if (FTD) {
      TemplateParameterList *InnerParams = FTD->getTemplateParameters();
      SmallVector<NamedDecl *, 16> AllParams;
      AllParams.reserve(TemplateParams->size() + InnerParams->size());
      AllParams.insert(AllParams.begin(), TemplateParams->begin(),
                       TemplateParams->end());
      SubstArgs.reserve(InnerParams->size());
      Depth1Args.reserve(InnerParams->size());

      // Later template parameters could refer to earlier ones, so build up
      // a list of substituted template arguments as we go.
      for (NamedDecl *Param : *InnerParams) {
        MultiLevelTemplateArgumentList Args;
        Args.setKind(TemplateSubstitutionKind::Rewrite);
        Args.addOuterTemplateArguments(Depth1Args);
        Args.addOuterRetainedLevel();
        if (NestedPattern)
          Args.addOuterRetainedLevels(NestedPattern->getTemplateDepth());
        auto [Depth, Index] = getDepthAndIndex(Param);
        // Depth can be 0 if FTD belongs to a non-template class/a class
        // template specialization with an empty template parameter list. In
        // that case, we don't want the NewDepth to overflow, and it should
        // remain 0.
        NamedDecl *NewParam = transformTemplateParameter(
            SemaRef, DC, Param, Args, Index + Depth1IndexAdjustment,
            Depth ? Depth - 1 : 0);
        if (!NewParam)
          return nullptr;
        // Constraints require that we substitute depth-1 arguments
        // to match depths when substituted for evaluation later
        Depth1Args.push_back(SemaRef.Context.getInjectedTemplateArg(NewParam));

        if (NestedPattern) {
          auto [Depth, Index] = getDepthAndIndex(NewParam);
          NewParam = transformTemplateParameter(
              SemaRef, DC, NewParam, OuterInstantiationArgs, Index,
              Depth - OuterInstantiationArgs.getNumSubstitutedLevels(),
              /*EvaluateConstraint=*/false);
        }

        assert(getDepthAndIndex(NewParam).first == 0 &&
               "Unexpected template parameter depth");

        AllParams.push_back(NewParam);
        SubstArgs.push_back(SemaRef.Context.getInjectedTemplateArg(NewParam));
      }

      // Substitute new template parameters into requires-clause if present.
      Expr *RequiresClause = nullptr;
      if (Expr *InnerRC = InnerParams->getRequiresClause()) {
        MultiLevelTemplateArgumentList Args;
        Args.setKind(TemplateSubstitutionKind::Rewrite);
        Args.addOuterTemplateArguments(Depth1Args);
        Args.addOuterRetainedLevel();
        if (NestedPattern)
          Args.addOuterRetainedLevels(NestedPattern->getTemplateDepth());
        ExprResult E =
            SemaRef.SubstConstraintExprWithoutSatisfaction(InnerRC, Args);
        if (!E.isUsable())
          return nullptr;
        RequiresClause = E.get();
      }

      TemplateParams = TemplateParameterList::Create(
          SemaRef.Context, InnerParams->getTemplateLoc(),
          InnerParams->getLAngleLoc(), AllParams, InnerParams->getRAngleLoc(),
          RequiresClause);
    }

    // If we built a new template-parameter-list, track that we need to
    // substitute references to the old parameters into references to the
    // new ones.
    MultiLevelTemplateArgumentList Args;
    Args.setKind(TemplateSubstitutionKind::Rewrite);
    if (FTD) {
      Args.addOuterTemplateArguments(SubstArgs);
      Args.addOuterRetainedLevel();
    }

    FunctionProtoTypeLoc FPTL = CD->getTypeSourceInfo()
                                    ->getTypeLoc()
                                    .getAsAdjusted<FunctionProtoTypeLoc>();
    assert(FPTL && "no prototype for constructor declaration");

    // Transform the type of the function, adjusting the return type and
    // replacing references to the old parameters with references to the
    // new ones.
    TypeLocBuilder TLB;
    SmallVector<ParmVarDecl *, 8> Params;
    SmallVector<TypedefNameDecl *, 4> MaterializedTypedefs;
    QualType NewType = transformFunctionProtoType(TLB, FPTL, Params, Args,
                                                  MaterializedTypedefs);
    if (NewType.isNull())
      return nullptr;
    TypeSourceInfo *NewTInfo = TLB.getTypeSourceInfo(SemaRef.Context, NewType);

    // At this point, the function parameters are already 'instantiated' in the
    // current scope. Substitute into the constructor's trailing
    // requires-clause, if any.
    AssociatedConstraint FunctionTrailingRC;
    if (const AssociatedConstraint &RC = CD->getTrailingRequiresClause()) {
      MultiLevelTemplateArgumentList Args;
      Args.setKind(TemplateSubstitutionKind::Rewrite);
      Args.addOuterTemplateArguments(Depth1Args);
      Args.addOuterRetainedLevel();
      if (NestedPattern)
        Args.addOuterRetainedLevels(NestedPattern->getTemplateDepth());
      ExprResult E = SemaRef.SubstConstraintExprWithoutSatisfaction(
          const_cast<Expr *>(RC.ConstraintExpr), Args);
      if (!E.isUsable())
        return nullptr;
      FunctionTrailingRC = AssociatedConstraint(E.get(), RC.ArgPackSubstIndex);
    }

    // C++ [over.match.class.deduct]p1:
    // If C is defined, for each constructor of C, a function template with
    // the following properties:
    // [...]
    // - The associated constraints are the conjunction of the associated
    // constraints of C and the associated constraints of the constructor, if
    // any.
    if (OuterRC) {
      // The outer template parameters are not transformed, so their
      // associated constraints don't need substitution.
      // FIXME: Should simply add another field for the OuterRC, instead of
      // combining them like this.
      if (!FunctionTrailingRC)
        FunctionTrailingRC = OuterRC;
      else
        FunctionTrailingRC = AssociatedConstraint(
            BinaryOperator::Create(
                SemaRef.Context,
                /*lhs=*/const_cast<Expr *>(OuterRC.ConstraintExpr),
                /*rhs=*/const_cast<Expr *>(FunctionTrailingRC.ConstraintExpr),
                BO_LAnd, SemaRef.Context.BoolTy, VK_PRValue, OK_Ordinary,
                TemplateParams->getTemplateLoc(), FPOptionsOverride()),
            FunctionTrailingRC.ArgPackSubstIndex);
    }

    return buildDeductionGuide(
        SemaRef, Template, TemplateParams, CD, CD->getExplicitSpecifier(),
        NewTInfo, CD->getBeginLoc(), CD->getLocation(), CD->getEndLoc(),
        /*IsImplicit=*/true, MaterializedTypedefs, FunctionTrailingRC);
  }

  /// Build a deduction guide with the specified parameter types.
  NamedDecl *buildSimpleDeductionGuide(MutableArrayRef<QualType> ParamTypes) {
    SourceLocation Loc = Template->getLocation();

    // Build the requested type.
    FunctionProtoType::ExtProtoInfo EPI;
    EPI.HasTrailingReturn = true;
    QualType Result = SemaRef.BuildFunctionType(DeducedType, ParamTypes, Loc,
                                                DeductionGuideName, EPI);
    TypeSourceInfo *TSI = SemaRef.Context.getTrivialTypeSourceInfo(Result, Loc);
    if (NestedPattern)
      TSI = SemaRef.SubstType(TSI, OuterInstantiationArgs, Loc,
                              DeductionGuideName);

    if (!TSI)
      return nullptr;

    FunctionProtoTypeLoc FPTL =
        TSI->getTypeLoc().castAs<FunctionProtoTypeLoc>();

    // Build the parameters, needed during deduction / substitution.
    SmallVector<ParmVarDecl *, 4> Params;
    for (auto T : ParamTypes) {
      auto *TSI = SemaRef.Context.getTrivialTypeSourceInfo(T, Loc);
      if (NestedPattern)
        TSI = SemaRef.SubstType(TSI, OuterInstantiationArgs, Loc,
                                DeclarationName());
      if (!TSI)
        return nullptr;

      ParmVarDecl *NewParam =
          ParmVarDecl::Create(SemaRef.Context, DC, Loc, Loc, nullptr,
                              TSI->getType(), TSI, SC_None, nullptr);
      NewParam->setScopeInfo(0, Params.size());
      FPTL.setParam(Params.size(), NewParam);
      Params.push_back(NewParam);
    }

    return buildDeductionGuide(
        SemaRef, Template, SemaRef.GetTemplateParameterList(Template), nullptr,
        ExplicitSpecifier(), TSI, Loc, Loc, Loc, /*IsImplicit=*/true);
  }

private:
  QualType transformFunctionProtoType(
      TypeLocBuilder &TLB, FunctionProtoTypeLoc TL,
      SmallVectorImpl<ParmVarDecl *> &Params,
      MultiLevelTemplateArgumentList &Args,
      SmallVectorImpl<TypedefNameDecl *> &MaterializedTypedefs) {
    SmallVector<QualType, 4> ParamTypes;
    const FunctionProtoType *T = TL.getTypePtr();

    //    -- The types of the function parameters are those of the constructor.
    for (auto *OldParam : TL.getParams()) {
      ParmVarDecl *NewParam = OldParam;
      // Given
      //   template <class T> struct C {
      //     template <class U> struct D {
      //       template <class V> D(U, V);
      //     };
      //   };
      // First, transform all the references to template parameters that are
      // defined outside of the surrounding class template. That is T in the
      // above example.
      if (NestedPattern) {
        NewParam = transformFunctionTypeParam(
            NewParam, OuterInstantiationArgs, MaterializedTypedefs,
            /*TransformingOuterPatterns=*/true);
        if (!NewParam)
          return QualType();
      }
      // Then, transform all the references to template parameters that are
      // defined at the class template and the constructor. In this example,
      // they're U and V, respectively.
      NewParam =
          transformFunctionTypeParam(NewParam, Args, MaterializedTypedefs,
                                     /*TransformingOuterPatterns=*/false);
      if (!NewParam)
        return QualType();
      ParamTypes.push_back(NewParam->getType());
      Params.push_back(NewParam);
    }

    //    -- The return type is the class template specialization designated by
    //       the template-name and template arguments corresponding to the
    //       template parameters obtained from the class template.
    //
    // We use the injected-class-name type of the primary template instead.
    // This has the convenient property that it is different from any type that
    // the user can write in a deduction-guide (because they cannot enter the
    // context of the template), so implicit deduction guides can never collide
    // with explicit ones.
    QualType ReturnType = DeducedType;
    auto TTL = TLB.push<TagTypeLoc>(ReturnType);
    TTL.setElaboratedKeywordLoc(SourceLocation());
    TTL.setQualifierLoc(NestedNameSpecifierLoc());
    TTL.setNameLoc(Primary->getLocation());

    // Resolving a wording defect, we also inherit the variadicness of the
    // constructor.
    FunctionProtoType::ExtProtoInfo EPI;
    EPI.Variadic = T->isVariadic();
    EPI.HasTrailingReturn = true;

    QualType Result = SemaRef.BuildFunctionType(
        ReturnType, ParamTypes, TL.getBeginLoc(), DeductionGuideName, EPI);
    if (Result.isNull())
      return QualType();

    FunctionProtoTypeLoc NewTL = TLB.push<FunctionProtoTypeLoc>(Result);
    NewTL.setLocalRangeBegin(TL.getLocalRangeBegin());
    NewTL.setLParenLoc(TL.getLParenLoc());
    NewTL.setRParenLoc(TL.getRParenLoc());
    NewTL.setExceptionSpecRange(SourceRange());
    NewTL.setLocalRangeEnd(TL.getLocalRangeEnd());
    for (unsigned I = 0, E = NewTL.getNumParams(); I != E; ++I)
      NewTL.setParam(I, Params[I]);

    return Result;
  }

  ParmVarDecl *transformFunctionTypeParam(
      ParmVarDecl *OldParam, MultiLevelTemplateArgumentList &Args,
      llvm::SmallVectorImpl<TypedefNameDecl *> &MaterializedTypedefs,
      bool TransformingOuterPatterns) {
    TypeSourceInfo *OldTSI = OldParam->getTypeSourceInfo();
    TypeSourceInfo *NewTSI;
    if (auto PackTL = OldTSI->getTypeLoc().getAs<PackExpansionTypeLoc>()) {
      // Expand out the one and only element in each inner pack.
      Sema::ArgPackSubstIndexRAII SubstIndex(SemaRef, 0u);
      NewTSI =
          SemaRef.SubstType(PackTL.getPatternLoc(), Args,
                            OldParam->getLocation(), OldParam->getDeclName());
      if (!NewTSI)
        return nullptr;
      NewTSI =
          SemaRef.CheckPackExpansion(NewTSI, PackTL.getEllipsisLoc(),
                                     PackTL.getTypePtr()->getNumExpansions());
    } else
      NewTSI = SemaRef.SubstType(OldTSI, Args, OldParam->getLocation(),
                                 OldParam->getDeclName());
    if (!NewTSI)
      return nullptr;

    // Extract the type. This (for instance) replaces references to typedef
    // members of the current instantiations with the definitions of those
    // typedefs, avoiding triggering instantiation of the deduced type during
    // deduction.
    NewTSI = ExtractTypeForDeductionGuide(
                 SemaRef, MaterializedTypedefs, NestedPattern,
                 TransformingOuterPatterns ? &Args : nullptr)
                 .transform(NewTSI);
    if (!NewTSI)
      return nullptr;
    // Resolving a wording defect, we also inherit default arguments from the
    // constructor.
    ExprResult NewDefArg;
    if (OldParam->hasDefaultArg()) {
      // We don't care what the value is (we won't use it); just create a
      // placeholder to indicate there is a default argument.
      QualType ParamTy = NewTSI->getType();
      NewDefArg = new (SemaRef.Context)
          OpaqueValueExpr(OldParam->getDefaultArgRange().getBegin(),
                          ParamTy.getNonLValueExprType(SemaRef.Context),
                          ParamTy->isLValueReferenceType()   ? VK_LValue
                          : ParamTy->isRValueReferenceType() ? VK_XValue
                                                             : VK_PRValue);
    }
    // Handle arrays and functions decay.
    auto NewType = NewTSI->getType();
    if (NewType->isArrayType() || NewType->isFunctionType())
      NewType = SemaRef.Context.getDecayedType(NewType);

    ParmVarDecl *NewParam = ParmVarDecl::Create(
        SemaRef.Context, DC, OldParam->getInnerLocStart(),
        OldParam->getLocation(), OldParam->getIdentifier(), NewType, NewTSI,
        OldParam->getStorageClass(), NewDefArg.get());
    NewParam->setScopeInfo(OldParam->getFunctionScopeDepth(),
                           OldParam->getFunctionScopeIndex());
    SemaRef.CurrentInstantiationScope->InstantiatedLocal(OldParam, NewParam);
    return NewParam;
  }
};

// Find all template parameters that appear in the given DeducedArgs.
// Return the indices of the template parameters in the TemplateParams.
SmallVector<unsigned> TemplateParamsReferencedInTemplateArgumentList(
    Sema &SemaRef, const TemplateParameterList *TemplateParamsList,
    ArrayRef<TemplateArgument> DeducedArgs) {

  llvm::SmallBitVector ReferencedTemplateParams(TemplateParamsList->size());
  SemaRef.MarkUsedTemplateParameters(
      DeducedArgs, TemplateParamsList->getDepth(), ReferencedTemplateParams);

  auto MarkDefaultArgs = [&](auto *Param) {
    if (!Param->hasDefaultArgument())
      return;
    SemaRef.MarkUsedTemplateParameters(
        Param->getDefaultArgument().getArgument(),
        TemplateParamsList->getDepth(), ReferencedTemplateParams);
  };

  for (unsigned Index = 0; Index < TemplateParamsList->size(); ++Index) {
    if (!ReferencedTemplateParams[Index])
      continue;
    auto *Param = TemplateParamsList->getParam(Index);
    if (auto *TTPD = dyn_cast<TemplateTypeParmDecl>(Param))
      MarkDefaultArgs(TTPD);
    else if (auto *NTTPD = dyn_cast<NonTypeTemplateParmDecl>(Param))
      MarkDefaultArgs(NTTPD);
    else
      MarkDefaultArgs(cast<TemplateTemplateParmDecl>(Param));
  }

  SmallVector<unsigned> Results;
  for (unsigned Index = 0; Index < TemplateParamsList->size(); ++Index) {
    if (ReferencedTemplateParams[Index])
      Results.push_back(Index);
  }
  return Results;
}

bool hasDeclaredDeductionGuides(DeclarationName Name, DeclContext *DC) {
  // Check whether we've already declared deduction guides for this template.
  // FIXME: Consider storing a flag on the template to indicate this.
  assert(Name.getNameKind() ==
             DeclarationName::NameKind::CXXDeductionGuideName &&
         "name must be a deduction guide name");
  auto Existing = DC->lookup(Name);
  for (auto *D : Existing)
    if (D->isImplicit())
      return true;
  return false;
}

// Returns all source deduction guides associated with the declared
// deduction guides that have the specified deduction guide name.
llvm::DenseSet<const NamedDecl *> getSourceDeductionGuides(DeclarationName Name,
                                                           DeclContext *DC) {
  assert(Name.getNameKind() ==
             DeclarationName::NameKind::CXXDeductionGuideName &&
         "name must be a deduction guide name");
  llvm::DenseSet<const NamedDecl *> Result;
  for (auto *D : DC->lookup(Name)) {
    if (const auto *FTD = dyn_cast<FunctionTemplateDecl>(D))
      D = FTD->getTemplatedDecl();

    if (const auto *GD = dyn_cast<CXXDeductionGuideDecl>(D)) {
      assert(GD->getSourceDeductionGuide() &&
             "deduction guide for alias template must have a source deduction "
             "guide");
      Result.insert(GD->getSourceDeductionGuide());
    }
  }
  return Result;
}

// The source deduction guides of the guides named \p Name that were generated
// from inherited constructors through the using-declaration designated by
// \p UsingLoc.
//
// Guides generated for a using-declaration are declared at the location of the
// base class it names (see
// DeclareImplicitDeductionGuidesFromInheritedConstructors), which identifies
// that using-declaration. We cannot instead recover it from the return type of
// the guide, because that has been substituted with the source guide's return
// type and may no longer name the helper class template.
class InheritedSourceDeductionGuides {
  ASTContext &Context;
  SmallVector<const CXXDeductionGuideDecl *, 8> Sources;

  static const CXXDeductionGuideDecl *getGuide(const NamedDecl *D) {
    if (const auto *FTD = dyn_cast<FunctionTemplateDecl>(D))
      D = FTD->getTemplatedDecl();
    return dyn_cast<CXXDeductionGuideDecl>(D);
  }

public:
  InheritedSourceDeductionGuides(Sema &SemaRef, DeclarationName Name,
                                 DeclContext *DC, SourceLocation UsingLoc)
      : Context(SemaRef.Context) {
    assert(Name.getNameKind() ==
               DeclarationName::NameKind::CXXDeductionGuideName &&
           "name must be a deduction guide name");
    for (auto *D : DC->lookup(Name)) {
      const CXXDeductionGuideDecl *GD = getGuide(D);
      if (GD &&
          GD->getSourceDeductionGuideKind() ==
              CXXDeductionGuideDecl::SourceDeductionGuideKind::
                  InheritedConstructor &&
          GD->getLocation() == UsingLoc)
        Sources.push_back(GD->getSourceDeductionGuide());
    }
  }

  // Whether an inherited guide was already generated from \p D.
  //
  // The guides of the base class can be declared more than once, e.g. when a
  // module that already declared guides for both classes is imported and the
  // base is not exported, so they are compared structurally rather than by
  // identity.
  bool contains(const NamedDecl *D) const {
    const CXXDeductionGuideDecl *DG = getGuide(D);
    if (!DG)
      return false;
    return llvm::any_of(Sources, [&](const CXXDeductionGuideDecl *Source) {
      return Source == DG ||
             (Source->getLocation() == DG->getLocation() &&
              Source->getDescribedFunctionTemplate() &&
              DG->getDescribedFunctionTemplate() &&
              Context.hasSameType(Source->getType(), DG->getType()));
    });
  }
};

// Build the associated constraints for the alias deduction guides.
// C++ [over.match.class.deduct]p3.3:
//   The associated constraints ([temp.constr.decl]) are the conjunction of the
//   associated constraints of g and a constraint that is satisfied if and only
//   if the arguments of A are deducible (see below) from the return type.
//
// The return result is expected to be the require-clause for the synthesized
// alias deduction guide.
Expr *
buildAssociatedConstraints(Sema &SemaRef, FunctionTemplateDecl *F,
                           TypeAliasTemplateDecl *AliasTemplate,
                           ArrayRef<DeducedTemplateArgument> DeduceResults,
                           unsigned FirstUndeducedParamIdx, Expr *IsDeducible) {
  Expr *RC = F->getTemplateParameters()->getRequiresClause();
  if (!RC)
    return IsDeducible;

  ASTContext &Context = SemaRef.Context;
  LocalInstantiationScope Scope(SemaRef);

  // In the clang AST, constraint nodes are deliberately not instantiated unless
  // they are actively being evaluated. Consequently, occurrences of template
  // parameters in the require-clause expression have a subtle "depth"
  // difference compared to normal occurrences in places, such as function
  // parameters. When transforming the require-clause, we must take this
  // distinction into account:
  //
  //   1) In the transformed require-clause, occurrences of template parameters
  //   must use the "uninstantiated" depth;
  //   2) When substituting on the require-clause expr of the underlying
  //   deduction guide, we must use the entire set of template argument lists;
  //
  // It's important to note that we're performing this transformation on an
  // *instantiated* AliasTemplate.

  // For 1), if the alias template is nested within a class template, we
  // calcualte the 'uninstantiated' depth by adding the substitution level back.
  unsigned AdjustDepth = 0;
  if (auto *PrimaryTemplate =
          AliasTemplate->getInstantiatedFromMemberTemplate())
    AdjustDepth = PrimaryTemplate->getTemplateDepth();

  // We rebuild all template parameters with the uninstantiated depth, and
  // build template arguments refer to them.
  SmallVector<TemplateArgument> AdjustedAliasTemplateArgs;

  for (auto *TP : *AliasTemplate->getTemplateParameters()) {
    // Rebuild any internal references to earlier parameters and reindex
    // as we go.
    MultiLevelTemplateArgumentList Args;
    Args.setKind(TemplateSubstitutionKind::Rewrite);
    Args.addOuterTemplateArguments(AdjustedAliasTemplateArgs);
    NamedDecl *NewParam = transformTemplateParameter(
        SemaRef, AliasTemplate->getDeclContext(), TP, Args,
        /*NewIndex=*/AdjustedAliasTemplateArgs.size(),
        getDepthAndIndex(TP).first + AdjustDepth);

    TemplateArgument NewTemplateArgument =
        Context.getInjectedTemplateArg(NewParam);
    AdjustedAliasTemplateArgs.push_back(NewTemplateArgument);
  }
  // Template arguments used to transform the template arguments in
  // DeducedResults.
  SmallVector<TemplateArgument> TemplateArgsForBuildingRC(
      F->getTemplateParameters()->size());
  // Transform the transformed template args
  MultiLevelTemplateArgumentList Args;
  Args.setKind(TemplateSubstitutionKind::Rewrite);
  Args.addOuterTemplateArguments(AdjustedAliasTemplateArgs);

  for (unsigned Index = 0; Index < DeduceResults.size(); ++Index) {
    const auto &D = DeduceResults[Index];
    if (D.isNull()) { // non-deduced template parameters of f
      NamedDecl *TP = F->getTemplateParameters()->getParam(Index);
      MultiLevelTemplateArgumentList Args;
      Args.setKind(TemplateSubstitutionKind::Rewrite);
      Args.addOuterTemplateArguments(TemplateArgsForBuildingRC);
      // Rebuild the template parameter with updated depth and index.
      NamedDecl *NewParam =
          transformTemplateParameter(SemaRef, F->getDeclContext(), TP, Args,
                                     /*NewIndex=*/FirstUndeducedParamIdx,
                                     getDepthAndIndex(TP).first + AdjustDepth);
      FirstUndeducedParamIdx += 1;
      assert(TemplateArgsForBuildingRC[Index].isNull());
      TemplateArgsForBuildingRC[Index] =
          Context.getInjectedTemplateArg(NewParam);
      continue;
    }
    TemplateArgumentLoc Input =
        SemaRef.getTrivialTemplateArgumentLoc(D, QualType(), SourceLocation{});
    TemplateArgumentLoc Output;
    if (!SemaRef.SubstTemplateArgument(Input, Args, Output)) {
      assert(TemplateArgsForBuildingRC[Index].isNull() &&
             "InstantiatedArgs must be null before setting");
      TemplateArgsForBuildingRC[Index] = Output.getArgument();
    }
  }

  // A list of template arguments for transforming the require-clause of F.
  // It must contain the entire set of template argument lists.
  MultiLevelTemplateArgumentList ArgsForBuildingRC;
  ArgsForBuildingRC.setKind(clang::TemplateSubstitutionKind::Rewrite);
  ArgsForBuildingRC.addOuterTemplateArguments(TemplateArgsForBuildingRC);
  // For 2), if the underlying deduction guide F is nested in a class template,
  // we need the entire template argument list, as the constraint AST in the
  // require-clause of F remains completely uninstantiated.
  //
  // For example:
  //   template <typename T> // depth 0
  //   struct Outer {
  //      template <typename U>
  //      struct Foo { Foo(U); };
  //
  //      template <typename U> // depth 1
  //      requires C<U>
  //      Foo(U) -> Foo<int>;
  //   };
  //   template <typename U>
  //   using AFoo = Outer<int>::Foo<U>;
  //
  // In this scenario, the deduction guide for `Foo` inside `Outer<int>`:
  //   - The occurrence of U in the require-expression is [depth:1, index:0]
  //   - The occurrence of U in the function parameter is [depth:0, index:0]
  //   - The template parameter of U is [depth:0, index:0]
  //
  // We add the outer template arguments which is [int] to the multi-level arg
  // list to ensure that the occurrence U in `C<U>` will be replaced with int
  // during the substitution.
  //
  // NOTE: The underlying deduction guide F is instantiated -- either from an
  // explicitly-written deduction guide member, or from a constructor.
  // getInstantiatedFromMemberTemplate() can only handle the former case, so we
  // check the DeclContext kind.
  if (F->getLexicalDeclContext()->getDeclKind() ==
      clang::Decl::ClassTemplateSpecialization) {
    auto OuterLevelArgs = SemaRef.getTemplateInstantiationArgs(
        F, F->getLexicalDeclContext(),
        /*Final=*/false, /*Innermost=*/std::nullopt,
        /*RelativeToPrimary=*/true,
        /*Pattern=*/nullptr,
        /*ForConstraintInstantiation=*/true);
    for (auto It : OuterLevelArgs)
      ArgsForBuildingRC.addOuterTemplateArguments(It.Args);
  }

  ExprResult E = SemaRef.SubstExpr(RC, ArgsForBuildingRC);
  if (E.isInvalid())
    return nullptr;
  if (!IsDeducible)
    return E.getAs<Expr>();

  auto Conjunction =
      SemaRef.BuildBinOp(SemaRef.getCurScope(), SourceLocation{},
                         BinaryOperatorKind::BO_LAnd, E.get(), IsDeducible);
  if (Conjunction.isInvalid())
    return nullptr;
  return Conjunction.getAs<Expr>();
}
// Build the is_deducible constraint for the alias deduction guides.
// [over.match.class.deduct]p3.3:
//    ... and a constraint that is satisfied if and only if the arguments
//    of A are deducible (see below) from the return type.
Expr *buildIsDeducibleConstraint(Sema &SemaRef,
                                 TypeAliasTemplateDecl *AliasTemplate,
                                 QualType ReturnType,
                                 SmallVector<NamedDecl *> TemplateParams) {
  ASTContext &Context = SemaRef.Context;
  // Constraint AST nodes must use uninstantiated depth.
  if (auto *PrimaryTemplate =
          AliasTemplate->getInstantiatedFromMemberTemplate();
      PrimaryTemplate && TemplateParams.size() > 0) {
    LocalInstantiationScope Scope(SemaRef);

    // Adjust the depth for TemplateParams.
    unsigned AdjustDepth = PrimaryTemplate->getTemplateDepth();
    SmallVector<TemplateArgument> TransformedTemplateArgs;
    for (auto *TP : TemplateParams) {
      // Rebuild any internal references to earlier parameters and reindex
      // as we go.
      MultiLevelTemplateArgumentList Args;
      Args.setKind(TemplateSubstitutionKind::Rewrite);
      Args.addOuterTemplateArguments(TransformedTemplateArgs);
      NamedDecl *NewParam = transformTemplateParameter(
          SemaRef, AliasTemplate->getDeclContext(), TP, Args,
          /*NewIndex=*/TransformedTemplateArgs.size(),
          getDepthAndIndex(TP).first + AdjustDepth);

      TemplateArgument NewTemplateArgument =
          Context.getInjectedTemplateArg(NewParam);
      TransformedTemplateArgs.push_back(NewTemplateArgument);
    }
    // Transformed the ReturnType to restore the uninstantiated depth.
    MultiLevelTemplateArgumentList Args;
    Args.setKind(TemplateSubstitutionKind::Rewrite);
    Args.addOuterTemplateArguments(TransformedTemplateArgs);
    ReturnType = SemaRef.SubstType(
        ReturnType, Args, AliasTemplate->getLocation(),
        Context.DeclarationNames.getCXXDeductionGuideName(AliasTemplate));
  }

  SmallVector<TypeSourceInfo *> IsDeducibleTypeTraitArgs = {
      Context.getTrivialTypeSourceInfo(
          Context.getDeducedTemplateSpecializationType(
              ElaboratedTypeKeyword::None, TemplateName(AliasTemplate),
              /*DeducedType=*/QualType(),
              /*IsDependent=*/true),
          AliasTemplate->getLocation()), // template specialization type whose
                                         // arguments will be deduced.
      Context.getTrivialTypeSourceInfo(
          ReturnType, AliasTemplate->getLocation()), // type from which template
                                                     // arguments are deduced.
  };
  return TypeTraitExpr::Create(
      Context, Context.getLogicalOperationType(), AliasTemplate->getLocation(),
      TypeTrait::BTT_IsDeducible, IsDeducibleTypeTraitArgs,
      AliasTemplate->getLocation(), /*Value*/ false);
}

std::pair<TemplateDecl *, llvm::ArrayRef<TemplateArgument>>
getRHSTemplateDeclAndArgs(Sema &SemaRef, TypeAliasTemplateDecl *AliasTemplate) {
  auto RhsType = AliasTemplate->getTemplatedDecl()->getUnderlyingType();
  TemplateDecl *Template = nullptr;
  llvm::ArrayRef<TemplateArgument> AliasRhsTemplateArgs;
  if (const auto *TST = RhsType->getAs<TemplateSpecializationType>()) {
    // Cases where the RHS of the alias is dependent. e.g.
    //   template<typename T>
    //   using AliasFoo1 = Foo<T>; // a class/type alias template specialization
    Template = TST->getTemplateName().getAsTemplateDecl();
    AliasRhsTemplateArgs =
        TST->getAsNonAliasTemplateSpecializationType()->template_arguments();
  } else if (const auto *RT = RhsType->getAs<RecordType>()) {
    // Cases where template arguments in the RHS of the alias are not
    // dependent. e.g.
    //   using AliasFoo = Foo<bool>;
    if (const auto *CTSD =
            dyn_cast<ClassTemplateSpecializationDecl>(RT->getDecl())) {
      Template = CTSD->getSpecializedTemplate();
      AliasRhsTemplateArgs = CTSD->getTemplateArgs().asArray();
    }
  }
  return {Template, AliasRhsTemplateArgs};
}

bool IsNonDeducedArgument(const TemplateArgument &TA) {
  // The following cases indicate the template argument is non-deducible:
  //   1. The result is null. E.g. When it comes from a default template
  //   argument that doesn't appear in the alias declaration.
  //   2. The template parameter is a pack and that cannot be deduced from
  //   the arguments within the alias declaration.
  // Non-deducible template parameters will persist in the transformed
  // deduction guide.
  return TA.isNull() ||
         (TA.getKind() == TemplateArgument::Pack &&
          llvm::any_of(TA.pack_elements(), IsNonDeducedArgument));
}

struct InheritedConstructorDeductionInfo {
  // Class template for which we are declaring deduction guides.
  // This is `C` in the standard wording.
  TemplateDecl *DerivedClassTemplate;

  // `typename CC<R>::type` in the standard wording, with the template argument
  // of CC being the type parameter that is substituted with the return type R
  // of each guide.
  TypeSourceInfo *CCType;
};

// Build the function type and return type for a deduction guide generated from
// an inherited constructor C++23 [over.match.class.deduct]p1.10:
// ... the set contains the guides of A with the return type R
// of each guide replaced with `typename CC<R>::type` ...
std::pair<TypeSourceInfo *, QualType>
buildInheritedConstructorDeductionGuideType(
    Sema &SemaRef, const InheritedConstructorDeductionInfo &Info,
    TypeSourceInfo *SourceGuideTSI) {
  ASTContext &Context = SemaRef.Context;
  const auto *FPT = SourceGuideTSI->getType()->getAs<FunctionProtoType>();
  assert(FPT && "Source Guide type should be a FunctionProtoType");

  // This substitution can fail in cases where the source return type
  // is not dependent and the derived class is not deducible.
  // FIXME: There is currently no diagnostic emitted in this case, as it is
  // nontrivial to propagate substitution failure messages up to the point where
  // deduction guides are used -- we do not have a type with which we can create
  // a deduction guide AST node and must encode the SFINAE message.
  Sema::SFINAETrap Trap(SemaRef);

  MultiLevelTemplateArgumentList Args;
  Args.addOuterTemplateArguments(Info.DerivedClassTemplate,
                                 TemplateArgument(FPT->getReturnType()),
                                 /*Final=*/false);
  Args.addOuterRetainedLevels(Info.DerivedClassTemplate->getTemplateDepth());
  TypeSourceInfo *ReturnTypeTSI = SemaRef.SubstType(
      Info.CCType, Args, Info.DerivedClassTemplate->getBeginLoc(),
      DeclarationName());
  if (!ReturnTypeTSI || Trap.hasErrorOccurred())
    return {nullptr, QualType()};
  QualType ReturnType = ReturnTypeTSI->getType();

  TypeLocBuilder TLB;
  TLB.pushFullCopy(ReturnTypeTSI->getTypeLoc());

  QualType FT = Context.getFunctionType(ReturnType, FPT->getParamTypes(),
                                        FPT->getExtProtoInfo());
  FunctionProtoTypeLoc NewTL = TLB.push<FunctionProtoTypeLoc>(FT);
  const FunctionProtoTypeLoc &TL =
      SourceGuideTSI->getTypeLoc().getAs<FunctionProtoTypeLoc>();
  NewTL.setLocalRangeBegin(TL.getLocalRangeBegin());
  NewTL.setLParenLoc(TL.getLParenLoc());
  NewTL.setRParenLoc(TL.getRParenLoc());
  NewTL.setExceptionSpecRange(TL.getExceptionSpecRange());
  NewTL.setLocalRangeEnd(TL.getLocalRangeEnd());
  for (unsigned I = 0, E = NewTL.getNumParams(); I != E; ++I)
    NewTL.setParam(I, TL.getParam(I));

  TypeSourceInfo *DGuideType = TLB.getTypeSourceInfo(Context, FT);
  return {DGuideType, ReturnType};
}

// Get the template arguments of the class template specialization named by the
// return type of the deduction guide \p DG. For a guide generated from an
// inherited constructor, whose return type has the form
// `typename CC<R>::type`, these are the template arguments of the derived
// template, recovered from the (only) partial specialization of CC.
static ArrayRef<TemplateArgument>
getTemplateArgumentsFromDeductionGuideReturnType(ASTContext &Context,
                                                 CXXDeductionGuideDecl *DG) {
  QualType RType = DG->getReturnType();

  if (const auto *TST = RType->getAs<TemplateSpecializationType>())
    return TST->template_arguments();

  // implicitly-generated deduction guide.
  if (const auto *ICNT = RType->getAsCanonical<InjectedClassNameType>())
    return cast<TemplateSpecializationType>(
               ICNT->getDecl()->getCanonicalTemplateSpecializationType(Context))
        ->template_arguments();

  // inherited constructor deduction guide.
  if (const auto *DNT = RType->getAs<DependentNameType>()) {
    // This is of the form `typename CC<Base<...>>::type`. We need to extract
    // the template arguments from the CC partial specialization, which are the
    // template arguments of the derived template.
    const Type *QualifierType = DNT->getQualifier().getAsType();
    assert(QualifierType && "Expected an inherited ctor deduction guide to "
                            "have a type stored specifier");

    const auto *CCSpecializationType =
        QualifierType->getAs<TemplateSpecializationType>();
    assert(CCSpecializationType &&
           "Expected an inherited ctor deduction guide to have a "
           "TemplateSpecializationType qualifier");

    const auto *TD = cast<ClassTemplateDecl>(
        CCSpecializationType->getTemplateName().getAsTemplateDecl());
    SmallVector<ClassTemplatePartialSpecializationDecl *, 1> PS;
    TD->getPartialSpecializations(PS);
    assert(PS.size() == 1 &&
           "Expected the CC template for inherited ctor deduction guide to "
           "have a single partial specialization");

    return PS[0]->getTemplateParameters()->getInjectedTemplateArgs(Context);
  }

  llvm_unreachable("Unhandled deduction guide return type");
}

// Build deduction guides for a type alias template from the given underlying
// deduction guide F.
// If F is synthesized from a base class (as an inherited constructor), then the
// return type will be transformed using FromInheritedCtor->CCType. The
// resulting deduction guide is added to
// FromInheritedCtor->DerivedClassTemplate, as opposed to the given
// AliasTemplate.
FunctionTemplateDecl *BuildDeductionGuideForTypeAlias(
    Sema &SemaRef, TypeAliasTemplateDecl *AliasTemplate,
    FunctionTemplateDecl *F, SourceLocation Loc,
    InheritedConstructorDeductionInfo *FromInheritedCtor = nullptr) {
  LocalInstantiationScope Scope(SemaRef);
  Sema::NonSFINAEContext _1(SemaRef);
  Sema::InstantiatingTemplate BuildingDeductionGuides(
      SemaRef, AliasTemplate->getLocation(), F,
      Sema::InstantiatingTemplate::BuildingDeductionGuidesTag{});
  if (BuildingDeductionGuides.isInvalid())
    return nullptr;

  auto &Context = SemaRef.Context;
  auto [Template, AliasRhsTemplateArgs] =
      getRHSTemplateDeclAndArgs(SemaRef, AliasTemplate);

  // We need both types desugared, before we continue to perform type deduction.
  // The intent is to get the template argument list 'matched', e.g. in the
  // following case:
  //
  //
  //  template <class T>
  //  struct A {};
  //  template <class T>
  //  using Foo = A<A<T>>;
  //  template <class U = int>
  //  using Bar = Foo<U>;
  //
  // In terms of Bar, we want U (which has the default argument) to appear in
  // the synthesized deduction guide, but U would remain undeduced if we deduced
  // A<A<T>> using Foo<U> directly.
  //
  // Instead, we need to canonicalize both against A, i.e. A<A<T>> and A<A<U>>,
  // such that T can be deduced as U.
  // The template arguments of the (trailing) return type of the deduction
  // guide.
  ArrayRef<TemplateArgument> FTemplateArgs =
      getTemplateArgumentsFromDeductionGuideReturnType(
          Context, cast<CXXDeductionGuideDecl>(F->getTemplatedDecl()));
  // Deduce template arguments of the deduction guide f from the RHS of
  // the alias.
  //
  // C++ [over.match.class.deduct]p3: ...For each function or function
  // template f in the guides of the template named by the
  // simple-template-id of the defining-type-id, the template arguments
  // of the return type of f are deduced from the defining-type-id of A
  // according to the process in [temp.deduct.type] with the exception
  // that deduction does not fail if not all template arguments are
  // deduced.
  //
  //
  //  template<typename X, typename Y>
  //  f(X, Y) -> f<Y, X>;
  //
  //  template<typename U>
  //  using alias = f<int, U>;
  //
  // The RHS of alias is f<int, U>, we deduced the template arguments of
  // the return type of the deduction guide from it: Y->int, X->U
  sema::TemplateDeductionInfo TDeduceInfo(Loc);
  // Must initialize n elements, this is required by DeduceTemplateArguments.
  SmallVector<DeducedTemplateArgument> DeduceResults(
      F->getTemplateParameters()->size());

  // FIXME: DeduceTemplateArguments stops immediately at the first
  // non-deducible template argument. However, this doesn't seem to cause
  // issues for practice cases, we probably need to extend it to continue
  // performing deduction for rest of arguments to align with the C++
  // standard.
  SemaRef.DeduceTemplateArguments(F->getTemplateParameters(), FTemplateArgs,
                                  AliasRhsTemplateArgs, TDeduceInfo,
                                  DeduceResults,
                                  /*NumberOfArgumentsMustMatch=*/false);

  SmallVector<TemplateArgument> DeducedArgs;
  SmallVector<unsigned> NonDeducedTemplateParamsInFIndex;
  // !!NOTE: DeduceResults respects the sequence of template parameters of
  // the deduction guide f.
  for (unsigned Index = 0; Index < DeduceResults.size(); ++Index) {
    const auto &D = DeduceResults[Index];
    if (!IsNonDeducedArgument(D))
      DeducedArgs.push_back(D);
    else
      NonDeducedTemplateParamsInFIndex.push_back(Index);
  }
  auto DeducedAliasTemplateParams =
      TemplateParamsReferencedInTemplateArgumentList(
          SemaRef, AliasTemplate->getTemplateParameters(), DeducedArgs);
  // All template arguments null by default.
  SmallVector<TemplateArgument> TemplateArgsForBuildingFPrime(
      F->getTemplateParameters()->size());

  // Create a template parameter list for the synthesized deduction guide f'.
  //
  // C++ [over.match.class.deduct]p3.2:
  //   If f is a function template, f' is a function template whose template
  //   parameter list consists of all the template parameters of A
  //   (including their default template arguments) that appear in the above
  //   deductions or (recursively) in their default template arguments
  SmallVector<NamedDecl *> FPrimeTemplateParams;
  // Store template arguments that refer to the newly-created template
  // parameters, used for building `TemplateArgsForBuildingFPrime`.
  SmallVector<TemplateArgument, 16> TransformedDeducedAliasArgs(
      AliasTemplate->getTemplateParameters()->size());
  // We might be already within a pack expansion, but rewriting template
  // parameters is independent of that. (We may or may not expand new packs
  // when rewriting. So clear the state)
  Sema::ArgPackSubstIndexRAII PackSubstReset(SemaRef, std::nullopt);

  for (unsigned AliasTemplateParamIdx : DeducedAliasTemplateParams) {
    auto *TP =
        AliasTemplate->getTemplateParameters()->getParam(AliasTemplateParamIdx);
    // Rebuild any internal references to earlier parameters and reindex as
    // we go.
    MultiLevelTemplateArgumentList Args;
    Args.setKind(TemplateSubstitutionKind::Rewrite);
    Args.addOuterTemplateArguments(TransformedDeducedAliasArgs);
    NamedDecl *NewParam = transformTemplateParameter(
        SemaRef, AliasTemplate->getDeclContext(), TP, Args,
        /*NewIndex=*/FPrimeTemplateParams.size(), getDepthAndIndex(TP).first);
    FPrimeTemplateParams.push_back(NewParam);

    TemplateArgument NewTemplateArgument =
        Context.getInjectedTemplateArg(NewParam);
    TransformedDeducedAliasArgs[AliasTemplateParamIdx] = NewTemplateArgument;
  }
  unsigned FirstUndeducedParamIdx = FPrimeTemplateParams.size();

  // To form a deduction guide f' from f, we leverage clang's instantiation
  // mechanism, we construct a template argument list where the template
  // arguments refer to the newly-created template parameters of f', and
  // then apply instantiation on this template argument list to instantiate
  // f, this ensures all template parameter occurrences are updated
  // correctly.
  //
  // The template argument list is formed, in order, from
  //   1) For the template parameters of the alias, the corresponding deduced
  //      template arguments
  //   2) For the non-deduced template parameters of f. the
  //      (rebuilt) template arguments corresponding.
  //
  // Note: the non-deduced template arguments of `f` might refer to arguments
  // deduced in 1), as in a type constraint.
  MultiLevelTemplateArgumentList Args;
  Args.setKind(TemplateSubstitutionKind::Rewrite);
  Args.addOuterTemplateArguments(TransformedDeducedAliasArgs);
  for (unsigned Index = 0; Index < DeduceResults.size(); ++Index) {
    const auto &D = DeduceResults[Index];
    auto *TP = F->getTemplateParameters()->getParam(Index);
    if (IsNonDeducedArgument(D)) {
      // 2): Non-deduced template parameters would be substituted later.
      continue;
    }
    TemplateArgumentLoc Input =
        SemaRef.getTrivialTemplateArgumentLoc(D, QualType(), SourceLocation{});
    TemplateArgumentListInfo Output;
    if (SemaRef.SubstTemplateArguments(Input, Args, Output))
      return nullptr;
    assert(TemplateArgsForBuildingFPrime[Index].isNull() &&
           "InstantiatedArgs must be null before setting");
    // CheckTemplateArgument is necessary for NTTP initializations.
    // FIXME: We may want to call CheckTemplateArguments instead, but we cannot
    // match packs as usual, since packs can appear in the middle of the
    // parameter list of a synthesized CTAD guide. See also the FIXME in
    // test/SemaCXX/cxx20-ctad-type-alias.cpp:test25.
    Sema::CheckTemplateArgumentInfo CTAI;
    for (auto TA : Output.arguments())
      if (SemaRef.CheckTemplateArgument(
              TP, TA, F, F->getLocation(), F->getLocation(),
              /*ArgumentPackIndex=*/-1, CTAI,
              Sema::CheckTemplateArgumentKind::CTAK_Specified))
        return nullptr;
    if (Input.getArgument().getKind() == TemplateArgument::Pack) {
      // We will substitute the non-deduced template arguments with these
      // transformed (unpacked at this point) arguments, where that substitution
      // requires a pack for the corresponding parameter packs.
      TemplateArgsForBuildingFPrime[Index] =
          TemplateArgument::CreatePackCopy(Context, CTAI.SugaredConverted);
    } else {
      assert(Output.arguments().size() == 1);
      TemplateArgsForBuildingFPrime[Index] = CTAI.SugaredConverted[0];
    }
  }

  // Case 2)
  //   ...followed by the template parameters of f that were not deduced
  //   (including their default template arguments)
  for (unsigned FTemplateParamIdx : NonDeducedTemplateParamsInFIndex) {
    auto *TP = F->getTemplateParameters()->getParam(FTemplateParamIdx);
    MultiLevelTemplateArgumentList Args;
    Args.setKind(TemplateSubstitutionKind::Rewrite);
    // We take a shortcut here, it is ok to reuse the
    // TemplateArgsForBuildingFPrime.
    Args.addOuterTemplateArguments(TemplateArgsForBuildingFPrime);
    NamedDecl *NewParam = transformTemplateParameter(
        SemaRef, F->getDeclContext(), TP, Args, FPrimeTemplateParams.size(),
        getDepthAndIndex(TP).first);
    FPrimeTemplateParams.push_back(NewParam);

    assert(TemplateArgsForBuildingFPrime[FTemplateParamIdx].isNull() &&
           "The argument must be null before setting");
    TemplateArgsForBuildingFPrime[FTemplateParamIdx] =
        Context.getInjectedTemplateArg(NewParam);
  }

  auto *TemplateArgListForBuildingFPrime =
      TemplateArgumentList::CreateCopy(Context, TemplateArgsForBuildingFPrime);
  // Form the f' by substituting the template arguments into f.
  if (auto *FPrime = SemaRef.InstantiateFunctionDeclaration(
          F, TemplateArgListForBuildingFPrime, AliasTemplate->getLocation(),
          Sema::CodeSynthesisContext::BuildingDeductionGuides)) {
    auto *GG = cast<CXXDeductionGuideDecl>(FPrime);

    TypeSourceInfo *TSI = GG->getTypeSourceInfo();
    QualType ReturnType = FPrime->getReturnType();
    TemplateDecl *DeducedTemplate =
        FromInheritedCtor ? FromInheritedCtor->DerivedClassTemplate
                          : AliasTemplate;
    if (FromInheritedCtor) {
      std::tie(TSI, ReturnType) = buildInheritedConstructorDeductionGuideType(
          SemaRef, *FromInheritedCtor, TSI);
      if (!TSI)
        return nullptr;
    }

    // We omit the deducible constraint for inherited constructor deduction
    // guides because they would take precedence over the derived class' own
    // deduction guides due to [over.match.best.general]p2.5 and
    // [temp.func.order]p6.4. If the alias were not deducible in this case, the
    // deduction guide would already not be deducible due to the partial
    // specialization `CC<>` failing substitution.
    // See https://github.com/cplusplus/CWG/issues/607
    Expr *IsDeducible = nullptr;
    if (!FromInheritedCtor)
      IsDeducible = buildIsDeducibleConstraint(
          SemaRef, AliasTemplate, ReturnType, FPrimeTemplateParams);
    Expr *RequiresClause =
        buildAssociatedConstraints(SemaRef, F, AliasTemplate, DeduceResults,
                                   FirstUndeducedParamIdx, IsDeducible);

    auto *FPrimeTemplateParamList = TemplateParameterList::Create(
        Context, AliasTemplate->getTemplateParameters()->getTemplateLoc(),
        AliasTemplate->getTemplateParameters()->getLAngleLoc(),
        FPrimeTemplateParams,
        AliasTemplate->getTemplateParameters()->getRAngleLoc(),
        /*RequiresClause=*/RequiresClause);
    auto *Result = cast<FunctionTemplateDecl>(buildDeductionGuide(
        SemaRef, DeducedTemplate, FPrimeTemplateParamList,
        GG->getCorrespondingConstructor(), GG->getExplicitSpecifier(), TSI,
        AliasTemplate->getBeginLoc(), AliasTemplate->getLocation(),
        AliasTemplate->getEndLoc(), F->isImplicit()));
    auto *DGuide = cast<CXXDeductionGuideDecl>(Result->getTemplatedDecl());
    DGuide->setDeductionCandidateKind(GG->getDeductionCandidateKind());
    DGuide->setSourceDeductionGuide(
        cast<CXXDeductionGuideDecl>(F->getTemplatedDecl()));
    DGuide->setSourceDeductionGuideKind(
        FromInheritedCtor
            ? CXXDeductionGuideDecl::SourceDeductionGuideKind::
                  InheritedConstructor
            : CXXDeductionGuideDecl::SourceDeductionGuideKind::Alias);
    return Result;
  }
  return nullptr;
}

void DeclareImplicitDeductionGuidesForTypeAlias(
    Sema &SemaRef, TypeAliasTemplateDecl *AliasTemplate, SourceLocation Loc,
    InheritedConstructorDeductionInfo *FromInheritedCtor = nullptr) {
  if (AliasTemplate->isInvalidDecl())
    return;
  TemplateDecl *DeducedTemplate = FromInheritedCtor
                                      ? FromInheritedCtor->DerivedClassTemplate
                                      : AliasTemplate;
  auto &Context = SemaRef.Context;
  auto [Template, AliasRhsTemplateArgs] =
      getRHSTemplateDeclAndArgs(SemaRef, AliasTemplate);
  if (!Template)
    return;
  llvm::DenseSet<const NamedDecl *> SourceDeductionGuides;
  std::optional<InheritedSourceDeductionGuides> InheritedSources;
  if (FromInheritedCtor)
    InheritedSources.emplace(
        SemaRef,
        Context.DeclarationNames.getCXXDeductionGuideName(DeducedTemplate),
        DeducedTemplate->getDeclContext(), AliasTemplate->getLocation());
  else
    SourceDeductionGuides = getSourceDeductionGuides(
        Context.DeclarationNames.getCXXDeductionGuideName(AliasTemplate),
        AliasTemplate->getDeclContext());
  // Whether a guide was already generated from the given guide of the
  // underlying template.
  auto HasGuideFrom = [&](const NamedDecl *D) {
    return InheritedSources ? InheritedSources->contains(D)
                            : SourceDeductionGuides.contains(D);
  };

  DeclarationNameInfo NameInfo(
      Context.DeclarationNames.getCXXDeductionGuideName(Template), Loc);
  LookupResult Guides(SemaRef, NameInfo, clang::Sema::LookupOrdinaryName);
  SemaRef.LookupQualifiedName(Guides, Template->getDeclContext());
  Guides.suppressDiagnostics();

  for (auto *G : Guides) {
    if (auto *DG = dyn_cast<CXXDeductionGuideDecl>(G)) {
      if (HasGuideFrom(DG))
        continue;
      // The deduction guide is a non-template function decl, we just clone it.
      auto *FunctionType =
          SemaRef.Context.getTrivialTypeSourceInfo(DG->getType());
      FunctionProtoTypeLoc FPTL =
          FunctionType->getTypeLoc().castAs<FunctionProtoTypeLoc>();

      // Clone the parameters.
      for (unsigned I = 0, N = DG->getNumParams(); I != N; ++I) {
        const auto *P = DG->getParamDecl(I);
        auto *TSI = SemaRef.Context.getTrivialTypeSourceInfo(P->getType());
        ParmVarDecl *NewParam = ParmVarDecl::Create(
            SemaRef.Context, G->getDeclContext(),
            DG->getParamDecl(I)->getBeginLoc(), P->getLocation(), nullptr,
            TSI->getType(), TSI, SC_None, nullptr);
        NewParam->setScopeInfo(0, I);
        FPTL.setParam(I, NewParam);
      }
      if (FromInheritedCtor) {
        std::tie(FunctionType, std::ignore) =
            buildInheritedConstructorDeductionGuideType(
                SemaRef, *FromInheritedCtor, FunctionType);
        if (!FunctionType)
          continue;
      }
      auto *Transformed = cast<CXXDeductionGuideDecl>(buildDeductionGuide(
          SemaRef, DeducedTemplate, /*TemplateParams=*/nullptr,
          /*Constructor=*/nullptr, DG->getExplicitSpecifier(), FunctionType,
          AliasTemplate->getBeginLoc(), AliasTemplate->getLocation(),
          AliasTemplate->getEndLoc(), DG->isImplicit()));
      Transformed->setSourceDeductionGuide(DG);
      Transformed->setSourceDeductionGuideKind(
          FromInheritedCtor
              ? CXXDeductionGuideDecl::SourceDeductionGuideKind::
                    InheritedConstructor
              : CXXDeductionGuideDecl::SourceDeductionGuideKind::Alias);

      if (FromInheritedCtor) {
        // We omit the deducible constraint for inherited constructor deduction
        // guides; see BuildDeductionGuideForTypeAlias.
        if (const AssociatedConstraint &RC = DG->getTrailingRequiresClause())
          Transformed->setTrailingRequiresClause(RC);
        continue;
      }

      // FIXME: Here the synthesized deduction guide is not a templated
      // function. Per [dcl.decl]p4, the requires-clause shall be present only
      // if the declarator declares a templated function, a bug in standard?
      AssociatedConstraint Constraint(buildIsDeducibleConstraint(
          SemaRef, AliasTemplate, Transformed->getReturnType(), {}));
      if (const AssociatedConstraint &RC = DG->getTrailingRequiresClause()) {
        auto Conjunction = SemaRef.BuildBinOp(
            SemaRef.getCurScope(), SourceLocation{},
            BinaryOperatorKind::BO_LAnd, const_cast<Expr *>(RC.ConstraintExpr),
            const_cast<Expr *>(Constraint.ConstraintExpr));
        if (!Conjunction.isInvalid()) {
          Constraint.ConstraintExpr = Conjunction.getAs<Expr>();
          Constraint.ArgPackSubstIndex = RC.ArgPackSubstIndex;
        }
      }
      Transformed->setTrailingRequiresClause(Constraint);
      continue;
    }
    FunctionTemplateDecl *F = dyn_cast<FunctionTemplateDecl>(G);
    if (!F || HasGuideFrom(F->getTemplatedDecl()))
      continue;
    // The **aggregate** deduction guides are handled in a different code path
    // (DeclareAggregateDeductionGuideFromInitList), which involves the tricky
    // cache.
    if (cast<CXXDeductionGuideDecl>(F->getTemplatedDecl())
            ->getDeductionCandidateKind() == DeductionCandidate::Aggregate)
      continue;

    BuildDeductionGuideForTypeAlias(SemaRef, AliasTemplate, F, Loc,
                                    FromInheritedCtor);
  }
}

// Check if a template is deducible as per [dcl.type.simple]p3
static bool IsDeducibleTemplate(const TemplateDecl *TD) {
  while (TD) {
    // [dcl.type.simple]p3: A deducible template is either a class template ...
    if (isa<ClassTemplateDecl>(TD))
      return true;

    // ... or is an alias template ...
    const auto *Alias = dyn_cast<TypeAliasTemplateDecl>(TD);
    if (!Alias)
      return false;

    QualType AliasType =
        Alias->getTemplatedDecl()->getUnderlyingType().getCanonicalType();

    // ... whose defining-type-id is of the form
    // [typename] [nested-name-specifier] [template] simple-template-id ...
    if (const auto *TST = AliasType->getAs<TemplateSpecializationType>()) {
      // ... and the template-name of the simple-template-id names a deducible
      // template
      TD = TST->getTemplateName().getAsTemplateDecl();
      continue;
    }

    // Handle the case that the RHS of the alias is not dependent
    // e.g. using AliasFoo = Foo<bool>;
    if (const auto *RT = AliasType->getAs<RecordType>())
      return isa<ClassTemplateSpecializationDecl>(RT->getDecl());

    return false;
  }

  return false;
}

// Build the return type `typename CC<R>::type` of the guides generated from
// inherited constructors, where R is CC's only template parameter.
// C++23 [over.match.class.deduct]p1.10:
//   ... the set contains the guides of A with the return type R of each guide
//   replaced with `typename CC<R>::type` ...
TypeSourceInfo *
buildInheritedGuideReturnType(Sema &SemaRef, TemplateDecl *Template,
                              ClassTemplateDecl *CCTemplateDecl) {
  ASTContext &Context = SemaRef.Context;
  auto *TParam = cast<TemplateTypeParmDecl>(
      CCTemplateDecl->getTemplateParameters()->getParam(0));
  TemplateName CCTemplateName =
      Context.getCanonicalTemplateName(TemplateName(CCTemplateDecl));
  TemplateArgument InjectedTParamArg = Context.getInjectedTemplateArg(TParam);
  QualType CCPartialSpecializationType = Context.getTemplateSpecializationType(
      ElaboratedTypeKeyword::None, CCTemplateName, InjectedTParamArg,
      Context.getCanonicalTemplateArgument(InjectedTParamArg));

  NestedNameSpecifier NNS(CCPartialSpecializationType.getTypePtr());
  QualType CCReturnType = Context.getDependentNameType(
      ElaboratedTypeKeyword::Typename, NNS, &Context.Idents.get("type"));

  NestedNameSpecifierLocBuilder NNSLocBuilder;
  NNSLocBuilder.MakeTrivial(Context, NNS, SourceRange(Template->getBeginLoc()));
  NestedNameSpecifierLoc QualifierLoc =
      NNSLocBuilder.getWithLocInContext(Context);

  TypeLocBuilder ReturnTypeTLB;
  DependentNameTypeLoc DepTL =
      ReturnTypeTLB.push<DependentNameTypeLoc>(CCReturnType);
  DepTL.setQualifierLoc(QualifierLoc);
  DepTL.setNameLoc(Template->getBeginLoc());
  DepTL.setElaboratedKeywordLoc(SourceLocation());

  return ReturnTypeTLB.getTypeSourceInfo(Context, CCReturnType);
}

// Declare the deduction guides that \p Template (a class template whose
// definition is \p Pattern) inherits from the base class named by \p BaseTSI
// through a using-declaration that names its constructors.
// C++23 [over.match.class.deduct]p1.10:
//   If C inherits constructors from a direct base class B named in the
//   using-declarator of a using-declaration, let A be an alias template whose
//   template parameter list is that of C and whose defining-type-id is B. Let
//   CC be a class template ... whose primary template is not defined, with a
//   single partial specialization whose template parameter list is that of A
//   and whose template argument list is that of A and which has a member
//   typedef `type` designating a template specialization with the template
//   argument list of A but with C as the template. The set contains the
//   guides of A with the return type R of each guide replaced with
//   `typename CC<R>::type`.
void DeclareImplicitDeductionGuidesFromInheritedConstructors(
    Sema &SemaRef, TemplateDecl *Template, ClassTemplateDecl *Pattern,
    TypeSourceInfo *BaseTSI, unsigned BaseIdx) {
  ASTContext &Context = SemaRef.Context;
  DeclContext *DC = Template->getDeclContext();
  const auto *BaseTST = BaseTSI->getType()->getAs<TemplateSpecializationType>();
  if (!BaseTST)
    return;
  SourceLocation BaseLoc = BaseTSI->getTypeLoc().getBeginLoc();

  TemplateDecl *BaseTD = BaseTST->getTemplateName().getAsTemplateDecl();

  // The alias template `A` that we build out of the base type must be a
  // deducible template. `A` will be of the correct form, so it is deducible iff
  // BaseTD is deducible.
  if (!BaseTD || !IsDeducibleTemplate(BaseTD))
    return;

  IdentifierInfo *AliasIdentifier =
      &Context.Idents.get((Twine("__ctad_A_") + BaseTD->getName() + "_to_" +
                           Template->getName() + "_" + Twine(BaseIdx))
                              .str());
  IdentifierInfo *CCTemplateII =
      &Context.Idents.get((Twine("__ctad_CC_") + BaseTD->getName() + "_to_" +
                           Template->getName() + "_" + Twine(BaseIdx))
                              .str());

  // This is called again whenever the deduction guides of the derived template
  // are looked up, since further deduction guides for the base may have been
  // declared since. In that case, reuse the alias template and CC that were
  // built the first time around.
  {
    TypeAliasTemplateDecl *ExistingATD = nullptr;
    ClassTemplateDecl *ExistingCC = nullptr;
    for (NamedDecl *ND : DC->lookup(DeclarationName(AliasIdentifier)))
      if ((ExistingATD = dyn_cast<TypeAliasTemplateDecl>(ND)))
        break;
    for (NamedDecl *ND : DC->lookup(DeclarationName(CCTemplateII)))
      if ((ExistingCC = dyn_cast<ClassTemplateDecl>(ND)))
        break;
    if (ExistingATD && ExistingCC) {
      InheritedConstructorDeductionInfo Info{
          Template,
          buildInheritedGuideReturnType(SemaRef, Template, ExistingCC)};
      DeclareImplicitDeductionGuidesForTypeAlias(SemaRef, ExistingATD, BaseLoc,
                                                 &Info);
      return;
    }
  }

  // Substitute any parameters with default arguments not present in the base,
  // since partial specializations cannot have default parameters.
  // See https://github.com/cplusplus/CWG/issues/627
  TemplateParameterList *TemplateTPL = Pattern->getTemplateParameters();
  SmallVector<unsigned> BaseDeducedTemplateParamsList =
      TemplateParamsReferencedInTemplateArgumentList(
          SemaRef, TemplateTPL, BaseTST->template_arguments());
  llvm::SmallSet<unsigned, 8> BaseDeducedTemplateParamsSet(
      BaseDeducedTemplateParamsList.begin(),
      BaseDeducedTemplateParamsList.end());
  SmallVector<NamedDecl *, 8> AliasTemplateParams;
  SmallVector<TemplateArgument, 8> SubstArgs;
  AliasTemplateParams.reserve(TemplateTPL->size());
  SubstArgs.reserve(TemplateTPL->size());
  LocalInstantiationScope Scope(SemaRef);
  for (unsigned I = 0, N = TemplateTPL->size(); I < N; ++I) {
    NamedDecl *Param = TemplateTPL->getParam(I);
    if (!BaseDeducedTemplateParamsSet.contains(I)) {
      if (auto *TTP = dyn_cast<TemplateTypeParmDecl>(Param);
          TTP && TTP->hasDefaultArgument()) {
        SubstArgs.push_back(TTP->getDefaultArgument().getArgument());
        continue;
      }

      if (auto *NTTP = dyn_cast<NonTypeTemplateParmDecl>(Param);
          NTTP && NTTP->hasDefaultArgument()) {
        SubstArgs.push_back(NTTP->getDefaultArgument().getArgument());
        continue;
      }

      if (auto *TTP = dyn_cast<TemplateTemplateParmDecl>(Param);
          TTP && TTP->hasDefaultArgument()) {
        SubstArgs.push_back(TTP->getDefaultArgument().getArgument());
        continue;
      }

      // We have a template parameter that is not present in the base and does
      // not have a default argument. We create the deduction guide anyway to
      // display a diagnostic.
    }

    MultiLevelTemplateArgumentList Args;
    Args.setKind(TemplateSubstitutionKind::Rewrite);
    Args.addOuterTemplateArguments(SubstArgs);
    Args.addOuterRetainedLevels(Template->getTemplateDepth());

    NamedDecl *NewParam = transformTemplateParameter(
        SemaRef, DC, Param, Args, AliasTemplateParams.size(),
        Template->getTemplateDepth());
    if (!NewParam)
      return;

    AliasTemplateParams.push_back(NewParam);
    SubstArgs.push_back(Context.getInjectedTemplateArg(NewParam));
  }

  Expr *RequiresClause = nullptr;
  MultiLevelTemplateArgumentList Args;
  Args.setKind(TemplateSubstitutionKind::Rewrite);
  Args.addOuterTemplateArguments(SubstArgs);
  Args.addOuterRetainedLevels(Template->getTemplateDepth());
  if (Expr *TemplateRC = TemplateTPL->getRequiresClause()) {
    ExprResult E = SemaRef.SubstExpr(TemplateRC, Args);
    if (E.isInvalid())
      return;
    RequiresClause = E.getAs<Expr>();
  }
  auto *AliasTPL = TemplateParameterList::Create(
      Context, TemplateTPL->getTemplateLoc(), TemplateTPL->getLAngleLoc(),
      AliasTemplateParams, TemplateTPL->getRAngleLoc(), RequiresClause);

  // Clone AliasTPL into a new parameter list for the partial specialization,
  // but with default arguments removed, using the template instantiator for
  // heavy lifting.
  LocalInstantiationScope CloneScope(SemaRef);
  MultiLevelTemplateArgumentList CloneArgs;
  CloneArgs.setKind(TemplateSubstitutionKind::Rewrite);
  CloneArgs.addOuterRetainedLevels(Template->getTemplateDepth());
  TemplateDeclInstantiator CloneTDI(SemaRef, DC, CloneArgs);
  TemplateParameterList *PartialSpecTPL =
      CloneTDI.SubstTemplateParams(AliasTPL);
  CloneScope.Exit();
  if (!PartialSpecTPL)
    return;
  for (NamedDecl *Param : *PartialSpecTPL) {
    if (auto *TTP = dyn_cast<TemplateTypeParmDecl>(Param))
      TTP->removeDefaultArgument();
    else if (auto *NTTP = dyn_cast<NonTypeTemplateParmDecl>(Param))
      NTTP->removeDefaultArgument();
    else if (auto *TTP = dyn_cast<TemplateTemplateParmDecl>(Param))
      TTP->removeDefaultArgument();
  }

  // C++23 [over.match.class.deduct]p1.10
  // Let A be an alias template whose template parameter list is that of
  // [Template] and whose defining-type-id is [BaseTSI] ...
  TypeSourceInfo *TransformedBase =
      SemaRef.SubstType(BaseTSI, Args, BaseLoc, DeclarationName(), true);
  if (!TransformedBase)
    return;
  TypeAliasDecl *BaseAD = TypeAliasDecl::Create(
      Context, DC, SourceLocation(), BaseLoc, AliasIdentifier, TransformedBase);
  TypeAliasTemplateDecl *BaseATD = TypeAliasTemplateDecl::Create(
      Context, DC, BaseLoc, DeclarationName(AliasIdentifier), AliasTPL, BaseAD);
  BaseAD->setDescribedAliasTemplate(BaseATD);
  BaseAD->setImplicit();
  BaseATD->setImplicit();

  DC->addDecl(BaseATD);

  // ... given a class template `template <typename> class CC;`
  // whose primary template is not defined ...
  TemplateTypeParmDecl *TParam = TemplateTypeParmDecl::Create(
      Context, DC, SourceLocation(), SourceLocation(),
      Template->getTemplateDepth(), 0, nullptr,
      /*Typename=*/true, /*ParameterPack=*/false);
  TParam->setImplicit();
  TemplateParameterList *CCTemplateTPL = TemplateParameterList::Create(
      Context, SourceLocation(), SourceLocation(),
      ArrayRef<NamedDecl *>(TParam), SourceLocation(), nullptr);

  CXXRecordDecl *CCTemplateRD =
      CXXRecordDecl::Create(Context, CXXRecordDecl::TagKind::Struct, DC,
                            SourceLocation(), SourceLocation(), CCTemplateII);
  ClassTemplateDecl *CCTemplateDecl = ClassTemplateDecl::Create(
      Context, DC, SourceLocation(), DeclarationName(CCTemplateII),
      CCTemplateTPL, CCTemplateRD);
  CCTemplateRD->setDescribedClassTemplate(CCTemplateDecl);
  CCTemplateDecl->setImplicit();
  CCTemplateRD->setImplicit();

  DC->addDecl(CCTemplateDecl);

  // ... and with a single partial specialization whose template parameter list
  // is that of A with the template argument list of A ...
  TemplateArgument AliasTA(TransformedBase->getType());
  TemplateArgument CanonAliasTA = Context.getCanonicalTemplateArgument(AliasTA);
  TemplateName CCTemplateName =
      Context.getCanonicalTemplateName(TemplateName(CCTemplateDecl));
  QualType CanonType = Context.getCanonicalTemplateSpecializationType(
      ElaboratedTypeKeyword::None, CCTemplateName, CanonAliasTA);

  ClassTemplatePartialSpecializationDecl *CCPartialSpecialization =
      ClassTemplatePartialSpecializationDecl::Create(
          Context, ClassTemplatePartialSpecializationDecl::TagKind::Struct, DC,
          SourceLocation(), SourceLocation(), PartialSpecTPL, CCTemplateDecl,
          CanonAliasTA, CanQualType::CreateUnsafe(CanonType), nullptr);
  CCPartialSpecialization->setImplicit();
  CCPartialSpecialization->startDefinition();

  TemplateArgumentListInfo TemplateArgs;
  TemplateArgs.addArgument({AliasTA, TransformedBase});
  CCPartialSpecialization->setTemplateArgsAsWritten(TemplateArgs);

  // ... having a member typedef `type` designating a template specialization
  // with the template argument list of A but with [Template] as the template
  TemplateName DerivedTN =
      Context.getCanonicalTemplateName(TemplateName(Template));
  TemplateArgumentListInfo DerivedArgsInfo;
  SmallVector<TemplateArgument, 8> CanonSubstArgs;
  for (unsigned I = 0, C = SubstArgs.size(); I < C; ++I) {
    DerivedArgsInfo.addArgument(SemaRef.getTrivialTemplateArgumentLoc(
        SubstArgs[I], QualType(), TemplateTPL->getParam(I)->getBeginLoc()));
    CanonSubstArgs.push_back(
        Context.getCanonicalTemplateArgument(SubstArgs[I]));
  }
  TypeSourceInfo *CCTypedefTSI = Context.getTemplateSpecializationTypeInfo(
      ElaboratedTypeKeyword::None, SourceLocation(), NestedNameSpecifierLoc(),
      Template->getBeginLoc(), DerivedTN, Template->getLocation(),
      DerivedArgsInfo, CanonSubstArgs);

  const IdentifierInfo &CCTypedefII = Context.Idents.get("type");
  TypedefDecl *DerivedTypedef =
      TypedefDecl::Create(Context, CCPartialSpecialization, BaseLoc, BaseLoc,
                          &CCTypedefII, CCTypedefTSI);

  DerivedTypedef->setImplicit();
  DerivedTypedef->setAccess(AS_public);
  CCPartialSpecialization->addDecl(DerivedTypedef);

  CCPartialSpecialization->completeDefinition();

  CCTemplateDecl->AddPartialSpecialization(CCPartialSpecialization, nullptr);
  DC->addDecl(CCPartialSpecialization);

  InheritedConstructorDeductionInfo Info{
      Template,
      buildInheritedGuideReturnType(SemaRef, Template, CCTemplateDecl)};
  DeclareImplicitDeductionGuidesForTypeAlias(SemaRef, BaseATD, BaseLoc, &Info);
}

// Build an aggregate deduction guide for a type alias template.
FunctionTemplateDecl *DeclareAggregateDeductionGuideForTypeAlias(
    Sema &SemaRef, TypeAliasTemplateDecl *AliasTemplate,
    MutableArrayRef<QualType> ParamTypes, SourceLocation Loc) {
  TemplateDecl *RHSTemplate =
      getRHSTemplateDeclAndArgs(SemaRef, AliasTemplate).first;
  if (!RHSTemplate)
    return nullptr;

  llvm::SmallVector<TypedefNameDecl *> TypedefDecls;
  llvm::SmallVector<QualType> NewParamTypes;
  ExtractTypeForDeductionGuide TypeAliasTransformer(SemaRef, TypedefDecls);
  for (QualType P : ParamTypes) {
    QualType Type = TypeAliasTransformer.TransformType(P);
    if (Type.isNull())
      return nullptr;
    NewParamTypes.push_back(Type);
  }

  auto *RHSDeductionGuide = SemaRef.DeclareAggregateDeductionGuideFromInitList(
      RHSTemplate, NewParamTypes, Loc);
  if (!RHSDeductionGuide)
    return nullptr;

  for (TypedefNameDecl *TD : TypedefDecls)
    TD->setDeclContext(RHSDeductionGuide->getTemplatedDecl());

  return BuildDeductionGuideForTypeAlias(SemaRef, AliasTemplate,
                                         RHSDeductionGuide, Loc);
}

// Find the type of the direct base class from which the using-declaration
// \p UUVD, which names constructors, inherits them.
// Returns null if the base cannot be determined.
TypeSourceInfo *
getInheritedConstructorBaseType(Sema &SemaRef, const CXXRecordDecl *Pattern,
                                UnresolvedUsingValueDecl *UUVD) {
  ASTContext &Context = SemaRef.Context;
  TypeLoc TL = UUVD->getQualifierLoc().getAsTypeLoc();
  if (!TL)
    return nullptr;

  // using Base<...>::Base;
  if (TL.getType()->getAs<TemplateSpecializationType>()) {
    unsigned Size = TL.getFullDataSize();
    TypeSourceInfo *TSI = Context.CreateTypeSourceInfo(TL.getType(), Size);
    TSI->getTypeLoc().initializeFullCopy(TL, Size);
    return TSI;
  }

  // using Derived::Base::Base;
  // Here `Derived::Base` is looked up in the current instantiation, whose base
  // classes are dependent, and so it is only known by name: it designates the
  // injected-class-name of a direct base. Match it by name.
  const auto *DNT = TL.getType()->getAs<DependentNameType>();
  if (!DNT)
    return nullptr;
  const Type *Qualifier = DNT->getQualifier().getAsType();
  if (!Qualifier)
    return nullptr;
  const CXXRecordDecl *QualifierRD = Qualifier->getAsCXXRecordDecl();
  if (!QualifierRD ||
      QualifierRD->getCanonicalDecl() != Pattern->getCanonicalDecl())
    return nullptr;

  TypeSourceInfo *Result = nullptr;
  for (const CXXBaseSpecifier &Base : Pattern->bases()) {
    const auto *BaseTST = Base.getType()->getAs<TemplateSpecializationType>();
    if (!BaseTST)
      continue;
    const TemplateDecl *BaseTD = BaseTST->getTemplateName().getAsTemplateDecl();
    if (!BaseTD || BaseTD->getIdentifier() != DNT->getIdentifier())
      continue;
    // Ambiguous: don't guess.
    if (Result)
      return nullptr;
    Result = Base.getTypeSourceInfo();
  }
  return Result;
}

// C++23 [over.match.class.deduct]p1.10: declare the guides that \p Template
// inherits from its base classes through using-declarations naming their
// constructors. Safe to call repeatedly; only guides not yet inherited from
// are added.
void DeclareImplicitDeductionGuidesFromAllInheritedConstructors(
    Sema &SemaRef, TemplateDecl *Template, ClassTemplateDecl *Pattern) {
  CXXRecordDecl *TemplatedDecl = Pattern->getTemplatedDecl();
  if (!TemplatedDecl->hasDefinition())
    return;
  unsigned BaseIdx = 0;
  for (Decl *D : TemplatedDecl->decls()) {
    auto *UUVD = dyn_cast<UnresolvedUsingValueDecl>(D);
    if (!UUVD || UUVD->getDeclName().getNameKind() !=
                     DeclarationName::CXXConstructorName)
      continue;

    unsigned Idx = BaseIdx++;
    TypeSourceInfo *TSI =
        getInheritedConstructorBaseType(SemaRef, TemplatedDecl, UUVD);
    if (!TSI)
      continue;
    DeclareImplicitDeductionGuidesFromInheritedConstructors(SemaRef, Template,
                                                            Pattern, TSI, Idx);
  }
}

// Whether \p Pattern has any using-declaration that names constructors.
bool hasInheritingConstructorUsing(const ClassTemplateDecl *Pattern) {
  const CXXRecordDecl *RD = Pattern->getTemplatedDecl();
  if (!RD->hasDefinition())
    return false;
  return llvm::any_of(RD->decls(), [](const Decl *D) {
    const auto *UUVD = dyn_cast<UnresolvedUsingValueDecl>(D);
    return UUVD && UUVD->getDeclName().getNameKind() ==
                       DeclarationName::CXXConstructorName;
  });
}

} // namespace

FunctionTemplateDecl *Sema::DeclareAggregateDeductionGuideFromInitList(
    TemplateDecl *Template, MutableArrayRef<QualType> ParamTypes,
    SourceLocation Loc) {
  llvm::FoldingSetNodeID ID;
  ID.AddPointer(Template);
  for (auto &T : ParamTypes)
    T.getCanonicalType().Profile(ID);
  unsigned Hash = ID.ComputeHash();

  auto Found = AggregateDeductionCandidates.find(Hash);
  if (Found != AggregateDeductionCandidates.end()) {
    CXXDeductionGuideDecl *GD = Found->getSecond();
    return GD->getDescribedFunctionTemplate();
  }

  if (auto *AliasTemplate = llvm::dyn_cast<TypeAliasTemplateDecl>(Template)) {
    if (auto *FTD = DeclareAggregateDeductionGuideForTypeAlias(
            *this, AliasTemplate, ParamTypes, Loc)) {
      auto *GD = cast<CXXDeductionGuideDecl>(FTD->getTemplatedDecl());
      GD->setDeductionCandidateKind(DeductionCandidate::Aggregate);
      AggregateDeductionCandidates[Hash] = GD;
      return FTD;
    }
  }

  if (CXXRecordDecl *DefRecord =
          cast<CXXRecordDecl>(Template->getTemplatedDecl())->getDefinition()) {
    if (TemplateDecl *DescribedTemplate =
            DefRecord->getDescribedClassTemplate())
      Template = DescribedTemplate;
  }

  DeclContext *DC = Template->getDeclContext();
  if (DC->isDependentContext())
    return nullptr;

  ConvertConstructorToDeductionGuideTransform Transform(
      *this, cast<ClassTemplateDecl>(Template));
  if (!isCompleteType(Loc, Transform.DeducedType))
    return nullptr;

  // In case we were expanding a pack when we attempted to declare deduction
  // guides, turn off pack expansion for everything we're about to do.
  ArgPackSubstIndexRAII SubstIndex(*this, std::nullopt);
  // Create a template instantiation record to track the "instantiation" of
  // constructors into deduction guides.
  InstantiatingTemplate BuildingDeductionGuides(
      *this, Loc, Template,
      Sema::InstantiatingTemplate::BuildingDeductionGuidesTag{});
  if (BuildingDeductionGuides.isInvalid())
    return nullptr;

  ClassTemplateDecl *Pattern =
      Transform.NestedPattern ? Transform.NestedPattern : Transform.Template;
  ContextRAII SavedContext(*this, Pattern->getTemplatedDecl());

  auto *FTD = cast<FunctionTemplateDecl>(
      Transform.buildSimpleDeductionGuide(ParamTypes));
  SavedContext.pop();
  auto *GD = cast<CXXDeductionGuideDecl>(FTD->getTemplatedDecl());
  GD->setDeductionCandidateKind(DeductionCandidate::Aggregate);
  AggregateDeductionCandidates[Hash] = GD;
  return FTD;
}

void Sema::DeclareImplicitDeductionGuides(TemplateDecl *Template,
                                          SourceLocation Loc) {
  if (auto *AliasTemplate = llvm::dyn_cast<TypeAliasTemplateDecl>(Template)) {
    DeclareImplicitDeductionGuidesForTypeAlias(*this, AliasTemplate, Loc);
    return;
  }
  CXXRecordDecl *DefRecord =
      dyn_cast_or_null<CXXRecordDecl>(Template->getTemplatedDecl());
  if (!DefRecord)
    return;
  if (const CXXRecordDecl *Definition = DefRecord->getDefinition()) {
    if (TemplateDecl *DescribedTemplate =
            Definition->getDescribedClassTemplate())
      Template = DescribedTemplate;
  }

  DeclContext *DC = Template->getDeclContext();
  if (DC->isDependentContext())
    return;

  ConvertConstructorToDeductionGuideTransform Transform(
      *this, cast<ClassTemplateDecl>(Template));
  if (!isCompleteType(Loc, Transform.DeducedType))
    return;

  ClassTemplateDecl *Pattern =
      Transform.NestedPattern ? Transform.NestedPattern : Transform.Template;
  bool AlreadyDeclared =
      hasDeclaredDeductionGuides(Transform.DeductionGuideName, DC);
  // The guides inherited from a base may need to be extended, since deduction
  // guides for the base can be declared after the ones of this template.
  // FIXME: This is only supported for templates that are not members of a class
  // template (or of a specialization of one, which includes the explicit
  // specializations of member templates). Otherwise, the template parameters
  // of the enclosing templates would have to be substituted into the base class
  // and the parameters of this template be shifted to the right depth; without
  // that, the wrong types are deduced. See
  // https://github.com/spwn02/clang-cxx26/issues/122.
  bool DeclareInherited = getLangOpts().CPlusPlus23 &&
                          !Transform.NestedPattern &&
                          Template->getTemplateParameters()->getDepth() == 0 &&
                          !isa<ClassTemplateSpecializationDecl>(DC) &&
                          hasInheritingConstructorUsing(Pattern);
  if (AlreadyDeclared && !DeclareInherited)
    return;

  // In case we were expanding a pack when we attempted to declare deduction
  // guides, turn off pack expansion for everything we're about to do.
  ArgPackSubstIndexRAII SubstIndex(*this, std::nullopt);
  // Create a template instantiation record to track the "instantiation" of
  // constructors into deduction guides.
  InstantiatingTemplate BuildingDeductionGuides(
      *this, Loc, Template,
      Sema::InstantiatingTemplate::BuildingDeductionGuidesTag{});
  if (BuildingDeductionGuides.isInvalid())
    return;

  // Convert declared constructors into deduction guide templates.
  // FIXME: Skip constructors for which deduction must necessarily fail (those
  // for which some class template parameter without a default argument never
  // appears in a deduced context).
  ContextRAII SavedContext(*this, Pattern->getTemplatedDecl());
  if (AlreadyDeclared) {
    DeclareImplicitDeductionGuidesFromAllInheritedConstructors(*this, Template,
                                                               Pattern);
    SavedContext.pop();
    return;
  }
  llvm::SmallPtrSet<NamedDecl *, 8> ProcessedCtors;
  bool AddedAny = false;
  for (NamedDecl *D : LookupConstructors(Pattern->getTemplatedDecl())) {
    D = D->getUnderlyingDecl();
    if (D->isInvalidDecl() || D->isImplicit())
      continue;

    D = cast<NamedDecl>(D->getCanonicalDecl());

    // Within C++20 modules, we may have multiple same constructors in
    // multiple same RecordDecls. And it doesn't make sense to create
    // duplicated deduction guides for the duplicated constructors.
    if (ProcessedCtors.count(D))
      continue;

    auto *FTD = dyn_cast<FunctionTemplateDecl>(D);
    auto *CD =
        dyn_cast_or_null<CXXConstructorDecl>(FTD ? FTD->getTemplatedDecl() : D);
    // Class-scope explicit specializations (MS extension) do not result in
    // deduction guides.
    if (!CD || (!FTD && CD->isFunctionTemplateSpecialization()))
      continue;

    // Cannot make a deduction guide when unparsed arguments are present.
    if (llvm::any_of(CD->parameters(), [](ParmVarDecl *P) {
          return !P || P->hasUnparsedDefaultArg();
        }))
      continue;

    ProcessedCtors.insert(D);
    Transform.transformConstructor(FTD, CD);
    AddedAny = true;
  }

  // C++17 [over.match.class.deduct]
  //    --  If C is not defined or does not declare any constructors, an
  //    additional function template derived as above from a hypothetical
  //    constructor C().
  if (!AddedAny)
    Transform.buildSimpleDeductionGuide({});

  //    -- An additional function template derived as above from a hypothetical
  //    constructor C(C), called the copy deduction candidate.
  cast<CXXDeductionGuideDecl>(
      cast<FunctionTemplateDecl>(
          Transform.buildSimpleDeductionGuide(Transform.DeducedType))
          ->getTemplatedDecl())
      ->setDeductionCandidateKind(DeductionCandidate::Copy);

  // C++23 [over.match.class.deduct]p1.10: guides for inherited constructors.
  if (DeclareInherited)
    DeclareImplicitDeductionGuidesFromAllInheritedConstructors(*this, Template,
                                                               Pattern);

  SavedContext.pop();
}
