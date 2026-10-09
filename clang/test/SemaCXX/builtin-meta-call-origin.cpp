// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s
namespace std {
struct source_location {
  struct __impl {
    const char *_M_file_name;
    const char *_M_function_name;
    unsigned _M_line;
    unsigned _M_column;
  };
};
}
const std::source_location::__impl *p = __builtin_meta_call_origin();
consteval void test() {
  __builtin_meta_call_origin(1); // expected-error {{too many arguments to function call, expected 0, have 1}}
}

static_assert(__is_same(decltype(__builtin_meta_call_origin()),
                        const std::source_location::__impl *));
namespace std::meta {
inline namespace __reflection_v2 {
consteval const source_location::__impl *inner() {
  return __builtin_meta_call_origin();
}
consteval const source_location::__impl *outer() { return inner(); }
}
}
constexpr unsigned direct_line = __LINE__; constexpr auto direct = std::meta::outer();
static_assert(direct->_M_line == direct_line);
consteval bool wrapper() {
  unsigned line = __LINE__; auto loc = std::meta::outer();
  return loc->_M_line == line;
}
static_assert(wrapper());
constexpr unsigned fallback_line = __LINE__; constexpr auto fallback = __builtin_meta_call_origin();
static_assert(fallback->_M_line == fallback_line);
