// ADDITIONAL_COMPILE_FLAGS: -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -fcontract-group-evaluation-semantic=observe=observe,enforce=enforce -g

#include "nttp_string.h"
#include "contracts_support.h"
#include "contracts_handler.h"

#include "check_assertion.h"
using namespace std::contracts;

void my_handler(const contract_violation&) {

}

int main() {
  ContractHandlerInstaller install_handler_guard(my_handler) ;

}