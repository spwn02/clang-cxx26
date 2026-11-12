void f(bool b) { (void)b; }
auto k() { return 1; }
auto kwritten() -> int { return 1; }
template <bool B> requires (B) void req1() {}
template <bool B> void req2() requires (B) {}
template <bool B> concept C = B;
