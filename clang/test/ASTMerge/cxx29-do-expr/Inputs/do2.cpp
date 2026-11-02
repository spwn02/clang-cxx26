// The same declarations again: the second import must find the first one.
inline int pick(bool b) {
  return do [q = 1] { if (b) do_return q; do_return 2; };
}
template <class T> constexpr T twice(T v) { return do [k = v] -> T { do_return k + k; }; }
inline constexpr int sum4 = do { int s = 0; for (int i = 0; i < 4; ++i) s += i; do_return s; };
inline long other(long v) { return do { do_return v * 3; }; }
