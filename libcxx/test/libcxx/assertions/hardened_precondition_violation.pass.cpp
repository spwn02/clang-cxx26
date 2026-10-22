//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// [intro.compliance.general]: violating a hardened precondition of a hardened implementation results in a contract
// violation. With the `enforce` assertion semantic, the contract-violation handler ([basic.contract.handler]) is
// called with a violation of kind `pre`, detection mode `predicate_false` and the `enforce` semantic, and the program
// terminates after it returns.

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -U_LIBCPP_HARDENING_MODE -D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG
// ADDITIONAL_COMPILE_FLAGS: -U_LIBCPP_ASSERTION_SEMANTIC -D_LIBCPP_ASSERTION_SEMANTIC=_LIBCPP_ASSERTION_SEMANTIC_ENFORCE

#include <cassert>
#include <contracts>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "test_macros.h"

void handle_contract_violation(const std::contracts::contract_violation& violation) {
  assert(violation.kind() == std::contracts::assertion_kind::pre);
  assert(violation.detection_mode() == std::contracts::detection_mode::predicate_false);
  assert(violation.semantic() == std::contracts::evaluation_semantic::enforce);
  assert(violation.is_terminating());
  assert(std::strstr(violation.comment(), "out of bounds") != nullptr);
  assert(violation.location().line() != 0);
  std::_Exit(0); // the handler ran; do not terminate through the library
}

int main(int, char**) {
  std::vector<int> v;
  (void)v[0];
  return 1; // not reached
}
