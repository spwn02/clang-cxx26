#include "token-sequence.h"

struct Host {
  static __macro access() { return ^^{ public }; }
};

template <class T> struct S {
  T::access!();
  int value;
};
