// RUN: %clang_cc1 -std=c++2d -verify %s

// [expr.call]: a destructor's precondition assertions are evaluated before
// its body, its postcondition assertions after the destruction has completed,
// also in constant evaluation (they were skipped before).

struct Pre {
  bool ok;
  constexpr Pre(bool b) : ok(b) {}
  constexpr ~Pre() pre(ok) {} // expected-error {{contract failed during execution of constexpr function}}
};
constexpr bool prePass() { Pre p(true); return true; }
static_assert(prePass());
constexpr bool preFail() { Pre p(false); return true; } // expected-note {{in call to 'p.~Pre()'}}
static_assert(preFail()); // expected-error {{static assertion expression is not an integral constant expression}} \
                          // expected-note {{in call to 'preFail()'}}

// A postcondition cannot name members of the destroyed object.
struct Post {
  constexpr ~Post() post(true) {}
};
constexpr bool postPass() { Post p; return true; }
static_assert(postPass());

struct PostFail {
  constexpr ~PostFail() post(false) {} // expected-error {{contract failed during execution of constexpr function}}
};
constexpr bool postFail() { PostFail p; return true; } // expected-note {{in call to 'p.~PostFail()'}}
static_assert(postFail()); // expected-error {{static assertion expression is not an integral constant expression}} \
                           // expected-note {{in call to 'postFail()'}}

// The precondition is evaluated before the body, which can still change the
// object: the failing precondition is reported even though the body would
// have fixed it.
struct Order {
  bool ok = false;
  constexpr ~Order() pre(ok) { ok = true; } // expected-error {{contract failed during execution of constexpr function}}
};
constexpr bool order() { Order o; return true; } // expected-note {{in call to 'o.~Order()'}}
static_assert(order()); // expected-error {{static assertion expression is not an integral constant expression}} \
                        // expected-note {{in call to 'order()'}}

// Members and bases are destroyed before the postcondition: a member
// destructor's failing postcondition is reached from the enclosing object.
struct Member { constexpr ~Member() post(false) {} }; // expected-error {{contract failed during execution of constexpr function}}
struct Holder { Member m; };
constexpr bool holder() { Holder h; return true; } // expected-note {{in call to 'h.m.~Member()'}} expected-note {{in call to 'h.~Holder()'}}
static_assert(holder()); // expected-error {{static assertion expression is not an integral constant expression}} \
                         // expected-note {{in call to 'holder()'}}
