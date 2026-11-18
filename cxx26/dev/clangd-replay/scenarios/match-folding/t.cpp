int f(int x) {
  return match (x) {
    case 0 => 1;
    case _ => 2;
  };
}
int g(int x) {
  match (x) {
    case 0 => x;
    case _ => x + 1;
  }
  return match (x,
                case 0,
                case _) ? 1 : 2;
}
