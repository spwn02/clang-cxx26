// RUN: %clang_cc1 -std=c++2c -verify %s

// Constructing the elements of an array whose own elements are arrays, one leaf at a time (std::allocator<int[2]>
// provides storage for int[2] objects that are not alive yet): the array element comes into existence implicitly
// ([intro.object], arrays are implicit-lifetime types) when one of its elements is constructed.
//
// Placement `new T[1]` over storage that holds an object of type T (std::construct_at of an array type is specified as
// `::new (voidify(*location)) T[1]()`) constructs that single object in place.

namespace std {
using size_t = decltype(sizeof(0));
template <class T> struct allocator {
  constexpr T* allocate(size_t n) { return static_cast<T*>(::operator new(n * sizeof(T))); }
  constexpr void deallocate(T* p, size_t) { ::operator delete(p); }
};
} // namespace std

constexpr void* operator new(std::size_t, void* p) noexcept { return p; }
constexpr void* operator new[](std::size_t, void* p) noexcept { return p; }

using A = int[2];
using AA = int[2][3];

constexpr bool value_init_array_element() {
  std::allocator<A> a;
  A* p = a.allocate(1);
  A* q = ::new (static_cast<void*>(p)) A[1]();
  bool ok = q == p && (*q)[0] == 0 && (*q)[1] == 0;
  (*q)[1] = 5;
  ok = ok && (*p)[1] == 5;
  a.deallocate(p, 1);
  return ok;
}
static_assert(value_init_array_element());

constexpr bool default_init_array_element() {
  std::allocator<A> a;
  A* p = a.allocate(1);
  A* q = ::new (static_cast<void*>(p)) A[1];
  (*q)[0] = 1;
  (*q)[1] = 2;
  bool ok = (*p)[0] == 1 && (*p)[1] == 2;
  a.deallocate(p, 1);
  return ok;
}
static_assert(default_init_array_element());

constexpr bool nested_array_element() {
  std::allocator<AA> a;
  AA* p = a.allocate(1);
  AA* q = ::new (static_cast<void*>(p)) AA[1]();
  bool ok = (*q)[1][2] == 0;
  a.deallocate(p, 1);
  return ok;
}
static_assert(nested_array_element());

// The element is replaced in place: a second construction over the same storage is fine.
constexpr bool reconstruct() {
  std::allocator<A> a;
  A* p = a.allocate(2);
  ::new (static_cast<void*>(p)) A[1]();
  (*p)[0] = 3;
  ::new (static_cast<void*>(p)) A[1]();
  bool ok = (*p)[0] == 0;
  ::new (static_cast<void*>(p + 1)) A[1]();
  ok = ok && (*(p + 1))[1] == 0;
  a.deallocate(p, 2);
  return ok;
}
static_assert(reconstruct());

// An array of arrays cannot be placed over an array of a different shape.
constexpr int wrong_shape() {
  int storage[3] = {1, 2, 3};
  ::new (static_cast<void*>(&storage)) int[1][2](); // expected-note {{placement new would change type of storage from 'int[3]' to 'int[1][2]'}}
  return storage[0];
}
constexpr int use_wrong_shape = wrong_shape(); // expected-error {{must be initialized by a constant expression}} \
                                               // expected-note {{in call to}}

// A one-element array of the same shape as the storage is the object itself.
constexpr bool same_shape() {
  int storage[2] = {1, 2};
  int(*p)[2] = ::new (static_cast<void*>(&storage)) int[1][2]();
  return (*p)[0] == 0 && storage[1] == 0;
}
static_assert(same_shape());

constexpr bool leaf_by_leaf() {
  std::allocator<A> a;
  A* p = a.allocate(2);
  for (int i = 0; i != 2; ++i)
    for (int j = 0; j != 2; ++j)
      ::new (static_cast<void*>(&p[i][j])) int(i * 2 + j);
  bool ok = p[0][0] == 0 && p[0][1] == 1 && p[1][0] == 2 && p[1][1] == 3;
  a.deallocate(p, 2);
  return ok;
}
static_assert(leaf_by_leaf());

constexpr bool leaf_by_leaf_3d() {
  std::allocator<AA> a;
  AA* p = a.allocate(1);
  for (int i = 0; i != 2; ++i)
    for (int j = 0; j != 3; ++j)
      ::new (static_cast<void*>(&(*p)[i][j])) int(i * 3 + j);
  bool ok = (*p)[1][2] == 5;
  a.deallocate(p, 1);
  return ok;
}
static_assert(leaf_by_leaf_3d());

// Reading a leaf that was never constructed is still an error.
constexpr int read_unconstructed() {
  std::allocator<A> a;
  A* p = a.allocate(1);
  ::new (static_cast<void*>(&p[0][0])) int(1);
  int v = p[0][1]; // expected-note {{read of object outside its lifetime is not allowed in a constant expression}}
  a.deallocate(p, 1);
  return v;
}
constexpr int use_read_unconstructed = read_unconstructed(); // expected-error {{must be initialized by a constant expression}} \
                                                             // expected-note {{in call to}}
