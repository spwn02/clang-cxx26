#include "../include/contracts"
#include <__config>
#include <__hardened_violation>
#include <__verbose_abort>
#include <exception>
#include <iostream>

using namespace std::contracts;

// [basic.contract.handler]: no standard library header declares the default
// contract-violation handler, and a replacement may have a non-throwing exception
// specification. Declare it here, weakly, for the runtime only (the exception
// specification is not part of the mangled name).
__attribute__((weak)) void handle_contract_violation(const std::contracts::contract_violation&);

struct std::contracts::_ContractViolationImpl {
  _AssertKind kind              = _AssertKind::assert;
  _EvaluationSemantic semantic  = _EvaluationSemantic::enforce;
  _DetectionMode mode           = _DetectionMode::predicate_false;
  const char* comment           = nullptr;
  std::source_location location = std::source_location::current();

  contract_violation __create_violation() const { return contract_violation{this}; }
};

namespace {

static void __default_violation_handler(const contract_violation& violation) {
  using namespace std::contracts;
  std::cerr << violation.location().file_name() << ":" << violation.location().line() << ": ";
  auto assert_str = [&]() -> std::pair<const char*, const char*> {
    switch (violation.kind()) {
    case _AssertKind::pre:
      return {"pre(", ")"};
    case _AssertKind::post:
      return {"post(", ")"};
    case _AssertKind::assert:
      return {"contract_assert(", ")"};
    }
    return {"", ""};
  }();
  std::cerr << assert_str.first << violation.comment() << assert_str.second;
  if (violation.detection_mode() == _DetectionMode::predicate_false) {
    std::cerr << " failed" << std::endl;
  } else {
    std::cerr << " exited via exception" << std::endl;
  }
}

template <int N>
struct _BuiltinContractStruct;

template <>
struct _BuiltinContractStruct<3> {
  enum { VERSION = 3 };
  unsigned version; // the version of the struct
  const char* file;
  const char* function;
  unsigned lineno = 0;
  unsigned column = 0;
  const char* comment;
  unsigned contract_kind;
};

union _BuiltinContractStructUnion {
  _BuiltinContractStruct<3> v3;
};

_ContractViolationImpl create_impl(void* data, _EvaluationSemantic* sem = nullptr, _DetectionMode* mode = nullptr) {
  _BuiltinContractStructUnion* data_union = static_cast<_BuiltinContractStructUnion*>(data);
  switch (data_union->v3.version) {
  case 3:
    return _ContractViolationImpl{
        .kind     = static_cast<_AssertKind>(data_union->v3.contract_kind),
        .semantic = sem ? *sem : _EvaluationSemantic::enforce,
        .mode     = mode ? *mode : _DetectionMode::predicate_false,
        .comment  = data_union->v3.comment,
        .location = std::source_location::__create_from_pointer(
            reinterpret_cast<const char*>(data) + offsetof(_BuiltinContractStruct<3>, file))};
  default:
    return _ContractViolationImpl{
        .kind     = _AssertKind::assert,
        .semantic = sem ? *sem : _EvaluationSemantic::enforce,
        .mode     = mode ? *mode : _DetectionMode::predicate_false,
        .comment  = "<unknown contract violation>",
        .location = std::source_location()};
  }
}

} // end namespace

void std::contracts::invoke_default_contract_violation_handler(const contract_violation& violation) noexcept {
  __default_violation_handler(violation);
}

std::source_location contract_violation::location() const noexcept { return __pimpl_->location; }
const char* contract_violation::comment() const noexcept { return __pimpl_->comment; }
_DetectionMode contract_violation::detection_mode() const noexcept { return __pimpl_->mode; }
bool contract_violation::is_terminating() const noexcept { return __pimpl_->semantic == _EvaluationSemantic::enforce; }
assertion_kind contract_violation::kind() const noexcept { return __pimpl_->kind; }
evaluation_semantic contract_violation::semantic() const noexcept { return __pimpl_->semantic; }

extern "C" _LIBCPP_EXPORTED_FROM_ABI void
__handle_contract_violation_v3(_EvaluationSemantic __semantic, _DetectionMode __mode, void* dataptr);

extern "C" _LIBCPP_EXPORTED_FROM_ABI void
__handle_contract_violation_v3(_EvaluationSemantic __semantic, _DetectionMode __mode, void* dataptr) {
  using namespace std::contracts;
  _ContractViolationImpl impl  = create_impl(dataptr, &__semantic, &__mode);
  contract_violation violation = impl.__create_violation();

  if (::handle_contract_violation)
    ::handle_contract_violation(violation);
  else
    invoke_default_contract_violation_handler(violation);

  if (violation.semantic() == _EvaluationSemantic::enforce) {
    std::terminate();
  }
}



// [intro.compliance.general]: violating a hardened precondition is a contract violation.
[[noreturn]] void std::__handle_hardened_violation(
    const char* __message, const char* __file, const char* __function, unsigned __line) {
  using namespace std::contracts;
  // The layout std::source_location uses (see <source_location>); the object has to outlive the handler call.
  struct _Location {
    const char* __file_name;
    const char* __function_name;
    unsigned __line;
    unsigned __column;
  };
  static thread_local _Location __location;
  __location = {__file, __function, __line, 0};

  _ContractViolationImpl impl{
      .kind     = _AssertKind::pre,
      .semantic = _EvaluationSemantic::enforce,
      .mode     = _DetectionMode::predicate_false,
      .comment  = __message,
      .location = std::source_location::__create_from_pointer(&__location)};
  contract_violation violation = impl.__create_violation();

  if (::handle_contract_violation)
    ::handle_contract_violation(violation);
  else
    invoke_default_contract_violation_handler(violation);

  _LIBCPP_VERBOSE_ABORT("%s", __message);
}
