// ADDITIONAL_COMPILE_FLAGS: -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe

// [basic.contract.handler]: no standard header declares the default
// contract-violation handler, so a program may replace it with a handler that
// is declared noexcept.

#include <contracts>
#include <cassert>

static int calls = 0;

void handle_contract_violation(const std::contracts::contract_violation&) noexcept { ++calls; }

int main(int, char**) {
  contract_assert(false);
  assert(calls == 1);
  return 0;
}
