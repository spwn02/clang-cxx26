//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23 || c++26

// P4380R1 section 2.3, "Literal testing" (Peter Dimov's example): tokenize() a
// 64 bit value as an unsigned _BitInt literal ("__uwb" is the implementation's
// spelling of the standard "uwb") and as an "ull" literal, inject a test of each
// pair, and compare the two. The literals are numbers of 62, 63 and 64 bits:
// the spelling of each literal token must be NUL-terminated, or the numeric
// literal parser reads on into whatever follows it (#496). The paper's popcount<N>
// deliberately uses the 32-bit __builtin_popcount on 64-bit chunks, to show that the
// generated tests catch it (they print exactly the failures the paper shows: 11 != 25,
// 13 != 31, 13 != 32, 13 != 29); this test uses __builtin_popcountll, so it passes.

// RUN: %{build}
// RUN: %{exec} %t.exe

#include <meta>
#include <print>
#include <ranges>
#include <cstdint>

constexpr std::uint64_t splitmix64( std::uint64_t z )
{
    z ^= z >> 30;
    z *= 0xbf58476d1ce4e5b9ull;
    z ^= z >> 27;
    z *= 0x94d049bb133111ebull;
    z ^= z >> 31;

    return z;
}

static int s_errors;

template<class T1, class T2>
void test( char const* q1, char const* q2, T1 const& t1, T2 const& t2 )
{
    if( !(t1 == t2) )
    {
        std::print( stderr, "Test '{} == {}' failed: '{}' != '{}'\n", q1, q2, t1, t2 );
        ++s_errors;
    }
}

template<int N> int popcount( unsigned _BitInt(N) const& v )
{
    int r = 0;

    for( int i = 0; i < N; i += 64 )
    {
        r += __builtin_popcountll( static_cast<std::uint64_t>( v >> i ) );
    }

    return r;
}

int popcount( std::uint64_t v )
{
    return __builtin_popcountll( v );
}

int main()
{
    constexpr auto fn = ^^{ popcount };

    consteval
    {
        for( auto v: std::views::iota( 0, 5 ) | std::views::transform( splitmix64 ) )
        {
            auto t1 = std::meta::tokenize( v, "__uwb" ); // Standardized as "uwb", but implemented as "__uwb"
            auto q1 = ^^{ \(fn)(\(t1)) };

            auto t2 = std::meta::tokenize( v, "ull" );
            auto q2 = ^^{ \(fn)(\(t2)) };

            queue_injection( ^^{ test( \(stringize(q1)), \(stringize(q2)), \(q1), \(q2) ); } );
        }
    }

    return s_errors;
}
