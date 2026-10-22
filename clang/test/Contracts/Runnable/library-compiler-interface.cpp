// The compiler-library callback: the violation handler gets the evaluation
// semantic of the translation unit (-fcontract-evaluation-semantic=), the
// source location of the assertion and the detection mode; observe and ignore
// are distinguished by whether the predicate is evaluated and the handler runs.
//
// RUN: %clang -std=c++26 -nostdlib++ -fcontract-evaluation-semantic=observe -DSEM_OBSERVE %s -o %t.observe -g
// RUN: %t.observe 0
// RUN: %t.observe 1
// RUN: %clang -std=c++26 -nostdlib++ -fcontract-evaluation-semantic=enforce -DSEM_ENFORCE %s -o %t.enforce -g
// RUN: %t.enforce 0
// RUN: %t.enforce 1
// RUN: %clang -std=c++26 -nostdlib++ -fcontract-evaluation-semantic=ignore -DSEM_IGNORE %s -o %t.ignore -g
// RUN: %t.ignore 0
// RUN: %t.ignore 1

#include "contracts.h"
#include "my_assert.h"
#include <stdio.h>
#include <string.h>

// This is the layout of the data emitted by clang for version 3 of callback
struct BuiltinContractStruct {
  enum { VERSION = 3 };
  unsigned version; // the version of the struct
  const char* file;
  const char* function;
  unsigned lineno = 0;
  unsigned column = 0;
  const char* comment;
  unsigned contract_kind;
};



bool location_equals(std::source_location LHS, std::source_location RHS, int rhs_offset = 0) {
  bool value = __builtin_strcmp(LHS.file_name(), RHS.file_name()) == 0
  && __builtin_strcmp(LHS.function_name(), RHS.function_name()) == 0
  && LHS.line() == RHS.line() + rhs_offset;
  if (!value) {
    const char* fmt_str = R"cpp(
Assertion Failed: Source locations not equal
Actual  : %s:%d in %s
Expected: %s:%d in %s
)cpp";
    fprintf(stderr, fmt_str, LHS.file_name(), (int)LHS.line(), LHS.function_name(),
            RHS.file_name(), (int)RHS.line() + rhs_offset, RHS.function_name());
  }
  return value;
}

using namespace std::contracts;


evaluation_semantic expected_semantic;
detection_mode expected_mode = detection_mode::predicate_false;
std::source_location expected_loc;
int expected_line_offset = 0;
unsigned handler_called = 0;



extern "C" void __handle_contract_violation_v3(unsigned __sem, unsigned __mode, void *data) {
  using namespace std::contracts;
  ++handler_called;
  assert(__sem == static_cast<unsigned>(expected_semantic));
  evaluation_semantic sem = static_cast<evaluation_semantic>(__sem);
  assert(__mode == static_cast<unsigned>(detection_mode::predicate_false) ||
         __mode == static_cast<unsigned>(detection_mode::evaluation_exception));
  assert(__mode == static_cast<unsigned>(expected_mode));
  detection_mode mode = static_cast<detection_mode>(__mode);
  BuiltinContractStruct *cs = (BuiltinContractStruct*)data;
  void* SourceLoc = reinterpret_cast<char*>(cs) + __builtin_offsetof(BuiltinContractStruct, file);
  std::source_location loc = std::source_location::__create_from_pointer(SourceLoc);
  assert(expected_loc.file_name());
  assert(location_equals(loc, expected_loc, expected_line_offset));
  _ContractViolationImpl impl{.kind = static_cast<assertion_kind>(cs->contract_kind),
                              .semantic = sem,
                              .mode = mode,
                              .comment = cs->comment};
  if (sem == evaluation_semantic::enforce && expected_semantic == evaluation_semantic::enforce) {
    exit(0);
  }
}

void expect_location(int offset, std::source_location loc = std::source_location::current()) {
  ::expected_loc = loc;
  expected_line_offset = offset;
}
unsigned evaluated_count;
bool check_evaluated(bool value) {
  ++evaluated_count;
  return value;
}

void reset_counters() {
evaluated_count = 0;
handler_called = 0;
}

#if defined(SEM_OBSERVE)
constexpr evaluation_semantic kSemantic = evaluation_semantic::observe;
#elif defined(SEM_ENFORCE)
constexpr evaluation_semantic kSemantic = evaluation_semantic::enforce;
#else
constexpr evaluation_semantic kSemantic = evaluation_semantic::ignore;
#endif

// The assertion sets the expected location (relative to its own line) when it
// is evaluated or skipped; for enforce a failing assertion ends the program in
// the handler (exit(0)).
void has_assert(bool x) {
  expect_location(+1);
  contract_assert(check_evaluated(x));
}

void has_pre(bool x)
  pre(check_evaluated(x)) {
  expect_location(-1);
}

void check(void (*fn)(bool)) {
  expected_semantic = kSemantic;
  fn(true); // sets the expected location
  assert(handler_called == 0);
  assert(evaluated_count == (kSemantic == evaluation_semantic::ignore ? 0u : 1u));
  reset_counters();

  fn(false);
  // Only reached by observe and ignore: enforce exits in the handler.
  assert(kSemantic != evaluation_semantic::enforce);
  assert(handler_called == (kSemantic == evaluation_semantic::observe ? 1u : 0u));
  assert(evaluated_count == (kSemantic == evaluation_semantic::ignore ? 0u : 1u));
  reset_counters();
}

void test_contract_assert() { check(has_assert); }

void test_pre_condition() { check(has_pre); }

int main(int argc, char **argv) {
  assert(argc == 2);
  int test_case = atoi(argv[1]);

  switch (test_case) {
  case 0:
    test_contract_assert();
    break;
  case 1:
    test_pre_condition();
    break;
  default:
    assert(false && "Not a valid test case");
  }
  return 0;
}
