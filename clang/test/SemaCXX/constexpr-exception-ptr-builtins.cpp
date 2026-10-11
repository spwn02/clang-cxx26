// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -fcxx-exceptions %s

// Internal evaluator operations used by libc++'s constexpr exception_ptr.
struct Tracked {
  int *Destructions;
  constexpr ~Tracked() { ++*Destructions; }
};

consteval bool capture_retain_rethrow_and_release() {
  int Destructions = 0;
  void *Handle = nullptr;
  try {
    try {
      throw Tracked{&Destructions};
    } catch (const Tracked &Caught) {
      Handle = __builtin_constexpr_exception_capture();
      if (!Handle || static_cast<const Tracked *>(Handle) != &Caught)
        return false;
      __builtin_constexpr_exception_retain(Handle);
      throw;
    }
  } catch (const Tracked &Caught) {
    if (static_cast<const Tracked *>(Handle) != &Caught || Destructions != 0)
      return false;
    __builtin_constexpr_exception_release(Handle);
    if (Destructions != 0)
      return false;
    __builtin_constexpr_exception_release(Handle);
  }
  return Destructions == 1;
}
static_assert(capture_retain_rethrow_and_release());

consteval bool null_capture_outside_handler() {
  return __builtin_constexpr_exception_capture() == nullptr;
}
static_assert(null_capture_outside_handler());

consteval void *escape_exception_handle() {
  try {
    throw 17; // expected-note 2{{heap allocation performed here}}
  } catch (int) {
    return __builtin_constexpr_exception_capture();
  }
}
constexpr void *EscapedHandle = escape_exception_handle();
// expected-error@-1 {{constexpr variable 'EscapedHandle' must be initialized by a constant expression}}
// expected-error@-2 {{call to consteval function 'escape_exception_handle' is not a constant expression}}
// expected-note@-3 2{{pointer to heap-allocated object is not a constant expression}}

consteval const int *escape_exception_reference() {
  try {
    throw 23;
  } catch (const int &Caught) {
    return &Caught;
  }
}
constexpr const int *EscapedReference = escape_exception_reference();
// expected-error@-1 {{constexpr variable 'EscapedReference' must be initialized by a constant expression}}
// expected-error@-2 {{call to consteval function 'escape_exception_reference' is not a constant expression}}
// expected-note@-3 2{{pointer to heap-allocated object is not a constant expression}}

consteval bool invalid_rethrow() {
  __builtin_constexpr_exception_rethrow(nullptr); // expected-note {{subexpression not valid in a constant expression}}
  return true;
}
static_assert(invalid_rethrow()); // expected-error {{not an integral constant expression}} expected-note {{in call to 'invalid_rethrow()'}}
