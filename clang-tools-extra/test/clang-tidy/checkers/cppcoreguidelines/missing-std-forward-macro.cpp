// RUN: %check_clang_tidy -std=c++29 %s cppcoreguidelines-missing-std-forward %t

// A minimal std::meta::token_sequence: a token literal needs the class defined.
namespace std::meta {
class token_sequence {
public:
  constexpr token_sequence() noexcept = default;
  friend consteval auto operator+=(token_sequence &, const token_sequence &)
      -> token_sequence &;
};
} // namespace std::meta

// A token-sequence macro takes its parameter by forwarding reference and
// splices it into the result: there is nothing to forward, so no warning.
template <class T>
macro fwd(T &&value) {
  return ^^{ \(value) };
}

// An ordinary function that does not forward is still diagnosed.
template <class T>
void ordinary(T &&value) {}
// CHECK-MESSAGES: :[[@LINE-1]]:19: warning: forwarding reference parameter 'value' is never forwarded inside the function body [cppcoreguidelines-missing-std-forward]
