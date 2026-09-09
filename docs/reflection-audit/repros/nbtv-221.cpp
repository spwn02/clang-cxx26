#include <expected>
#include <vector>
std::expected<bool, int> value();
auto v = std::vector{value(), value()};
