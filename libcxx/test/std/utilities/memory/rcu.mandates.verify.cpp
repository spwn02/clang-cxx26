// REQUIRES: std-at-least-c++26
// UNSUPPORTED: no-threads

// <rcu>
// Mandates that T is an rcu-protectable type.

#include <rcu>

// expected-note@*:* 0+ {{in instantiation of}}
// expected-note@*:* 0+ {{explicitly marked deleted here}}
// expected-note@*:* 0+ {{candidate function not viable}}

struct Other;
struct DifferentBaseTypes : std::rcu_obj_base<DifferentBaseTypes>, std::rcu_obj_base<Other> {};
struct BadMoveDeleter {
  BadMoveDeleter()                 = default;
  BadMoveDeleter(BadMoveDeleter&&) = delete;
  void operator()(int*) const;
};
struct BadCallDeleter {
  void operator()(double*) const;
};

void test(DifferentBaseTypes& object) {
  static_cast<std::rcu_obj_base<DifferentBaseTypes>&>(object)
      .retire(); // expected-error@*:* {{T must be an rcu-protectable type}}
}

void deleter_mandates(int* ptr) {
  std::rcu_retire(ptr, BadMoveDeleter{}); // expected-error@*:* {{call to deleted constructor of 'BadMoveDeleter'}}
  std::rcu_retire(ptr, BadCallDeleter{}); // expected-error@*:* {{no matching function for call to object}}
}
