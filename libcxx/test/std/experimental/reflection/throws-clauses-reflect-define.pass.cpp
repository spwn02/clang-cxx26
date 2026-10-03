//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// Regression for #185: exceptions must be catchable during constant evaluation.
#include <meta>
#include <array>
#include <vector>
#include <tuple>
#include <variant>
using namespace std::meta;
template<class Probe> consteval bool throws_exception(Probe probe) {
  try { probe(); } catch (const exception&) { return true; }
  return false;
}
template<class Probe> consteval bool returns_normally(Probe probe) {
  try { probe(); } catch (const exception&) { return false; }
  return true;
}
#define THROWS(...) static_assert(throws_exception([] consteval { (void)(__VA_ARGS__); }))
#define NORMAL(...) static_assert(returns_normally([] consteval { (void)(__VA_ARGS__); }))
namespace NS {}
struct Incomplete;
struct C { int member; unsigned bits : 3; static int sm; void mf(); };
struct Other { int member; };
struct Op { bool operator==(const Op&) const; };
struct Abstract { virtual void f() = 0; };
struct VirtualAbstract : virtual Abstract {};
enum class E { a };
enum class Opaque : int;
int global;
int& ref = global;
constexpr int value = 42;
constexpr int array[] = {1, 2};
void fn([[maybe_unused]] int parameter) {}
void noargs() {}
template<class> struct Box {};
template<class T> requires (sizeof(T) == 1) struct Small {};
template<class T> auto undeduced();
[[maybe_unused]] constexpr auto unchecked = access_context::unchecked();
[[maybe_unused]] constexpr auto closure = [] {};
struct Ptr { const char* p; };

struct ThrowBegin {
  using value_type = int;
  constexpr int* begin() { if consteval { throw exception("range", ^^int); } else { return nullptr; } }
  constexpr int* end() { return nullptr; }
};
struct ThrowCopy {
  bool fail;
  constexpr ThrowCopy(bool b) : fail(b) {}
  constexpr ThrowCopy(const ThrowCopy& other) : fail(other.fail) {
    if consteval { if (fail) throw exception("copy", ^^int); }
  }
};

extern int& unknown_reference;
thread_local int thread_object;
auto pending();
[[maybe_unused]] constexpr const int* const_pointer = &value;
struct ConstMember { const int member; };
void noexcept_fn() noexcept {}

// Clause 1:
// \throws
// Any of
// \begin{itemize}
// \item
// an exception thrown by any operation
// on \tcode{r} or
// on iterators and sentinels referring to \tcode{r},
// \item
// an exception thrown
// by the evaluation of any argument of \tcode{reflect_constant} or
// by any evaluation of \tcode{reflect_constant_array}, or
// \item
// \tcode{meta::exception}
// if any invocation of \tcode{reflect_constant}
// would exit via an exception.
// \end{itemize}
static_assert(throws_exception([] consteval { int local = 0; (void)reflect_constant_array(std::array<const int*, 1>{&local}); }));
NORMAL(reflect_constant_array(std::array{1, 2}));
THROWS(reflect_constant_array(ThrowBegin{}));
THROWS(reflect_constant_array(std::array<ThrowCopy, 1>{ThrowCopy{true}}));
static_assert(throws_exception([] consteval { int local = 0; const int* a[1][1] = {{&local}}; (void)reflect_constant_array(a); }));

// Clause 37:
// \throws
// \tcode{meta::exception} unless
// the \grammarterm{template-id} \tcode{TCls<$V$>} would be valid.
THROWS(reflect_constant(Ptr{"literal"}));
THROWS(reflect_constant("literal"));
static_assert(throws_exception([] consteval { int local = 0; (void)reflect_constant(&local); }));
NORMAL(reflect_constant(42));
NORMAL(reflect_constant(Ptr{nullptr}));

// Clause 38:
// \throws
// \tcode{meta::exception} if
// \tcode{E} is not suitable for use as a constant template argument
// for a constant template parameter of type \tcode{T\&}\iref{temp.arg.nontype},
// where \tcode{E} is an lvalue constant expression that
// computes the object that \tcode{expr} refers to.
static_assert(throws_exception([] consteval { int local = 0; (void)reflect_object(local); }));
THROWS(reflect_object("literal"));
NORMAL(reflect_object(global));

// Clause 39:
// \throws
// \tcode{meta::exception} if
// \tcode{F} is not suitable for use as a constant template argument
// for a constant template parameter of type \tcode{T\&}\iref{temp.arg.nontype},
// where \tcode{F} is an lvalue constant expression that
// computes the function that \tcode{fn} refers to.
NORMAL(reflect_function(noargs));

