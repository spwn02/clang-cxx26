# [basic.memobj] example of P3726R2: `a.arr[2] = 2` after `std::start_lifetime(a.arr)`, and `constexpr A v1;`

Tracker text for #306. Classification: **editorial** (the example is non-normative). Not submitted; the owner decides
whether it is sent to cplusplus/draft.

## Statement

The example added to [basic.memobj] by P3726R2 (draft commit `b621f1a42`, present in N5050) contains two lines that the
normative text contradicts:

```cpp
struct A { ... union { int i; int arr[4]; Y y; }; };

constexpr A v1;         // OK, no constituent values
constexpr A v3 = []{
  A a;
  std::start_lifetime(a.arr);   // OK, arr is now the active element of the union
  new (&a.arr[1]) int(1);
  a.arr[2] = 2;
  return a;
}();                    // OK, the constituent values are {v3.arr[1], v3.arr[2]}
```

1. `a.arr[2] = 2`: [obj.lifetime] `start_lifetime` says "No initialization is performed and no subobject has its
   lifetime started." By [class.union.general], S(`a.arr[2]`) is S(`a.arr`) = {`a.arr`} (the `A[B]` rule), `a.arr` is
   already within its lifetime, so no object is implicitly created, and `a.arr[2]` is modified outside its lifetime
   ([basic.life]). The paper's own prose (section 2, the `FixedVector` example) agrees with the wording: "no array element
   starts its lifetime yet", elements are constructed with placement new. The constituent values would then be
   {`v3.arr[1]`} (plus `v3.arr[2]` only if it was constructed).
2. `constexpr A v1;`: [dcl.init.general] const-default-constructible requires, for a class that is not a union, that
   the type of each anonymous union member is const-default-constructible; a union with at least one non-static data
   member needs exactly one variant member with a default member initializer. `A`'s anonymous union has none, so `A`
   is not const-default-constructible and the declaration is ill-formed.

## Suggested fix

Replace `a.arr[2] = 2;` by `new (&a.arr[2]) int(2);`. For `constexpr A v1;` the declaration needs another form or the
example line has to go (`A v1{}` aggregate-initializes the anonymous union with its first member, so it would have a
constituent value); the editors choose.

## Checked

- P3726R2 changed neither the S(E) rule nor the implicit-creation paragraph of [class.union.general] (`git show
  b621f1a42 -- source/classes.tex` only removes the P3074R7 trivial-default-constructor rule).
- Nothing after `b621f1a42` in cplusplus/draft changes either paragraph or the example; no draft issue or paper issue
  mentions `start_lifetime` (cplusplus/papers #2356 is the paper's own tracking issue).
- This fork follows the wording (classic and bytecode evaluators both reject line 1; Sema rejects `constexpr A v1;`).
