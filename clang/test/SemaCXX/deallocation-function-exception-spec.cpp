// RUN: %clang_cc1 -std=c++23 -fsyntax-only -verify=cxx23 %s
// RUN: %clang_cc1 -std=c++2c -fsyntax-only -verify=cxx26 %s
// cxx23-no-diagnostics

// P3424R2: a deallocation function shall not have a potentially throwing exception specification (C++26).

namespace std {
using size_t = decltype(sizeof(0));
struct destroying_delete_t { explicit destroying_delete_t() = default; };
inline constexpr destroying_delete_t destroying_delete{};
} // namespace std

struct ImplicitlyNoexcept {
  void operator delete(void*);
  void operator delete[](void*, std::size_t);
};
struct NoexceptTrue {
  void operator delete(void*) noexcept(true);
  void operator delete[](void*) noexcept;
};
struct ThrowNothing {
  void operator delete(void*) throw();
};

struct NoexceptFalse {
  void operator delete(void*) noexcept(false); // cxx26-error {{deallocation function 'operator delete' must not have a potentially throwing exception specification}}
};
struct ArrayNoexceptFalse {
  void operator delete[](void*) noexcept(false); // cxx26-error {{deallocation function 'operator delete[]' must not have a potentially throwing exception specification}}
};
struct DestroyingNoexceptFalse {
  void operator delete(DestroyingNoexceptFalse*, std::destroying_delete_t) noexcept(false); // cxx26-error {{deallocation function 'operator delete' must not have a potentially throwing exception specification}}
};
struct SizedAndAligned {
  void operator delete(void*, std::size_t) noexcept(false); // cxx26-error {{deallocation function 'operator delete' must not have a potentially throwing exception specification}}
};
struct Delayed {
  static constexpr bool Throws = true;
  void operator delete(void*) noexcept(!Throws); // cxx26-error {{deallocation function 'operator delete' must not have a potentially throwing exception specification}}
};

void operator delete(void*, double) noexcept(false); // cxx26-error {{deallocation function 'operator delete' must not have a potentially throwing exception specification}}
void operator delete[](void*, double) noexcept;

// A class template: the exception specification is checked once it is instantiated.
template <class T>
struct Templated {
  void operator delete(void*) noexcept(sizeof(T) == 1); // cxx26-error {{deallocation function 'operator delete' must not have a potentially throwing exception specification}}
};
Templated<char> ok;
void use(Templated<int>* p) { delete p; } // cxx26-note {{in instantiation of exception specification for 'operator delete' requested here}}

// A function that is not a deallocation function may throw.
struct NotDeallocation {
  void deallocate(void*) noexcept(false);
  static void* operator new(std::size_t) noexcept(false);
};
