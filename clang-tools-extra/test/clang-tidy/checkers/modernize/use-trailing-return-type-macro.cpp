// RUN: %check_clang_tidy -std=c++29 %s modernize-use-trailing-return-type %t

// A minimal std::meta::token_sequence: a token literal needs the class defined.
namespace std::meta {
class token_sequence {
public:
  constexpr token_sequence() noexcept = default;
};
} // namespace std::meta

// A token-sequence macro declaration has no written return type: nothing to
// move to a trailing return type, so no warning and no fix.
__macro identity() {
  return ^^{ 1 };
}

macro empty() {
  return ^^{};
}

// An ordinary function in the same file is still diagnosed.
int ordinary();
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: use a trailing return type for this function [modernize-use-trailing-return-type]
// CHECK-FIXES: auto ordinary() -> int;
