//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26
// UNSUPPORTED: no-threads
// ADDITIONAL_COMPILE_FLAGS: -fexperimental-library

// <io_bridge>

// P4093R1 section 4, the abstraction floor: a result that destructures into (error_code, more...) is rejected at compile
// time by as_sender. The check is structural: io_result<size_t>, tuple<error_code, size_t, bool> and pair<error_code, size_t>
// are all caught.

#include <io_bridge>

#include <cstddef>
#include <tuple>
#include <utility>

namespace io = std::io;

io::task<io::io_result<std::size_t>> read_some_like();
io::task<std::tuple<std::error_code, std::size_t, bool>> tuple_like();
io::task<std::pair<std::error_code, std::size_t>> pair_like();

void rejected() {
  // expected-error-re@*:* {{static assertion failed{{.*}}as_sender does not accept awaitables{{.*}}}}
  (void)io::as_sender(read_some_like());
  // expected-error-re@*:* {{static assertion failed{{.*}}as_sender does not accept awaitables{{.*}}}}
  (void)io::as_sender(tuple_like());
  // expected-error-re@*:* {{static assertion failed{{.*}}as_sender does not accept awaitables{{.*}}}}
  (void)io::as_sender(pair_like());
}
