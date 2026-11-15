#include <__meta/token_locations.h>
import Location;

static_assert(std::meta::__x::__token_begin(locations[0]).line() == 8);
static_assert(std::meta::__x::__token_end(locations[0]).column() ==
              std::meta::__x::__token_begin(locations[0]).column() + 10);
static_assert(std::meta::__x::__token_end(locations[1]).column() ==
              std::meta::__x::__token_begin(locations[1]).column() + 1);
