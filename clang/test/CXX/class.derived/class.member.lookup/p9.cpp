// RUN: %clang_cc1 -fsyntax-only -verify %s
// expected-no-diagnostics

namespace rdar8436162 {
  class ClsA {
  public:
    static void f();
    void g();
  };

  class ClsB : virtual private ClsA {
  public:
    using ClsA::f;
    using ClsA::g;
  };

  class ClsF : virtual private ClsA {
  public:
    using ClsA::f;
    using ClsA::g;
  };

  class ClsE : public ClsB, public ClsF {
    void test() { 
      f();
      g(); // OK: both lookups find the declaration of ClsA::g ([class.member.lookup])
    }
  };
}
