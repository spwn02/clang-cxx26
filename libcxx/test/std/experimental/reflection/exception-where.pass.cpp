//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// [meta.syn]: E.where() represents from where the call to F originated.
#include <meta>
#include <initializer_list>
#include <iterator>
#include <vector>
#include <string_view>

using namespace std::meta;
struct Incomplete;
using infos = std::initializer_list<info>;

template <class Probe>
consteval bool check_where(Probe probe, unsigned line) {
  try {
    probe();
  } catch (const exception& e) {
    return e.where().line() == line &&
           std::string_view(e.where().file_name()) == __FILE__;
  }
  return false;
}

#define CHECK(...) static_assert(check_where([] consteval { (void)(__VA_ARGS__); }, __LINE__))
CHECK(size_of(^^void));
CHECK(size_of(^^Incomplete)); // Compiler Diagnoser path.
CHECK(members_of(^^int, access_context::unchecked()));
CHECK(substitute(info{}, infos{^^int}));
CHECK(extract<int>(info{}));
CHECK(identifier_of(info{}));
CHECK(bases_of(^^int, access_context::unchecked()));
CHECK(constant_of(info{}));
CHECK(offset_of(^^int));
CHECK(nonstatic_data_members_of(^^int, access_context::unchecked()));
CHECK(remove_cvref(^^::)); // Nested library-to-library calls.
#undef CHECK

// The origin is inside a user wrapper, rather than its caller.
// Keep each __LINE__ assignment on the same source line as its call.
consteval bool wrapper() {
  unsigned line = 0;
  try {
    line = __LINE__; (void)size_of(^^Incomplete);
  } catch (const exception& e) {
    return e.where().line() == line &&
           std::string_view(e.where().file_name()) == __FILE__;
  }
  return false;
}
static_assert(wrapper());

#define USER_WRAPPER(NAME, ...) \
  constexpr unsigned NAME##_line = __LINE__; \
  consteval void NAME() { (void)(__VA_ARGS__); }
USER_WRAPPER(throwing_wrapper, size_of(^^Incomplete))
#undef USER_WRAPPER
static_assert(check_where(throwing_wrapper, throwing_wrapper_line));
consteval void wrapper_chain() { throwing_wrapper(); }
static_assert(check_where(wrapper_chain, throwing_wrapper_line));

consteval bool constructed() {
  unsigned line = __LINE__; exception e("user", ^^int);
  return e.where().line() == line &&
         std::string_view(e.where().file_name()) == __FILE__;
}
static_assert(constructed());

consteval bool constructed_u8() {
  unsigned line = __LINE__; exception e(u8"user", ^^int);
  return e.where().line() == line &&
         std::string_view(e.where().file_name()) == __FILE__;
}
static_assert(constructed_u8());

// User code running inside a metafunction: F is the function that threw, so
// where() is the call to F inside the user code, and an exception the user
// constructs is never rewritten by the metafunction it propagates through.
template <class Source>
struct UserRange {
  struct It {
    using value_type = info;
    using difference_type = std::ptrdiff_t;
    consteval info operator*() const { return Source::get(); }
    consteval It& operator++() { return *this; }
    consteval void operator++(int) {}
    consteval bool operator==(std::default_sentinel_t) const { return false; }
  };
  consteval It begin() const { return {}; }
  consteval std::default_sentinel_t end() const { return {}; }
};
constexpr unsigned own_line = __LINE__ + 2;
struct OwnException {
  static consteval info get() { throw exception(u8"mine", ^^int); }
};
constexpr unsigned lib_line = __LINE__ + 2;
struct LibraryException {
  static consteval info get() { (void)size_of(^^Incomplete); return ^^int; }
};

consteval bool user_exception_through_substitute() {
  try {
    (void)substitute(^^std::vector, UserRange<OwnException>{});
  } catch (const exception& e) {
    return e.from() == (^^int) && e.where().line() == own_line;
  }
  return false;
}
static_assert(user_exception_through_substitute());

consteval bool user_exception_through_define_static_array() {
  try {
    (void)std::define_static_array(UserRange<OwnException>{});
  } catch (const exception& e) {
    return e.from() == (^^int) && e.where().line() == own_line;
  }
  return false;
}
static_assert(user_exception_through_define_static_array());

consteval bool library_exception_in_user_code() {
  try {
    (void)substitute(^^std::vector, UserRange<LibraryException>{});
  } catch (const exception& e) {
    return identifier_of(e.from()) == "size_of" && e.where().line() == lib_line;
  }
  return false;
}
static_assert(library_exception_in_user_code());

int main(int, char**) {}
