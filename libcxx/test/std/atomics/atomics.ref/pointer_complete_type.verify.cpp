// REQUIRES: std-at-least-c++26

// <atomic>
// Mandates that the pointee type is a complete object type for pointer arithmetic.

#include <atomic>

// expected-note@*:* 0+ {{in instantiation of}}

struct IncompleteFetchMax;
struct IncompleteFetchMin;
struct IncompleteStoreMax;
struct IncompleteStoreMin;

void fetch_max_mandates(IncompleteFetchMax*& value) {
  std::atomic_ref<IncompleteFetchMax*> ref(value);
  ref.fetch_max(
      value); // expected-error@*:* {{Mandates: atomic_ref pointer arithmetic requires a complete object type}}
}

void fetch_min_mandates(IncompleteFetchMin*& value) {
  std::atomic_ref<IncompleteFetchMin*> ref(value);
  ref.fetch_min(
      value); // expected-error@*:* {{Mandates: atomic_ref pointer arithmetic requires a complete object type}}
}

void store_max_mandates(IncompleteStoreMax*& value) {
  std::atomic_ref<IncompleteStoreMax*> ref(value);
  ref.store_max(
      value); // expected-error@*:* {{Mandates: atomic_ref pointer arithmetic requires a complete object type}}
}

void store_min_mandates(IncompleteStoreMin*& value) {
  std::atomic_ref<IncompleteStoreMin*> ref(value);
  ref.store_min(
      value); // expected-error@*:* {{Mandates: atomic_ref pointer arithmetic requires a complete object type}}
}
