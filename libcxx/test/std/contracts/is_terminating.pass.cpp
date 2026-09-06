// ADDITIONAL_COMPILE_FLAGS: -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -fcontract-group-evaluation-semantic=observe=observe,enforce=enforce

// contract_violation::is_terminating() (P3227R1): true iff the violation's
// evaluation semantic is a terminating one (enforce), false otherwise
// (observe). Untracked by any Cxx2cPapers.csv row (Contracts-family wording
// papers collapse into the single P2900R14 row) -- found by comparing this
// fork's <contracts> synopsis directly against eel.is/c++draft's
// [support.contract.violation], not from the CSV.

#include <contracts>
#include "contracts_support.h"
#include "contracts_handler.h"
#include "test_register.h"

namespace is_terminating_test {

REGISTER_TEST(observe_is_not_terminating) {
  ContractHandlerInstaller CHI;
  bool ran = false;
  CHI.install([&](const std::contracts::contract_violation& violation) {
    ran = true;
    assert(violation.semantic() == std::contracts::evaluation_semantic::observe);
    assert(!violation.is_terminating());
  });
  contract_assert [[clang::contract_group("observe")]] (false);
  assert(ran);
};

REGISTER_TEST(enforce_is_terminating) {
  ContractHandlerInstaller CHI;
  bool ran = false;
  CHI.install([&](const std::contracts::contract_violation& violation) {
    ran = true;
    assert(violation.semantic() == std::contracts::evaluation_semantic::enforce);
    assert(violation.is_terminating());
    // An enforce-semantics violation calls std::terminate() after this
    // handler returns; throw instead so the test process itself doesn't
    // die, matching exceptions-test.pass.cpp's established pattern.
    throw 42;
  });
  try {
    contract_assert [[clang::contract_group("enforce")]] (false);
    assert(false);
  } catch (int v) {
    assert(v == 42);
  }
  assert(ran);
};

} // namespace is_terminating_test
