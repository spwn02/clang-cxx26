# P3037R6 `constexpr` `shared_ptr` audit

Issue #17 (Wave 4) starts from commits `9b5c9c9f380b` and
`2811cb52368a`. The former added the non-array `shared_ptr` allocation and
reference-counting path; the latter fixed its out-of-line virtual declaration
and custom-allocator instantiation hazards.

## Current gap

The existing landing covers the default/nullptr, same-type copy/move,
destructor, same-type assignment, `swap`, empty `reset`, core observers, and
the non-array `make_shared`/`allocate_shared` entry points. It does not yet
cover the remaining P3037R6 declarations:

- converting `shared_ptr` constructors and assignments, the `weak_ptr`
  constructor, `unique_ptr` constructor/assignment, and pointer/deleter
  `reset` overloads;
- `owner_before`, array indexing, comparisons, specialized `swap`, the
  supported pointer casts, and `get_deleter`;
- all `weak_ptr` constructors, destructor, assignments, modifiers, observers,
  and its constant-evaluation lock path;
- array `shared_ptr` factory overloads and their control-block destruction
  paths;
- all `enable_shared_from_this` special members, `shared_from_this`, and
  `weak_from_this`.

P3037R6 also specifies the array factory overloads as `constexpr`; they are in
scope. Hash/owner-hash and `reinterpret_pointer_cast` remain non-`constexpr`,
as their constant-evaluation implementations require operations excluded by
the paper. The implementation will add function qualifiers and constant-only
plain reference-count/deallocation paths, without changing object layout or
ABI.

