# P3074R7 — trivial unions (C++26)

Status: implemented in C++26 mode (`__cpp_trivial_union == 202502L`); earlier modes are unchanged.

## Rules implemented

* **Default constructor** ([class.default.ctor]): a union's defaulted default constructor is no longer deleted or
  non-trivial because of its variant members (only a default member initializer makes it non-trivial). Variant members
  without a default member initializer are no longer checked for a usable/trivial default constructor, nor for a usable
  destructor (2.6). Non-variant subobjects, `const` unions and reference members behave as before. If the constructor is
  trivial, it begins the lifetime of the first variant member (of the union and of each anonymous union member) when that
  has implicit-lifetime type.
* **Destructor** ([class.dtor]): a union's defaulted destructor is trivial, except that it stays deleted when a variant
  member has a non-trivial (or deleted/inaccessible) destructor **and** either the union has no trivial defaulted default
  constructor (user-provided, deleted, or none declared) or that variant member has a default member initializer. This is
  the "constructor and destructor must match" principle from Core: `U u;` is either ill-formed or fine.
* Copy/move constructors and assignments are unchanged (still deleted for non-trivial variant members).
* Anonymous unions of a class are treated the same way with the enclosing class as `X`.

## Implementation map

* `DeclCXX.cpp` `addedMember`: for C++26 unions the non-trivial variant-member destructor/default-constructor no longer clear the
  trivial/irrelevant bits; the destructor is flagged `NeedOverloadResolutionForDestructor` so Sema decides deletion once all
  constructors are known. `DeclCXX.h` `setImplicitDestructorIsDeleted` also clears `HasIrrelevantDestructor`, otherwise uses
  of a deleted union destructor would never be diagnosed.
* `SemaDeclCXX.cpp`: `shouldDeleteForClassSubobject` / `shouldDeleteForSubobjectCall` (rules above, helper
  `defaultInitIsNotTriviallyDefaulted`), `checkTrivialClassMembers` (explicitly defaulted `U() = default;` / `~U() = default;`).
* `ExprConstant.cpp` `handleDefaultInitValue`: trivial default-initialization of a union activates the first variant member
  if implicit-lifetime (`isImplicitLifetimeType`, mirrors `__builtin_is_implicit_lifetime`). Not done for members of a class
  whose own (user-provided) constructor leaves an anonymous union alone ([class.base.init]p9), nor for members created
  implicitly inside an activated member.
* `InitPreprocessor.cpp`: `__cpp_trivial_union`.

## Deliberately not done / follow-ups

* libc++ `<inplace_vector>` constexpr for non-trivial `T` and `__cpp_lib_constexpr_inplace_vector` (P3074R7 4.3): library item.
* libc++ `optional`/`variant` storage could drop their hand-written union destructors; not attempted.
* CodeGen needs nothing: trivial constructor/destructor emit no code (`CodeGenCXX/cxx26-trivial-union.cpp`).

## Tests

`SemaCXX/cxx26-trivial-union.cpp`, `SemaCXX/cxx26-trivial-union-constexpr.cpp`, `CodeGenCXX/cxx26-trivial-union.cpp`,
`PCH/cxx26-trivial-union.cpp`, `Lexer/cxx-features.cpp`; updated `CXX/drs/cwg6xx.cpp` (DR 667) and
`SemaCXX/builtin-is-within-lifetime.cpp` (first member is within its lifetime after default initialization).
