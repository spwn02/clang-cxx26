#ifndef MS_MANGLE_REFLECTION_UNNAMED_H
#define MS_MANGLE_REFLECTION_UNNAMED_H
struct Anon {
  union { int a; float b; };
  struct { int c; int d; };
  union { int e; };
};
template <class> struct Box {};
#endif
