//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17
// REQUIRES: localization

#include <istream>

template <class Stream>
concept CanSelectCharIgnore = requires(Stream& stream) {
  stream.template ignore<char>(1, typename Stream::char_type{});
};

static_assert(CanSelectCharIgnore<std::istream>);
static_assert(!CanSelectCharIgnore<std::wistream>);

int main(int, char**) { return 0; }
