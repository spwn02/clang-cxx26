#include <__meta/token_locations.h>
#include <meta>

__macro bad(std::meta::token_sequence tokens) {
  std::meta::__x::__error(tokens[0], "bad name",
                          std::meta::__x::__replace(tokens[0], tokens[0], "good"));
  std::meta::__x::__error(tokens[1], "second problem");
  return ^^{ 0 };
}
int a = bad!(wrong other);
int after = 1;
