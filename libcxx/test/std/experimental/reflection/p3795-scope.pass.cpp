// UNSUPPORTED: no-reflection
// ADDITIONAL_COMPILE_FLAGS: -freflection

#include <meta>

using namespace std::meta;

static_assert(current_namespace() == ^^::);

namespace ns {
static_assert(current_namespace() == ^^ns);

consteval info function_scope() {
  return current_function();
}
static_assert(function_scope() == ^^function_scope);

struct S {
  static constexpr info class_scope = current_class();
  static_assert(class_scope == ^^S);

  static consteval info member_scope() {
    return current_class();
  }
};

static_assert(S::member_scope() == ^^S);
}

consteval info invalid_function_scope() {
  return current_namespace();
}

static_assert(invalid_function_scope() == ^^::);

int main() {}
