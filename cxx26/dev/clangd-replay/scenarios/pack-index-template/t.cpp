template <typename> struct A { };
template <template <typename> class... TT>
using Sel = TT...[0]<int>;
