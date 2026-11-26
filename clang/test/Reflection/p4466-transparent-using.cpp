// RUN: %clang -std=c++26 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/libcxx/test-suite-install/include/c++/v1 -fsyntax-only -Xclang -verify %s

#include <meta>
using info = decltype(^^int);

namespace meta = std::meta;

namespace NS {
  enum class Enum1 { E1 };
  using Enum1::E1;
  enum Enum2 { E2 };
  using Enum2::E2;

  inline namespace Inline {
    void f() {}
  }
  inline namespace Sibling {
    using Inline::f;
  }
  using Sibling::f;

  void g() {}
  using NS::g;
  inline namespace Inline {
    using NS::g;
  }
}

namespace Types {
  inline namespace Inline {
    struct type {};
    using alias = int;
    inline constexpr int variable = 1;
    enum class E { value };
  }
  using Inline::type;
  using Inline::alias;
  using Inline::variable;
  using Inline::E;
}

static_assert(^^NS::f == ^^NS::Inline::f);
static_assert(^^NS::g == ^^NS::g);
static_assert(^^NS::Enum1::E1 == ^^NS::Enum1::E1);
static_assert(^^NS::Enum2::E2 == ^^NS::Enum2::E2);
static_assert(^^Types::type == ^^Types::Inline::type);
static_assert(^^Types::variable == ^^Types::Inline::variable);
static_assert(meta::is_function(^^NS::f));
static_assert(meta::identifier_of(^^Types::type) == "type");
static_assert(meta::members_of(^^Types::type, meta::access_context::unchecked()) ==
              meta::members_of(^^Types::Inline::type,
                               meta::access_context::unchecked()));

namespace InlineExport {
  inline namespace Inline {
    void f() {}
  }
  using Inline::f;
}
static_assert(^^InlineExport::f == ^^InlineExport::Inline::f);

namespace UnnamedExport {
  namespace {
    void f() {}
  }
}
static_assert(meta::is_function(^^UnnamedExport::f));

// Lookup through an intervening unnamed namespace is transparent.
namespace Anonymous {
  inline namespace Source {
    namespace {
    struct type {};
    }
  }
  using Source::type;
}
static_assert(^^Anonymous::type == ^^Anonymous::Source::type);

// A transparent using-declarator naming a type behaves the same in a type
// operand and a reflection-name operand. The explicit typename workaround
// remains valid as well.
static_assert(^^typename Types::type == ^^Types::Inline::type);
static_assert(^^typename Types::alias == ^^Types::Inline::alias);

namespace Outer {
  inline namespace V1 {
    struct chained {};
  }
  using V1::chained;
  using Outer::chained;
}
static_assert(^^Outer::chained == ^^Outer::V1::chained);

namespace Templates {
  inline namespace V1 {
    template <class T> struct Box {};
    template <class T> void function(T) {}
  }
  using V1::Box;
  using V1::function;
}
static_assert(^^Templates::Box == ^^Templates::V1::Box);
static_assert(^^Templates::function == ^^Templates::V1::function);

// [expr.reflect]/5.1 keeps non-transparent cases ill-formed.
constexpr info r1 = ^^NS::E1;
// expected-error@-1 {{cannot take the reflection of a using-declarator}}
constexpr info r2 = ^^NS::E2;
// expected-error@-1 {{cannot take the reflection of a using-declarator}}
constexpr info r3 = ^^NS::Sibling::f;
// expected-error@-1 {{cannot take the reflection of a using-declarator}}
constexpr info r6 = ^^NS::Inline::g;
// expected-error@-1 {{cannot take the reflection of a using-declarator}}

namespace Overloads {
  inline namespace Inline {
    void f(int) {}
    void f(double) {}
  }
  using Inline::f;
}
constexpr info r_overload = ^^Overloads::f;
// expected-error@-1 {{cannot take the reflection of an overload set}}

// A lookup set is ill-formed when any contributing using-declarator fails the
// transparency conditions, even when another declaration is transparent.
namespace Mixed {
  inline namespace Inline {
    void f(int) {}
  }
  namespace Nested {
    void f(double) {}
  }
  using Inline::f;
  using Nested::f;
}
constexpr info r_mixed = ^^Mixed::f;
// expected-error@-1 {{cannot take the reflection of a using-declarator}}

namespace EnumUsing {
  enum Enum2 { E2 };
  using enum Enum2;
}
constexpr info r8 = ^^EnumUsing::E2;
// expected-error@-1 {{cannot take the reflection of a using-declarator}}

namespace Outside {
  using NS::g;
}
constexpr info r4 = ^^Outside::g;
// expected-error@-1 {{cannot take the reflection of a using-declarator}}

namespace GlobalLookup {
  void function() {}
}
using GlobalLookup::function;
constexpr info r_global = ^^function;
// expected-error@-1 {{cannot take the reflection of a using-declarator}}

struct Base { void member() {} };
struct Derived : Base { using Base::member; };
constexpr info r5 = ^^Derived::member;
// expected-error@-1 {{cannot take the reflection of a using-declarator}}

struct EnumBase { enum E { value }; };
struct EnumDerived : EnumBase { using enum EnumBase::E; };
constexpr info r_enum_using_class = ^^EnumDerived::value;
// expected-error@-1 {{cannot take the reflection of a using-declarator}}

// Reflection-name lookup in a dependent class context still diagnoses the
// non-transparent class-scope using-declarator at instantiation.
template <class T>
struct Dependent : T {
  using T::member;
  static constexpr info reflected = ^^Dependent::member;
};
constexpr info r7 = Dependent<Base>::reflected;
// expected-note@-1 {{in instantiation of static data member}}
// expected-error@-4 {{cannot take the reflection of a using-declarator}}
