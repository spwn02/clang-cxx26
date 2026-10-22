// ADDITIONAL_COMPILE_FLAGS: -std=c++26 -fcontract-evaluation-semantic=observe
#include <cassert>
#include <contracts>
#include <iostream>
#include <vector>
#include <compare>
#include <source_location>
#include <utility>
#include <tuple>
#include <fstream>
#include <unordered_map>
#include <format>
#include <vector>
#include <map>
#include <variant>
#include <regex>
#include <string>
#include <stdexcept>
#include <set>
#include "check_assertion.h"
#include "dump_struct.h"
#include "contracts_support.h"

// NO-EXEC: compile-only header-inclusion smoke test for contracts against
// the full standard library surface; main() is intentionally empty.
int main() {}