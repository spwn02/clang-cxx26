#ifndef MS_MANGLE_REFLECTION_H
#define MS_MANGLE_REFLECTION_H

template <auto R> struct holder {};

template <class T> struct base {};
struct first : base<int> {};
struct second : base<int> {};
using info = decltype(^^int);
struct sentinel {};
struct parameter {};
struct annotation {};
consteval info reflected_base(info r) {
  return __metafunction(2, r, ^^sentinel, 0, ^^base);
}
template <auto R> int code() { return 0; }

consteval info reflected_parameter(info r, unsigned i = 0) {
  return __metafunction(98, r, ^^sentinel, i, ^^parameter);
}
consteval info reflected_annotation(info r, unsigned i = 0) {
  return __metafunction(105, r, ^^sentinel, i, ^^annotation);
}

#endif
