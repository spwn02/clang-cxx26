// RUN: %clang_cc1 %s -std=c++26 -freflection -fcxx-exceptions -fexceptions -Wall -Wextra -Werror -verify
// expected-no-diagnostics

using info = decltype(^^int);
namespace std::meta {
struct exception { info from; const char* what; };
constexpr info __make_exception(info from, const char* what) {
  throw exception{from, what};
}
}

consteval info object(info r) {
  return __metafunction(21, r, ^^object);
}
consteval info substitution(info templ, const info* args, unsigned count,
                           bool diagnose) {
  return __metafunction(24, templ, static_cast<const info*>(args), count, diagnose, ^^substitution);
}
template<class T> consteval info constant(T value) {
  return __metafunction(95, ^^T, value, ^^constant<T>);
}

template<class> struct TT {};
namespace NS {}
int global;
thread_local int tls;

consteval bool thread_local_object() {
  try { (void)object(^^tls); }
  catch (std::meta::exception& e) { return e.from == ^^object; }
  return false;
}
static_assert(thread_local_object());
static_assert(object(^^global) != ^^global);

consteval bool invalid_argument(info arg, bool diagnose) {
  info arguments[] = {arg};
  try { (void)substitution(^^TT, arguments, 1, diagnose); }
  catch (std::meta::exception& e) { return e.from == ^^substitution; }
  return false;
}
static_assert(invalid_argument(^^NS, false));
static_assert(invalid_argument(^^::, false));
static_assert(invalid_argument(info{}, false));
static_assert(invalid_argument(^^NS, true));
static_assert(invalid_argument(info{}, true));
consteval bool valid_arguments() {
  info types[] = {^^int};
  info values[] = {constant(1)};
  return substitution(^^TT, types, 1, false) == (^^TT<int>) &&
         substitution(^^TT, values, 1, false) == info{};
}
static_assert(valid_arguments());

consteval bool local_pointer() {
  int local = 1;
  try { (void)constant(&local); }
  catch (std::meta::exception& e) { return e.from == ^^constant<int*>; }
  return false;
}
static_assert(local_pointer());
static_assert(constant(&global) != info{});
static_assert(constant(42) != info{});
