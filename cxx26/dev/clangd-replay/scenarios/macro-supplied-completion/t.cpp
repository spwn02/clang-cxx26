#include <__meta/token_locations.h>
namespace mx = std::meta::__x;
using std::meta::token_sequence;

__macro html(token_sequence raw) {
  for (std::meta::token t : raw)
    if (mx::__is_completion(t)) {
      (void)mx::__complete(t, "hx_alpha", "alpha",
                           mx::__completion_kind::property, "attribute",
                           "Alpha HTML attribute");
      (void)mx::__complete(t, "hx_beta", "beta", mx::__completion_kind::field,
                           "field", "Beta HTML attribute");
    }
  return ^^{ };
}
__macro inject(token_sequence raw) {
  for (std::meta::token t : raw)
    if (mx::__is_completion(t))
      (void)mx::__complete(t, "hx_gamma", "gamma", mx::__completion_kind::text);
  return ^^{ \(raw) };
}
int partial_value;
void dsl_prefix() { html!(<div class="x">); }
void dsl_middle() { html!(<div class="x">); }
void dsl_empty() { html!(<div >); }
void dsl_string() { html!(<div title="ab">); }
void injected() { (void)inject!(partial); }
