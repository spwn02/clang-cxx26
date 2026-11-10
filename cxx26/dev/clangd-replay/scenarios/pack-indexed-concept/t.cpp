template <class T> concept Any = true;
template <template <class> concept... CC>
struct S {
  template <CC...[0] T> void f(T);
  void g(CC...[1] auto);
};
