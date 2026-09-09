#include <meta>
#include <memory>
#include <tuple>
int g(std::tuple<std::unique_ptr<int>, char, double, float> tup) { template for (auto &elem : tup) { (void)elem; } return 0; }
