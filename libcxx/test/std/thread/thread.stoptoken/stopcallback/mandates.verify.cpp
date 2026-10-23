// REQUIRES: std-at-least-c++20

// <stop_token>
// Mandates that CallbackFn satisfies invocable and destructible.

#include <stop_token>

// expected-note@*:* 0+ {{in instantiation of}}
// expected-note@*:* 0+ {{because}}

struct NotInvocable {
  ~NotInvocable() = default;
};

struct NotDestructible {
  void operator()() const;
  ~NotDestructible() = delete;
};

// expected-error@*:* {{Mandates: stop_callback is instantiated with an argument for the template parameter Callback that satisfies invocable.}}
static_assert(sizeof(std::stop_callback<NotInvocable>) > 0);
// expected-error@*:* {{Mandates: stop_callback is instantiated with an argument for the template parameter Callback that satisfies destructible.}}
static_assert(sizeof(std::stop_callback<NotDestructible>) > 0);
