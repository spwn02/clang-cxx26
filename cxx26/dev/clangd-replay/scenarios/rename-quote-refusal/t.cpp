namespace std::meta {
template <class... Ts> consteval auto id(Ts const &...) -> int;
using token = decltype(id("x"));
class token_sequence {
public:
  constexpr token_sequence() noexcept = default;
  friend consteval auto operator+=(token_sequence &, const token_sequence &)
      -> token_sequence &;
  friend consteval auto operator+(token_sequence, const token_sequence &)
      -> token_sequence;
  friend consteval bool operator==(const token_sequence &,
                                   const token_sequence &);
};
} // namespace std::meta
using std::meta::token_sequence;
void target() {}
constexpr auto quote = ^^{ target(); };
int main() { target(); }
