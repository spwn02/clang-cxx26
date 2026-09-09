#include <meta>
#include <utility>
static_assert(^^decltype(std::move(1)) != ^^int);
