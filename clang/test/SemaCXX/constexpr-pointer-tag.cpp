// RUN: %clang_cc1 -std=c++2c -fsyntax-only -verify %s

// P3125R6: __builtin_pointer_tag_insert / _get / _strip put a small tag into the low bits of a pointer that are zero
// because of the alignment of the object it points into, also in constant expressions.

alignas(8) constexpr int Aligned = 1;
alignas(8) constexpr int Array[4] = {1, 2, 3, 4}; // elements have alignment 4 at their offsets
struct alignas(16) S {
  int a;
  int b;
};
constexpr S Obj{10, 20};
constexpr char Chars[8] = {};

// The tag is part of the pointer value and survives a round trip.
constexpr const void *Tagged = __builtin_pointer_tag_insert(&Aligned, 5, 3);
static_assert(__builtin_pointer_tag_get(Tagged, 3) == 5);
static_assert(__builtin_pointer_tag_strip(Tagged, 3) == &Aligned);
static_assert(*static_cast<const int *>(__builtin_pointer_tag_strip(Tagged, 3)) == 1);
static_assert(Tagged != &Aligned); // tagged pointers differ from untagged ones
static_assert(__builtin_pointer_tag_insert(&Aligned, 5, 3) == Tagged);
static_assert(__builtin_pointer_tag_insert(&Aligned, 4, 3) != Tagged);

// A zero tag does not change the pointer.
static_assert(__builtin_pointer_tag_insert(&Aligned, 0, 3) == &Aligned);
static_assert(__builtin_pointer_tag_get(&Aligned, 3) == 0);

// Subobjects: the designator is kept, so stripping restores the exact subobject.
constexpr const void *TaggedB = __builtin_pointer_tag_insert(&Obj.b, 3, 2);
static_assert(__builtin_pointer_tag_get(TaggedB, 2) == 3);
static_assert(*static_cast<const int *>(__builtin_pointer_tag_strip(TaggedB, 2)) == 20);
constexpr const void *TaggedElem = __builtin_pointer_tag_insert(&Array[3], 2, 2);
static_assert(__builtin_pointer_tag_get(TaggedElem, 2) == 2);
static_assert(*static_cast<const int *>(__builtin_pointer_tag_strip(TaggedElem, 2)) == 4);

// nullptr can be tagged: the tag is all that is left.
constexpr const void *TaggedNull = __builtin_pointer_tag_insert(nullptr, 3, 2);
static_assert(__builtin_pointer_tag_get(TaggedNull, 2) == 3);
static_assert(__builtin_pointer_tag_strip(TaggedNull, 2) == nullptr);
static_assert(__builtin_pointer_tag_insert(nullptr, 0, 2) == nullptr);


// Errors.
constexpr const void *TooBig = __builtin_pointer_tag_insert(&Aligned, 8, 3); // expected-error {{constexpr variable 'TooBig' must be initialized by a constant expression}} expected-note {{tag value 8 does not fit in 3 bit(s)}}
constexpr const void *TooManyBits = __builtin_pointer_tag_insert(&Aligned, 1, 4); // expected-error {{constexpr variable 'TooManyBits' must be initialized by a constant expression}} expected-note {{the object the pointer points into is only aligned to 8 byte(s)}}
constexpr const void *Unaligned = __builtin_pointer_tag_insert(&Chars[1], 1, 1); // expected-error {{constexpr variable 'Unaligned' must be initialized by a constant expression}} expected-note {{the object the pointer points into is only aligned to 1 byte(s)}}
constexpr const void *Misaligned = __builtin_pointer_tag_insert(&Array[1], 1, 3); // expected-error {{constexpr variable 'Misaligned' must be initialized by a constant expression}} expected-note {{cannot tag a pointer that is not aligned to 8 byte(s)}}
constexpr const void *PastEnd = __builtin_pointer_tag_insert(Array + 4, 1, 2); // expected-error {{constexpr variable 'PastEnd' must be initialized by a constant expression}} expected-note {{cannot tag a pointer past the end of an object}}
constexpr const void *BadBits = __builtin_pointer_tag_insert(&Aligned, 0, 64); // expected-error {{constexpr variable 'BadBits' must be initialized by a constant expression}} expected-note {{invalid number of pointer tag bits 64}}
constexpr unsigned long GetUnaligned = __builtin_pointer_tag_get(&Chars[0], 1); // expected-error {{constexpr variable 'GetUnaligned' must be initialized by a constant expression}} expected-note {{the object the pointer points into is only aligned to 1 byte(s)}}

// An already tagged pointer cannot be tagged again.
constexpr const void *Twice = __builtin_pointer_tag_insert(Tagged, 1, 3); // expected-error {{constexpr variable 'Twice' must be initialized by a constant expression}} expected-note {{cannot tag a pointer that is not aligned to 8 byte(s)}}

// A heap allocation has the alignment of the allocated type.
constexpr bool Heap() {
  S *p = new S{1, 2};
  const void *t = __builtin_pointer_tag_insert(p, 9, 4);
  bool ok = __builtin_pointer_tag_get(t, 4) == 9 && static_cast<S *>(__builtin_pointer_tag_strip(t, 4)) == p;
  delete p;
  return ok;
}
static_assert(Heap());
