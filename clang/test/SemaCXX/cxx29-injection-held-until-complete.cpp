// RUN: %clang_cc1 -std=c++2d -fsyntax-only -verify %s

// expected-no-diagnostics

// Injections queued by an inject_members callback drain after the annotated
// class is complete. Anything instantiated while the class is still being
// completed (the callbacks of later annotations, the implicit special members
// of the class) used to drain the whole queue on the way out, so the injected
// declarations were parsed while the class was still incomplete.

#include "../Reflection/Inputs/token-sequence.h"

namespace std::meta {
using info = decltype(^^::);
consteval auto queue_injection(token_sequence) -> void;
} // namespace std::meta

using std::meta::token_sequence;

// Queues an injection that needs the complete class.
struct requires_complete_class {
  consteval auto inject_members(std::meta::info r) const -> token_sequence {
    std::meta::queue_injection(^^{
      static_assert(sizeof(\(r)) == sizeof(int));
      constexpr int injected_after_completion = 1;
    });
    return ^^{ };
  }
};

// A callback that is itself a template is instantiated when the compiler
// calls it, in the middle of the completion of the annotated class, and
// instantiates a class template in turn. The end of every class template
// instantiation drains the queue of pending injections.
template <class T> struct trait { static constexpr int value = 0; };

struct instantiates_a_class_template {
  template <class T = char>
  consteval auto inject_members(std::meta::info) const -> token_sequence {
    static_assert(trait<T>::value == 0);
    return ^^{ };
  }
};

struct [[= requires_complete_class{}]] [[= instantiates_a_class_template{}]] annotated {
  int member;
};

static_assert(injected_after_completion == 1);
