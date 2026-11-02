int f(int n) {
  return do [k = n + 1] -> int {
    if (k > 3)
      do_return k;
    do_return 0;
  };
}
