//===----------------------------------------------------------------------===//
// P3795R2 data_member_options::annotations
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection -fannotation-attributes

#include <meta>

using namespace std::meta;

namespace {
struct S;

consteval {
  define_aggregate(^^S, {
      data_member_spec(^^int, {.name = "member",
                               .annotations = {reflect_constant(42),
                                               reflect_constant(7.0f)}})});
}

static_assert(annotations_of(^^S::member).size() == 2);
static_assert(extract<int>(annotations_of(^^S::member)[0]) == 42);
static_assert(extract<float>(annotations_of(^^S::member)[1]) == 7.0f);

consteval bool test_exception() {
  try {
    throw exception(u8"test", ^^data_member_spec);
  } catch (const exception &e) {
    return e.from() == ^^data_member_spec;
  }
  return false;
}

static_assert(test_exception());
} // namespace

int main() {}
