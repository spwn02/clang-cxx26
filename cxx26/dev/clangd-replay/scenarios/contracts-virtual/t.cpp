struct B {
  virtual int f(int x) pre(x > 0);
};
struct D : B {
  int f(int x) override pre(x > 1);
};
