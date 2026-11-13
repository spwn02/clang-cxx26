template <class T> struct Box {
  T value;
  template <class Self> T first(this Self &&self) { return self.value; }
  template <class Self> T two(this Self &&self, int a) { return self.value; }
};
template <class T> void dependent(Box<T> box) {
  box.first();
  box.two(1);
}
