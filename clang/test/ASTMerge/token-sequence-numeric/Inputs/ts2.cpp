#include "token-sequence.h"

namespace std::meta {
using info = decltype(^^::);
consteval auto queue_injection(token_sequence) -> void;
consteval auto queue_injection(info, token_sequence) -> void;
}

// Adjacent numeric literals: the importer gives the destination its own
// NUL-terminated copy of each spelling (the source spellings sit one after the
// other in a PCH blob, and in the context's allocator without padding).
constexpr std::meta::token_sequence adjacent = ^^{ 12345678, 90123456, 78 };

// Injected from the merged translation unit, which has no token literal of its
// own (and so no definition of the class).
consteval void inject_adjacent() {
  std::meta::queue_injection(^^{ constexpr int arr[] = { \(adjacent) }; });
}

// The form with a target namespace is imported with its operand.
namespace target {}
consteval void inject_into_target() {
  std::meta::queue_injection(^^target, ^^{ constexpr int tgt[] = { \(adjacent) }; });
}
