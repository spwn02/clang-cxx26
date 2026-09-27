//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <stacktrace>

// string to_string(const stacktrace_entry&);
// template<class Allocator> string to_string(const basic_stacktrace<Allocator>&);
// ostream& operator<<(ostream&, const stacktrace_entry&);
// template<class Allocator> ostream& operator<<(ostream&, const basic_stacktrace<Allocator>&);

#include <cassert>
#include <sstream>
#include <stacktrace>
#include <string>

// LWG3515: a bare, non-template requires-expression does not get SFINAE treatment (an
// ill-formed simple-requirement there is a hard error); wrap it in a concept so substitution
// actually applies.
template <class T>
concept can_stream_to_wide = requires(std::wostream& os, const T& t) { os << t; };

int main(int, char**) {
  std::stacktrace_entry empty_entry;
  std::string empty_str = std::to_string(empty_entry);
  assert(!empty_str.empty()); // spec: "0x0" for an empty entry.

  std::stacktrace st = std::stacktrace::current();
  assert(!st.empty());

  // [stacktrace.entry.observers]: "the streamed result is the same as the
  // result of ... std::to_string(f)".
  {
    std::string s     = std::to_string(st[0]);
    std::ostringstream oss;
    oss << st[0];
    assert(oss.str() == s);
    assert(!s.empty());
  }

  // Same relationship for the whole basic_stacktrace.
  {
    std::string s     = std::to_string(st);
    std::ostringstream oss;
    oss << st;
    assert(oss.str() == s);
    assert(!s.empty());
  }

  // An empty basic_stacktrace still produces a (possibly empty) string, not
  // UB or a crash.
  std::stacktrace empty_trace;
  std::string empty_trace_str = std::to_string(empty_trace);
  (void)empty_trace_str;

  // LWG3515: operator<< is declared taking `ostream&` (resp. templated only on Allocator),
  // not templated on charT/traits -- otherwise it would be well-formed (but not usable) for
  // any character type, since the effects always stream a std::string. A concept, wrapping the
  // requires-expression so template substitution actually applies (a bare, non-template
  // requires-expression does not get SFINAE treatment: an ill-formed simple-requirement there is
  // a hard error, not `false`), must see the non-template overload as simply not viable for a
  // wide stream.
  {
    static_assert(!can_stream_to_wide<std::stacktrace_entry>,
                  "operator<<(ostream&, const stacktrace_entry&) must not be a charT/traits template");
    static_assert(!can_stream_to_wide<std::stacktrace>,
                  "operator<<(ostream&, const basic_stacktrace<Allocator>&) must not be a charT/traits template");
  }

  return 0;
}
