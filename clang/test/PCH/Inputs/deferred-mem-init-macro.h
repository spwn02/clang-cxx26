#include "token-sequence.h"
__macro init_it(int v) { return ^^{ member(\(v)) }; }
template <class T> struct DeferredCtorInit {
  int member;
  constexpr DeferredCtorInit(int v) : init_it!(v) {}
};
