// RUN: %clang_cc1 -std=c++26 -freflection -fattribute-reflection -verify %s
// P3385R8 [expr.reflect]: Computing the reflection of assume is ill-formed.
constexpr auto a = ^^[[assume(true)]]; // expected-error {{reflecting over the unsupported 'assume' attribute is ill-formed}}
constexpr auto b = ^^[[nodiscard]];
constexpr auto c = ^^[[clang::always_inline]];
constexpr auto d = ^^[[clang::assume(true)]];
