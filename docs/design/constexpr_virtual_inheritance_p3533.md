# P3533R2 constexpr virtual inheritance — design note (#45)

Status: **plan only**, nothing implemented. Probed 2026-09-25 on HEAD `b1811f61aca4` (assertions build).
Primary source: https://wg21.link/P3533R2 (adopted for C++26).

## What the paper changes

Deletes the [dcl.constexpr] rule that a constexpr constructor/destructor may not belong to a class with virtual
bases, removes the term *constexpr-suitable* everywhere, and therefore drops the literal-type restriction on
virtual bases. Adds the feature-test macro `__cpp_constexpr_virtual_inheritance` (value assigned at adoption; the paper text shows a placeholder).

## Current behaviour (HEAD, `-std=c++26`)

The earlier triage note in #45 ("the synthesized derived constructor isn't constexpr") understated it: every layer
rejects the feature.

```cpp
struct A { int a = 1; constexpr virtual ~A() = default; };
struct B : virtual A {};
struct C : B {};
constexpr int g() { C c; return c.a; }
static_assert(g() == 1);
// note: cannot construct object of type 'C' with virtual base class in a constant expression

struct A2 { constexpr virtual int f() const { return 1; } };
// error: constexpr member function not allowed in struct with virtual base class   (SemaDeclCXX.cpp:1931)
// error: constexpr constructor not allowed in struct with virtual base class

struct A3 { int a; }; struct B3 : virtual A3 {};
constexpr B3 x = B3{};   // error: struct with virtual base class is not a literal type
```

## Touch points

Sema
- `SemaDeclCXX.cpp` (`CheckConstexprFunctionDefinition`, ~1931-1970): drop the `getNumVBases()` rejection (diags
  `err_constexpr_virtual_base`, `note_constexpr_virtual_base_here`, `note_non_literal_virtual_base` in
  `DiagnosticSemaKinds.td` ~3039-3045) and gate on `LangOpts.CPlusPlus26` so earlier modes keep the old diagnostics.
- `DeclCXX.cpp` `CXXRecordDecl::isLiteral()` / `hasNonLiteralTypeFieldsOrBases`: virtual bases no longer make a class
  non-literal in C++26.
- Line 2489 of `SemaDeclCXX.cpp` asserts `getNumVBases() == 0` in the constexpr-constructor path: must go.
- `InitPreprocessor.cpp`: define `__cpp_constexpr_virtual_inheritance` for C++26.

Constant evaluator (`ExprConstant.cpp`), the real work
- Object representation: `APValue` structs store non-virtual bases only; virtual base subobjects need a slot
  (or an indirect lookup through the most-derived object) so that `HandleLValueBase` (~3508) can address them, and
  `HandleConstructorCall`/`HandleClassZeroInitialization` must build them.
- Constructor evaluation: an Itanium-style *complete-object* constructor initialises virtual bases first, exactly
  once, in the most-derived class; base-object constructors skip them. The evaluator currently evaluates the
  constructor body directly (`~7747: assert(!BaseIt->isVirtual() && "virtual base for literal type")`), so it must
  distinguish most-derived vs base construction and use the mem-initializer of the *most derived* class for a virtual base.
- Dynamic type / virtual calls: `ExprConstant.cpp` ~7010 refuses to compute a dynamic type with virtual bases and
  ~7068 assumes the final overrider lies on the path from the dynamic to the static type; both comments say they
  "will need modifications if this restriction is relaxed". Needs cross-path final-overrider search, `dynamic_cast`
  to/from virtual bases, and `typeid`.
- Destruction order, `std::construct_at`/`destroy_at` lifetime tracking, member-pointer casts through virtual bases
  (asserts at ~11797, ~11814, ~11973 are deliberate guards, keep them).
- Diagnostics: existing note "cannot construct object of type 'C' with virtual base class in a constant expression"
  stays for pre-C++26 modes.
- Reflection interplay: `members_of`/`bases_of` and `is_virtual` already model virtual bases; splicing a virtual base subobject is
  diagnosed (`DiagnosticSemaKinds.td` ~3271). Check that the new constexpr path doesn't bypass that diagnostic.

libc++: no library dependency; libc++ classes with virtual bases (`basic_ios`) could later become constexpr, out of scope.

## Staged milestones

1. Sema: relax the rules for C++26 + the macro; tests that now compile but still fail *evaluation* with the existing note (clean intermediate state).
2. Evaluator: virtual-base slots in `APValue` (and serialization: `PropertiesBase.td` struct case, PCH round trip), complete-object
   construction/destruction; non-virtual-call cases only.
3. Virtual dispatch, `dynamic_cast`, `typeid`, final-overrider search across virtual bases.
4. Diamond/adjustment cases, `constexpr` destructors, conversion of pointers to virtual bases, `-fexperimental-new-constant-interpreter`
   should keep rejecting (or be tested to reject) rather than crash.

## Test plan

Paper examples plus: diamond with a shared virtual base initialised once; virtual base initialised by the
most-derived mem-initializer (side-effect counters); virtual function called through a virtual-base reference; constexpr
virtual destructor; PCH/module round trip of a constexpr variable of such a class (the serialization traps recorded in
the memory notes about APValue apply); `-std=c++23` still rejects with today's diagnostics. Full Sema/CodeGenCXX/PCH sweep.

## Risks and unknowns

- `APValue` layout change ripples into serialization, `ExprConstantMeta.cpp` (reflection reads bases) and the
  ARM/Itanium ABI layout code (`getVBaseClassOffset` is a *runtime-layout* concept, not an evaluator one).
- Evaluator must behave identically to the codegen'd constructor for observable side effects (init order).
- Upstream Clang has no implementation; expect to be the reference.

Effort: milestone 1 is small (an afternoon); the evaluator (2-3) is a multi-session compiler change with the highest risk of subtle bugs.
