// A minimal std::meta::token_sequence, matching the token-seq scenario.
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
  friend consteval bool operator==(token, const token_sequence &);
};
} // namespace std::meta

using std::meta::token_sequence;

__macro declare_it(token_sequence name) { return ^^{ int \(name) = 1; }; }
declare_it!(zz);
int use_zz() { return zz; }

__macro bad(token_sequence t) { return ^^{ int w = \(t); }; }
bad!(undeclared_name);
