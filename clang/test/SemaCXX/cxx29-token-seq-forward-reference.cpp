// RUN: %clang_cc1 -std=c++2d -verify %s -I %S/../Reflection/Inputs

#include "token-sequence.h"

namespace std::meta {
using info = decltype(^^::);
consteval auto queue_injection(info target_ns, token_sequence) -> void;
} // namespace std::meta

// P4380 leaves open how code may refer to a declaration that a LATER
// injection introduces (the paper's vtable_for example: a variable template
// versus a forward declaration). This port gives injected declarations no
// special standing: an injected declaration is found where the injection has
// happened, exactly as if it had been written at that point. These tests pin
// that behavior so a change in it is deliberate.
namespace N1 {
consteval void inject() { std::meta::queue_injection(^^N1, ^^{ constexpr int later = 42; }); }

int before() { return later; } // expected-error {{use of undeclared identifier 'later'}}

consteval { inject(); }

static_assert(later == 42);
int after() { return later; }
} // namespace N1

// Forward-declaring an entity by hand does not make the injected definition
// complete it: the injection declares a second, distinct entity (the
// redeclaration is not matched against the earlier one), so naming it is
// ambiguous. Whether it should merge is part of the paper's open question.
namespace N2 {
extern const int pending; // expected-note {{candidate found by name lookup is 'N2::pending'}}
consteval void inject() { std::meta::queue_injection(^^N2, ^^{ constexpr int pending = 7; }); } // expected-note {{candidate found by name lookup is 'N2::pending'}}
consteval { inject(); }
static_assert(pending == 7); // expected-error {{reference to 'pending' is ambiguous}}
} // namespace N2
