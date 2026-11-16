// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26

#include <__meta/token_locations.h>
#include <concepts>
#include <meta>
#include <string_view>
#include <type_traits>

using std::meta::token;
using std::meta::__x::__diagnostic_fixit;

static_assert(requires(token __t, std::string_view __message,
                       __diagnostic_fixit __fix) {
  { std::meta::__x::__error(__t, __message, __fix) } -> std::same_as<bool>;
  { std::meta::__x::__error(__t, __t, __message, __fix) } -> std::same_as<bool>;
  { std::meta::__x::__warning(__t, __message, __fix) } -> std::same_as<bool>;
  { std::meta::__x::__warning(__t, __t, __message, __fix) } -> std::same_as<bool>;
  { std::meta::__x::__note(__t, __message, __fix) } -> std::same_as<bool>;
  { std::meta::__x::__note(__t, __t, __message, __fix) } -> std::same_as<bool>;
});

constexpr auto __replace =
    std::meta::__x::__replace(token{}, token{}, "replacement");
constexpr auto __insert = std::meta::__x::__insert_after(token{}, "text");
constexpr auto __remove = std::meta::__x::__remove(token{}, token{});

static_assert(__replace.__operation == __diagnostic_fixit::__kind::replace);
static_assert(__replace.__replacement == "replacement");
static_assert(__insert.__operation == __diagnostic_fixit::__kind::insert_after);
static_assert(__insert.__replacement == "text");
static_assert(__remove.__operation == __diagnostic_fixit::__kind::remove);
static_assert(__remove.__replacement.empty());

int main(int, char**) { return 0; }
