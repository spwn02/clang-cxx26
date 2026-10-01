// UNSUPPORTED: no-reflection, c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

#include <meta>

using namespace std::meta;

// These expressions must execute at the call point, rather than in a helper.
#define EXPECT_FUNCTION_THROW()                                               \
  {                                                                           \
    bool caught = false;                                                      \
    try { (void)current_function(); } catch (const exception&) { caught = true; } \
    if (!caught) throw "current_function did not throw";                       \
  }
#define EXPECT_CLASS_THROW()                                                  \
  {                                                                           \
    bool caught = false;                                                      \
    try { (void)current_class(); } catch (const exception&) { caught = true; }   \
    if (!caught) throw "current_class did not throw";                          \
  }
#define CHECK_NAMESPACE(N)                                                    \
  static_assert(current_namespace() == N);                                     \
  static_assert(access_context::current().scope() == N);                       \
  EXPECT_FUNCTION_THROW();                                                    \
  EXPECT_CLASS_THROW()
#define CHECK_CLASS(C, N)                                                      \
  static_assert(current_class() == C);                                         \
  static_assert(is_class_type(access_context::current().scope()));              \
  static_assert(access_context::current().scope() == C);                       \
  static_assert(current_namespace() == N);                                     \
  EXPECT_FUNCTION_THROW()
#define CHECK_FUNCTION(F, N)                                                   \
  static_assert(current_function() == F);                                      \
  static_assert(is_function(access_context::current().scope()));                \
  static_assert(access_context::current().scope() == F);                       \
  static_assert(current_namespace() == N)

consteval { CHECK_NAMESPACE(^^::); }
namespace outer {
consteval { CHECK_NAMESPACE(^^outer); }
namespace inner {
consteval {
  CHECK_NAMESPACE(^^inner);
  { consteval { CHECK_NAMESPACE(^^inner); } }
  [] {
    // Ordinary lambdas retain their own scope, even inside a consteval block.
    static constexpr info function = current_function();
    static_assert(is_function(function));
    static_assert(is_class_type(current_class()));
    static_assert(access_context::current().scope() == function);
    static_assert(current_namespace() == ^^inner);
    consteval {
      CHECK_FUNCTION(function, ^^inner);
      static_assert(current_class() == parent_of(function));
      consteval { CHECK_FUNCTION(function, ^^inner); }
    }
  }();
}
struct C {
  consteval {
    CHECK_CLASS(^^C, ^^inner);
    consteval { CHECK_CLASS(^^C, ^^inner); }
  }
  void member() {
    consteval {
      CHECK_FUNCTION(^^member, ^^inner);
      static_assert(current_class() == ^^C);
    }
  }
};
template<class T> struct CT {
  consteval { CHECK_CLASS(^^CT, ^^inner); }
  void member() {
    consteval {
      CHECK_FUNCTION(^^member, ^^inner);
      static_assert(current_class() == ^^CT);
    }
  }
};
CT<int> instance;
template void CT<int>::member();
void function() {
  { consteval { CHECK_FUNCTION(^^function, ^^inner); EXPECT_CLASS_THROW(); } }
}
template<class T> void function_template() {
  consteval {
    CHECK_FUNCTION(^^function_template<T>, ^^inner);
    EXPECT_CLASS_THROW();
  }
}
template void function_template<int>();
}
}

int main() {}
