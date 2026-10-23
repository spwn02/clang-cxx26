// REQUIRES: std-at-least-c++26

// <atomic>
// Mandates that pointer atomic arithmetic has a complete object pointee type.

#include <atomic>

// expected-note@*:* 0+ {{in instantiation of}}

struct FetchMaxType;
struct FetchMinType;
struct StoreMaxType;
struct StoreMinType;

void fetch_max_mandates(std::atomic<FetchMaxType*>& value, FetchMaxType* operand) {
  (void)value.fetch_max(
      operand); // expected-error@*:* {{Mandates: atomic pointer arithmetic requires a complete object type}}
}

void fetch_min_mandates(std::atomic<FetchMinType*>& value, FetchMinType* operand) {
  (void)value.fetch_min(
      operand); // expected-error@*:* {{Mandates: atomic pointer arithmetic requires a complete object type}}
}

void store_max_mandates(std::atomic<StoreMaxType*>& value, StoreMaxType* operand) {
  value.store_max(
      operand); // expected-error@*:* {{Mandates: atomic pointer arithmetic requires a complete object type}}
}

void store_min_mandates(std::atomic<StoreMinType*>& value, StoreMinType* operand) {
  value.store_min(
      operand); // expected-error@*:* {{Mandates: atomic pointer arithmetic requires a complete object type}}
}

void volatile_fetch_max_mandates(volatile std::atomic<FetchMaxType*>& value, FetchMaxType* operand) {
  (void)value.fetch_max(
      operand); // expected-error@*:* {{Mandates: atomic pointer arithmetic requires a complete object type}}
}

void volatile_fetch_min_mandates(volatile std::atomic<FetchMinType*>& value, FetchMinType* operand) {
  (void)value.fetch_min(
      operand); // expected-error@*:* {{Mandates: atomic pointer arithmetic requires a complete object type}}
}

void volatile_store_max_mandates(volatile std::atomic<StoreMaxType*>& value, StoreMaxType* operand) {
  value.store_max(
      operand); // expected-error@*:* {{Mandates: atomic pointer arithmetic requires a complete object type}}
}

void volatile_store_min_mandates(volatile std::atomic<StoreMinType*>& value, StoreMinType* operand) {
  value.store_min(
      operand); // expected-error@*:* {{Mandates: atomic pointer arithmetic requires a complete object type}}
}
