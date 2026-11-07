//===-- SwapBinaryOperandsTests.cpp -----------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TweakTesting.h"
#include "gmock/gmock-matchers.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace clang {
namespace clangd {
namespace {

TWEAK_TEST(SwapBinaryOperands);

TEST_F(SwapBinaryOperandsTest, Test) {
  Context = Function;
  EXPECT_EQ(apply("int *p = nullptr; bool c = ^p == nullptr;"),
            "int *p = nullptr; bool c = nullptr == p;");
  EXPECT_EQ(apply("int *p = nullptr; bool c = p ^== nullptr;"),
            "int *p = nullptr; bool c = nullptr == p;");
  EXPECT_EQ(apply("int x = 3; bool c = ^x >= 5;"),
            "int x = 3; bool c = 5 <= x;");
  EXPECT_EQ(apply("int x = 3; bool c = x >^= 5;"),
            "int x = 3; bool c = 5 <= x;");
  EXPECT_EQ(apply("int x = 3; bool c = x >=^ 5;"),
            "int x = 3; bool c = 5 <= x;");
  EXPECT_EQ(apply("int x = 3; bool c = x >=^ 5;"),
            "int x = 3; bool c = 5 <= x;");
  EXPECT_EQ(apply("int f(); int x = 3; bool c = x >=^ f();"),
            "int f(); int x = 3; bool c = f() <= x;");
  EXPECT_EQ(apply(R"cpp(
            int f();
            #define F f
            int x = 3; bool c = x >=^ F();
            )cpp"),
            R"cpp(
            int f();
            #define F f
            int x = 3; bool c = F() <= x;
            )cpp");
  EXPECT_EQ(apply(R"cpp(
            int f();
            #define F f()
            int x = 3; bool c = x >=^ F;
            )cpp"),
            R"cpp(
            int f();
            #define F f()
            int x = 3; bool c = F <= x;
            )cpp");
  EXPECT_EQ(apply(R"cpp(
            int f(bool);
            #define F(v) f(v)
            int x = 0;
            bool c = F(x^ < 5);
            )cpp"),
            R"cpp(
            int f(bool);
            #define F(v) f(v)
            int x = 0;
            bool c = F(5 > x);
            )cpp");
  ExtraArgs = {"-std=c++20"};
  Context = CodeContext::File;
  EXPECT_UNAVAILABLE(R"cpp(
            namespace std {
                struct strong_ordering {
                    int val; 
                    static const strong_ordering less;
                    static const strong_ordering equivalent;
                    static const strong_ordering equal;
                    static const strong_ordering greater;
                }; 
                    inline constexpr strong_ordering strong_ordering::less {-1};
                    inline constexpr strong_ordering strong_ordering::equivalent {0};
                    inline constexpr strong_ordering strong_ordering::equal {0};
                    inline constexpr strong_ordering strong_ordering::greater {1};
            };
            #define F(v) v
            int x = 0;
            auto c = F(5^ <=> x);
            )cpp");
}

// The expansion of an expression macro is made of tokens that all have the
// location of the invocation: its operands and operator are not three pieces of
// the file. The operator of an expansion is not offered; one written in an
// argument of the invocation, or next to it, is.
TEST_F(SwapBinaryOperandsTest, ExpressionMacroExpansion) {
  ExtraArgs = {"-std=c++2d"};
  Context = CodeContext::File;
  // (In the header: the carets of a token sequence literal are not markers.)
  Header = R"cpp(
namespace std::meta {
template <class... Ts> consteval auto id(Ts const &...) -> int;
using token = decltype(id("x"));
class token_sequence {
public:
  constexpr token_sequence() noexcept = default;
};
} // namespace std::meta
__macro both(bool a, bool b) { return ^^{ \(a) or \(b) }; }
)cpp";
  const char *Pre = "bool f(bool x, bool y) {\n";
  // The operator of the expansion is not offered...
  EXPECT_UNAVAILABLE(std::string(Pre) + "  return both!(x, y)^;\n}\n");
  EXPECT_UNAVAILABLE(std::string(Pre) + "  return bo^th!(x, y);\n}\n");
  // ... but a binary operator written in an argument of the invocation is.
  EXPECT_AVAILABLE(std::string(Pre) + "  return both!(x ^== y, y);\n}\n");
  // ... and so is one next to the invocation.
  EXPECT_AVAILABLE(std::string(Pre) + "  return both!(x, y) ^== x;\n}\n");
}

// A macro that stands for an overloaded operator: the operator written by the
// user is replaced by the expansion, whose operands and operator would be three
// overlapping edits (found with 'clangd --check' on the P4380R1 tuple example,
// #498; this test fails without the check in prepare).
TEST_F(SwapBinaryOperandsTest, OperatorMacro) {
  ExtraArgs = {"-std=c++2d"};
  Context = CodeContext::File;
  Header = R"cpp(
namespace std::meta {
template <class... Ts> consteval auto id(Ts const &...) -> int;
using token = decltype(id("x"));
class token_sequence {
public:
  constexpr token_sequence() noexcept = default;
};
} // namespace std::meta
struct Bool {
  bool b;
  __macro operator or(this Bool self, bool rhs) {
    return ^^{ \(self).b or \(rhs) };
  }
};
bool call();
)cpp";
  EXPECT_UNAVAILABLE("bool f() { return Bool{true} o^r call(); }");
  EXPECT_UNAVAILABLE("bool f() { return ^Bool{true} or call(); }");
}

} // namespace
} // namespace clangd
} // namespace clang
