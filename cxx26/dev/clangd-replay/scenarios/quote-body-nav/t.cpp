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

struct Point { int x; };
int helper(int v) { return v; }
namespace n1 { int pick() { return 1; } }
namespace n2 { int pick() { return 2; } }
__macro ref_it(int v) { return ^^{ helper(\(v)) + Point{1}.x }; }
__macro decl_it() { return ^^{ int made = 0; }; }
__macro pick_it() { return ^^{ pick() }; }
__macro unused_it() { return ^^{ helper(0) }; }
__macro tpl_it() { return ^^{ helper(1) }; }
int a1() { return ref_it!(1); }
int a2() { return ref_it!(2); }
int d1() { decl_it!(); return made; }
int p1() { using namespace n1; return pick_it!(); }
int p2() { using namespace n2; return pick_it!(); }
template <class T> int tpl() { return tpl_it!(); }
int tpl_use() { return tpl<int>(); }
