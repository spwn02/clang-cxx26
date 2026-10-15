// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// [dcl.contract.res]: a result binding denotes the object or reference returned and has the return type, so for a
// function returning a reference it denotes the referenced object (#314).
constexpr int qq = 4;
constexpr const int &same(const int &v) post(r: &r == &v) { return v; }
static_assert(&same(qq) == &qq);

constexpr const int &value(const int &v) post(r: r == 4) { return v; }
static_assert(value(qq) == 4);

constexpr int other = 5;
constexpr const int &wrong(const int &v) post(r: &r == &other) { return v; } // expected-error {{contract failed during execution of constexpr function}}
constexpr const int &w = wrong(qq); // expected-error {{must be initialized by a constant expression}} expected-note {{in call to 'wrong(qq)'}}

constexpr const int &wrongValue(const int &v) post(r: r == 5) { return v; } // expected-error {{contract failed during execution of constexpr function}}
constexpr const int &w2 = wrongValue(qq); // expected-error {{must be initialized by a constant expression}} expected-note {{in call to 'wrongValue(qq)'}}
