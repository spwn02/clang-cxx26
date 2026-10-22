// UNSUPPORTED: no-exceptions

// RUN: %{build} -std=c++26 -fcontract-evaluation-semantic=observe -DSEM_OBSERVE -o %t.observe.exe
// RUN: %{exec} %t.observe.exe
// RUN: %{build} -std=c++26 -fcontract-evaluation-semantic=enforce -DSEM_ENFORCE -o %t.enforce.exe
// RUN: %{exec} %t.enforce.exe

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

#ifdef SEM_OBSERVE
REGISTER_TEST(observe_is_not_terminating) {
  ContractHandlerInstaller CHI;
  bool ran = false;
  CHI.install([&](const std::contracts::contract_violation& violation) {
    ran = true;
    assert(violation.semantic() == std::contracts::evaluation_semantic::observe);
    assert(!violation.is_terminating());
  });
  contract_assert(false);
  assert(ran);
};
#endif // SEM_OBSERVE

#ifdef SEM_ENFORCE
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
    contract_assert(false);
    assert(false);
  } catch (int v) {
    assert(v == 42);
  }
  assert(ran);
};
#endif // SEM_ENFORCE

} // namespace is_terminating_test
