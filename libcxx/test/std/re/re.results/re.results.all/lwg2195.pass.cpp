// <regex>

#include <regex>

int main() {
  std::match_results<const char*> __source;
  std::match_results<const char*> __copy(__source, std::allocator<std::sub_match<const char*>>{});
  std::match_results<const char*> __move(std::move(__source), std::allocator<std::sub_match<const char*>>{});
  (void)__copy;
  (void)__move;
}
