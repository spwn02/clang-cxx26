// RUN: %clang_cc1 -std=c++29 -fsyntax-only \
// RUN:   -code-completion-at=%s:5:1 %s | FileCheck %s

struct Option {
  constexpr Option() noexcept(sizeof(int)::)
  = default;
};

// CHECK: COMPLETION: Option : Option
