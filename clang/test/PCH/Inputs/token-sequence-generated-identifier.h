#include "token-sequence.h"

struct Injector {
  static __macro inject() {
    return ^^{ int generated_identifier_with_length = 1; };
  }
};
template <class T> struct Container {
  T::inject!();
};
