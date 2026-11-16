#include <__meta/token_locations.h>
namespace mx = std::meta::__x;
using std::meta::token_sequence;
__macro offer(token_sequence raw) {
  for (std::meta::token t : raw)
    if (mx::__is_completion(t)) {
      (void)mx::__complete(t, "hx_alpha", "alpha",
                           mx::__completion_kind::property);
      (void)mx::__complete(t, "hx_beta", "beta",
                           mx::__completion_kind::field);
    }
  return ^^{ };
}
void probe() { offer!(<div cla/*CURSOR*/ss="x">); }
