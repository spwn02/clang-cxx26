struct I {
  int v;
  I operator++(int) = default;
};
int f() { I i{1}; i++; return i.v; }
