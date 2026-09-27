// -*- C++ -*-
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -ffreestanding

// P1642R11 [iterator.synopsis]: std::ostream_iterator stays hosted-only.
//
// Deliberately references the TYPE only (no object construction, no <sstream>/<ostream>):
// ostream_iterator has no default constructor, so a construction-based probe fails hosted too
// (for the wrong reason), and pulling in a real stream header to construct one would leak this
// same type back in through <ostream>'s own unconditional internal includes -- an unrelated
// transitive-include path, not a statement about <iterator>'s own gating.

#include <iterator>

using __probe = std::ostream_iterator<char>;
