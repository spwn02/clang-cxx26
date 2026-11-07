//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23 || c++26

// P4380R1 section 3.4, the second abbreviated lambda (block 046): fun!(p => p.x)
// and fun!((l, r) => l.y < r.y), with the $ interpolator. A macro that throws
// ("failed to parse fun") is a compile-time error; the parse itself is checked
// by the programs below.

// The paper's macro declares a local `args` while the structured binding `args`
// is being initialized.
// ADDITIONAL_COMPILE_FLAGS: -Wno-shadow

// RUN: %{build}
// RUN: %{exec} %t.exe

#include <algorithm>
#include <cassert>
#include <meta>
#include <ranges>
#include <vector>

#include "test_macros.h"

struct Fun {
    std::meta::token_sequence args;
    std::meta::token_sequence body;
};

__macro fun(std::meta::token_sequence tokens) {
    auto [args, body] = do -> Fun {
        auto args = std::meta::list_builder(^^{ , });

        for (size_t i = 0; i < tokens.size(); ++i) {
            auto const tok = tokens[i];
            if (token_kind_of(tok) == std::meta::token_kind::identifier) {
                args += ^^{ auto&& $tok };
            } else if (tok == ^^{ = }) {
                if (tokens[i + 1] != ^^{ > }) {
                    throw "expected '=>' token";
                }

                do_return {
                    .args = args,
                    .body = tokens.subseq(i + 2),
                };
            }
        }

        throw "failed to parse fun";
    };

    return ^^{
        [&]($args) -> decltype(auto) {
            return $body;
        }
    };
}


struct Point {
  int x;
  int y;
};

int main(int, char**) {
  auto get_x = fun!(p => p.x);
  Point p{3, 4};
  assert(get_x(p) == 3);

  std::vector<Point> points = {{1, 2}, {3, 1}, {2, 0}};
  std::ranges::sort(points, fun!((l, r) => l.y < r.y));
  assert(points[0].y == 0 && points[1].y == 1 && points[2].y == 2);

  // the parameters are the identifiers before the =>, in order
  auto sub = fun!((a, b) => a - b);
  assert(sub(10, 4) == 6);

  // the body is a token sequence of any length
  auto body = fun!(n => n * n + 1);
  assert(body(4) == 17);
  return 0;
}
