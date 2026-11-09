// RUN: %clang_cc1 -std=c++23 -fsyntax-only -verify=cxx23 %s
// RUN: %clang_cc1 -std=c++2c -fsyntax-only -verify=cxx23 %s
// RUN: %clang_cc1 -std=c++2d -fsyntax-only -verify=cxx29 %s

// P2953R5: restrictions on explicitly defaulted assignment operators (C++29). Before, such declarations were well-formed and
// defined as deleted when defaulted on their first declaration.

struct RvalueRefQualified {
  RvalueRefQualified& operator=(const RvalueRefQualified&) && = default;
  // cxx29-error@-1 {{an explicitly-defaulted copy assignment operator may not have the '&&' ref-qualifier}}
};
struct MoveRvalueRefQualified {
  MoveRvalueRefQualified& operator=(MoveRvalueRefQualified&&) && = default;
  // cxx29-error@-1 {{an explicitly-defaulted move assignment operator may not have the '&&' ref-qualifier}}
};
struct ConstQualified {
  ConstQualified& operator=(const ConstQualified&) const = default;
  // cxx23-warning@-1 {{explicitly defaulted copy assignment operator is implicitly deleted}}
  // cxx23-note@-2 {{function is implicitly deleted because its declared type does not match the type of an implicit copy assignment operator}}
  // cxx29-error@-3 {{an explicitly-defaulted copy assignment operator may not have 'const' or 'volatile' qualifiers}}
};
struct ConstMoveParameter {
  ConstMoveParameter& operator=(const ConstMoveParameter&&) = default;
  // cxx23-warning@-1 {{explicitly defaulted move assignment operator is implicitly deleted}}
  // cxx23-note@-2 {{function is implicitly deleted because its declared type does not match the type of an implicit move assignment operator}}
  // cxx29-error@-3 {{the parameter for an explicitly-defaulted move assignment operator may not be const}}
};
struct VolatileParameter {
  VolatileParameter& operator=(volatile VolatileParameter&) = default;
  // cxx23-warning@-1 {{explicitly defaulted copy assignment operator is implicitly deleted}}
  // cxx23-note@-2 {{function is implicitly deleted because its declared type does not match the type of an implicit copy assignment operator}}
  // cxx29-error@-3 {{the parameter for an explicitly-defaulted copy assignment operator may not be volatile}}
};

// Still allowed in C++26: the '&' ref-qualifier, an explicit object parameter of type "lvalue reference to C", a non-const
// reference parameter instead of const, and exception specifications that differ.
struct Allowed {
  Allowed& operator=(const Allowed&) & = default;
};
struct AllowedExplicitObject {
  AllowedExplicitObject& operator=(this AllowedExplicitObject&, const AllowedExplicitObject&) = default;
};
struct AllowedNonConst {
  AllowedNonConst& operator=(AllowedNonConst&) = default;
  AllowedNonConst(const AllowedNonConst&) = default;
};
struct AllowedExceptionSpec {
  AllowedExceptionSpec& operator=(const AllowedExceptionSpec&) noexcept(false) = default;
};

// A member of type "reference to C" (not const) makes the implicit copy assignment take C&; a const C& parameter is then
// still deleted when defaulted on the first declaration, and ill-formed afterwards.
struct HasNonConstCopy {
  HasNonConstCopy(HasNonConstCopy&);
  HasNonConstCopy& operator=(HasNonConstCopy&);
};
struct DeletedConstParameter {
  HasNonConstCopy member;
  DeletedConstParameter& operator=(const DeletedConstParameter&) = default;
  // cxx23-warning@-1 {{explicitly defaulted copy assignment operator is implicitly deleted}}
  // cxx23-note@-2 {{function is implicitly deleted because its declared type does not match the type of an implicit copy assignment operator}}
  // cxx29-warning@-3 {{explicitly defaulted copy assignment operator is implicitly deleted}}
  // cxx29-note@-4 {{function is implicitly deleted because its declared type does not match the type of an implicit copy assignment operator}}
};

// Not C++26 specific: other special members keep their existing behavior.
struct Others {
  Others(const Others&) = default;
  Others(Others&&) = default;
  ~Others() = default;
};

// [dcl.fct.def.default]/2 (N5054) covers every special member function, not only assignment operators: a defaulted
// constructor whose type differs in a way the paragraph does not list is ill-formed, where it was defined as deleted.
struct ConstMoveCtor {
  ConstMoveCtor(const ConstMoveCtor&&) = default;
  // cxx23-warning@-1 {{explicitly defaulted move constructor is implicitly deleted}}
  // cxx23-note@-2 {{function is implicitly deleted because its declared type does not match the type of an implicit move constructor}}
  // cxx29-error@-3 {{the parameter for an explicitly-defaulted move constructor may not be const}}
};
struct VolatileCopyCtor {
  VolatileCopyCtor(volatile VolatileCopyCtor&) = default;
  // cxx23-warning@-1 {{explicitly defaulted copy constructor is implicitly deleted}}
  // cxx23-note@-2 {{function is implicitly deleted because its declared type does not match the type of an implicit copy constructor}}
  // cxx29-error@-3 {{the parameter for an explicitly-defaulted copy constructor may not be volatile}}
};
// The two listed differences stay: 'C&' for 'const C&', and 'const C&' for 'C&' (deleted on the first declaration).
struct NonConstCopyCtor { NonConstCopyCtor(NonConstCopyCtor&) = default; };
struct ConstParamForNonConst {
  int& r;
  ConstParamForNonConst(const ConstParamForNonConst&) = default;
};