// Clause 40:
// \throws
// \tcode{meta::exception} unless the following conditions are met:
// \begin{itemize}
// \item
//   \tcode{dealias(type)} represents either an object type or a reference type;
// \item
//   if \tcode{options.name} contains a value, then:
//   \begin{itemize}
//   \item
//     \tcode{holds_alternative<u8string>(options.name->\exposid{contents})} is \tcode{true}
//     and \tcode{get<u8string>(\brk{}options.name->\exposid{contents})}
//     contains the spelling of a valid \grammarterm{token}
//     that is an \grammarterm{identifier}\iref{lex.name}
//     when interpreted with UTF-8, or
//   \item
//     \tcode{holds_alternative<string>(options.name->\exposid{contents})} is \tcode{true}
//     and \tcode{get<string>(opt\-ions.name->\exposid{contents})}
//     contains the spelling of a valid \grammarterm{token}
//     that is an \grammarterm{identifier}\iref{lex.name}
//     when interpreted with the ordinary literal encoding;
//   \end{itemize}
//   \begin{note}
//   Lexical constructs like
//   \grammarterm{universal-character-name}s\iref{lex.universal.char} are not processed.
//   For example, \tcode{R"(\textbackslash u03B1)"} is an invalid identifier
//   and is not interpreted as \tcode{"$\alpha$"}.
//   \end{note}
// \item
//   if \tcode{options.name} does not contain a value,
//   then \tcode{options.bit_width} contains a value and
//   \tcode{options.annotations} is empty;
// \item
//   if \tcode{options.bit_width} contains a value $V$, then
//   \begin{itemize}
//   \item
//     \tcode{is_integral_type(type) || is_enum_type(type)} is \tcode{true},
//   \item
//     \tcode{options.alignment} does not contain a value,
//   \item
//     \tcode{options.no_unique_address} is \tcode{false},
//   \item
//     $V$ is not negative, and
//   \item
//     if $V$ equals \tcode{0},
//     then \tcode{options.name} does not contain a value; and
//   \item
//     if \tcode{options.name} does not contain a value, then
//     \tcode{is_const(type) || is_volatile(type)} is \tcode{false}; and
//   \end{itemize}
//   \item
//     if \tcode{options.alignment} contains a value,
//     it is an alignment value\iref{basic.align}
//     not less than \tcode{alignment_of(type)}; and
//   \item
//     for every reflection \tcode{r} in \tcode{options.annotations},
//     \tcode{\exposid{has-type}(r)} is \tcode{true},
//     \tcode{type_of(r)} represents a non-array object type, and
//     evaluation of \tcode{constant_of(r)} does not exit via an exception.
// \end{itemize}
THROWS(data_member_spec(^^void, {.name="m"}));
THROWS(data_member_spec(^^void(), {.name="m"}));
THROWS(data_member_spec(^^int, {.name="1bad"}));
THROWS(data_member_spec(^^int, {.name=u8"1bad"}));
THROWS(data_member_spec(^^int, {}));
THROWS(data_member_spec(^^int, {.bit_width=1, .annotations={reflect_constant(1)}}));
THROWS(data_member_spec(^^float, {.name="m", .bit_width=1}));
THROWS(data_member_spec(^^int, {.name="m", .alignment=8, .bit_width=1}));
THROWS(data_member_spec(^^int, {.name="m", .bit_width=1, .no_unique_address=true}));
THROWS(data_member_spec(^^int, {.name="m", .bit_width=-1}));
THROWS(data_member_spec(^^int, {.name="m", .bit_width=0}));
THROWS(data_member_spec(^^const int, {.bit_width=1}));
THROWS(data_member_spec(^^volatile int, {.bit_width=1}));
THROWS(data_member_spec(^^int, {.name="m", .alignment=3}));
THROWS(data_member_spec(^^int, {.name="m", .alignment=1}));
THROWS(data_member_spec(^^int, {.name="m", .annotations={^^int}}));
THROWS(data_member_spec(^^int, {.name="m", .annotations={^^array}}));
THROWS(data_member_spec(^^int, {.name="m", .annotations={^^global}}));
NORMAL(data_member_spec(^^int, {.name="m"}));
NORMAL(data_member_spec(^^int, {.name=u8"m"}));
NORMAL(data_member_spec(^^int, {.bit_width=0}));
NORMAL(data_member_spec(^^int, {.name="m", .annotations={reflect_constant(1)}}));
THROWS(data_member_spec(^^int, {.name="m", .annotations={^^noargs}}));
THROWS(data_member_spec(^^int, {.name="m", .annotations={^^C::member}}));
THROWS(data_member_spec(^^int, {.name="class"}));
THROWS(data_member_spec(^^int, {.name=u8"class"}));
THROWS(data_member_spec(^^int, {.name=R"(\u03B1)"}));
THROWS(data_member_spec(^^int, {.name=u8R"(\u03B1)"}));
NORMAL(data_member_spec(^^int&, {.name="m"}));
NORMAL(data_member_spec(^^int, {.name="m", .alignment=8}));
NORMAL(data_member_spec(^^int, {.name="m", .bit_width=1}));
NORMAL(data_member_spec(^^int, {.name="m", .no_unique_address=true}));

#undef THROWS
#undef NORMAL
int main(int, char**) {}
