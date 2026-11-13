template <class T> struct Option {};
struct StringView {};
template <class F> struct Sequence {
  template <class G> auto filterMap(G) { return 0; }
};
void complete(Sequence<int> seq) {
  seq.filterMap([](int value) -> Option</*CURSOR*/
}
