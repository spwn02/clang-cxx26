#include <meta>
namespace n {}
template <std::meta::info info> consteval int f() {
  int i = 0;
  return [&]<int x>() {
    template for (constexpr std::meta::info member : std::define_static_array(std::meta::members_of(info, std::meta::access_context::unchecked()))) i += 1;
    return i;
  }.template operator()<0>();
}
static_assert(f<^^n>() == 0);
