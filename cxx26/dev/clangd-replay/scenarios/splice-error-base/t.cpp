namespace std::meta {
// (The compiler implements id, ignoring the declared return type: a token.)
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
  friend consteval bool operator==(token, const token_sequence &);
};
} // namespace std::meta

namespace std::meta { using info = decltype(^^::); }
using std::meta::info;
using std::meta::token_sequence;

struct Other { int x; };

struct derive {
  consteval auto inject_members(info) const -> token_sequence {
    constexpr info m = ^^Other::x;
    return ^^{ int get() const { return object.\(m); } };
  }
};

struct [[=derive{}]] Point { int y{}; };
