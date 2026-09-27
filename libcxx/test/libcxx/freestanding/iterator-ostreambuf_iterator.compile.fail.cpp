// -*- C++ -*-
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -ffreestanding

// P1642R11 [iterator.synopsis]: std::ostreambuf_iterator stays hosted-only.
//
// Deliberately references the TYPE only (no object construction, no <sstream>/<ostream>) -- see
// iterator-ostream_iterator.compile.fail.cpp for why: constructing one for real would require
// <sstream>, which pulls this exact type back in through <ostream>'s own unconditional internal
// includes (__ostream/basic_ostream.h, print.h, put_character_sequence.h all include
// <__iterator/ostreambuf_iterator.h> directly), an unrelated transitive path that would make the
// probe meaningless regardless of <iterator>'s own gating.

#include <iterator>

using __probe = std::ostreambuf_iterator<char>;
