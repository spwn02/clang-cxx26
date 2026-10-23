//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// The rvalue stream operators require public, unambiguous derivation from ios_base.

#include <istream>
#include <ostream>

struct PrivateInput : private std::istream {
  using std::istream::istream;
  PrivateInput& operator>>(int&);
};

struct PublicInput : std::istream {
  using std::istream::istream;
  PublicInput& operator>>(int&);
};

struct PrivateOutput : private std::ostream {
  using std::ostream::ostream;
  PrivateOutput& operator<<(const int&);
};

struct PublicOutput : std::ostream {
  using std::ostream::ostream;
  PublicOutput& operator<<(const int&);
};

struct FirstIosBase : std::ios_base {};
struct SecondIosBase : std::ios_base {};

struct AmbiguousStream : FirstIosBase, SecondIosBase {
  AmbiguousStream& operator>>(int&);
  AmbiguousStream& operator<<(const int&);
};

template <class Stream>
concept HasRvalueExtract =
    requires(Stream&& stream, int& value) { std::operator>>(static_cast<Stream&&>(stream), value); };

template <class Stream>
concept HasRvalueInsert =
    requires(Stream&& stream, const int& value) { std::operator<<(static_cast<Stream&&>(stream), value); };

static_assert(!HasRvalueExtract<PrivateInput>);
static_assert(HasRvalueExtract<PublicInput>);
static_assert(!HasRvalueExtract<AmbiguousStream>);
static_assert(!HasRvalueInsert<PrivateOutput>);
static_assert(HasRvalueInsert<PublicOutput>);
static_assert(!HasRvalueInsert<AmbiguousStream>);

int main(int, char**) { return 0; }
