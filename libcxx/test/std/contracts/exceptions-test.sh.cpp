// UNSUPPORTED: no-exceptions

// RUN: %{build} -std=c++26 -fcontract-evaluation-semantic=observe -DSEM_OBSERVE -o %t.observe.exe
// RUN: %{exec} %t.observe.exe
// RUN: %{build} -std=c++26 -fcontract-evaluation-semantic=enforce -DSEM_ENFORCE -o %t.enforce.exe
// RUN: %{exec} %t.enforce.exe

#include <contracts>
#include "nttp_string.h"
#include "contracts_support.h"
#include "contracts_handler.h"
#include "test_register.h"
#include <exception>
#include <stdexcept>

namespace exception_handling {

#ifdef SEM_OBSERVE
void foo() {
  contract_assert((true) ? throw 42 : true);
}

REGISTER_TEST(test_ex_handling_observe) {
  ContractHandlerInstaller CHI;
  int called = 0;
  CHI.install([&]() {
    ++called;
    assert(std::current_exception());
  });
  contract_assert((true) ? throw 42 : true);
  assert(called);
};
#endif // SEM_OBSERVE

#ifdef SEM_ENFORCE
REGISTER_TEST(test_ex_handling_enforce) {
  ContractHandlerInstaller CHI;
  int called = 0;
  CHI.install([&]() {
    ++called;
    assert(!std::current_exception());
    assert(KV<"Local"> == 1);
    assert(KV<"Local2"> == 0);

    throw 42;
  });

  try {

    [] {
      CAliveCounter<"Local"> Local;
      contract_assert((CAliveCounter<"Local2">{}, false));
      assert(KV<"Local2"> == 1);
    }();
    assert(false);
  } catch (int) {
    KV<"InCatch">++;
  }
  assert(called == 1);
  assert(NC<"Local"> == 0);
  assert(KV<"Local2"> == 0);
  assert(NC<"InCatch">.consume() == 1);

};
#endif // SEM_ENFORCE

} // namespace exception_handling
