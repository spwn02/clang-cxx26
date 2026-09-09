#include <meta>
template <typename T> void print_fields() {
  constexpr std::meta::info class_info = ^^T;
  template for (constexpr auto field : std::define_static_array(std::meta::nonstatic_data_members_of(class_info, std::meta::access_context::unchecked()))) {
    constexpr auto field_info = std::meta::display_string_of(std::meta::type_of(field));
    (void)field_info;
  }
}
struct Car { std::vector<int> cost; int amountOfWheels; };
int main() { print_fields<Car>(); }
