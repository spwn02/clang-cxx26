struct P { int a, b; };

inline int select(int x, P p) {
  if (case [0, auto b] = p)
    return b;
  return match (x) {
    case 0 => 10;
    case 1 || 2 => 20;
    case auto y if (y > 5) => y;
    case _ => -1;
  };
}

inline bool test(int x) { return match(x, case 1 || 2); }

template <class T>
int dependent(T p) {
  if (case [auto a, auto b] = p)
    return a + b;
  return match (p) { case [0, _] => 1; case _ => 2; };
}
