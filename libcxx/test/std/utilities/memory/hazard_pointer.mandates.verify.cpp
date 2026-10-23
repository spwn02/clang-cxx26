//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: no-threads

// <hazard_pointer>

// [saferecl.hp.holder.mem], [saferecl.hp.base]:
//   hazard_pointer::try_protect, hazard_pointer::reset_protection(const T*) and
//   hazard_pointer_obj_base<T, D>::retire: Mandates: T is a hazard-protectable type
//   (exactly one public non-virtual base hazard_pointer_obj_base<T, D>).

#include <atomic>
#include <hazard_pointer>

// expected-note@*:* 0+ {{in instantiation of}}

struct Good : std::hazard_pointer_obj_base<Good> {};
struct WithDeleter : std::hazard_pointer_obj_base<WithDeleter, void (*)(WithDeleter*)> {};
struct PrivateBase : private std::hazard_pointer_obj_base<PrivateBase> {};
struct TwoBases : std::hazard_pointer_obj_base<TwoBases>,
                  std::hazard_pointer_obj_base<TwoBases, void (*)(TwoBases*)> {};
struct Other;
struct DifferentBaseTypes : std::hazard_pointer_obj_base<DifferentBaseTypes>, std::hazard_pointer_obj_base<Other> {};
struct Unrelated {};

void test() {
  std::hazard_pointer h = std::make_hazard_pointer();

  Good g;
  Good* gp = &g;
  std::atomic<Good*> gsrc{gp};
  h.reset_protection(gp);
  (void)h.try_protect(gp, gsrc);
  (void)h.protect(gsrc);

  WithDeleter w;
  WithDeleter* wp = &w;
  std::atomic<WithDeleter*> wsrc{wp};
  (void)h.try_protect(wp, wsrc);

  int value = 1;
  int* ip   = &value;
  std::atomic<int*> isrc{ip};
  h.reset_protection(ip);        // expected-error@*:* {{T must be a hazard-protectable type}}
  (void)h.try_protect(ip, isrc); // expected-error@*:* {{T must be a hazard-protectable type}}

  Unrelated u;
  Unrelated* up = &u;
  h.reset_protection(up); // expected-error@*:* {{T must be a hazard-protectable type}}

  PrivateBase pb;
  PrivateBase* pbp = &pb;
  h.reset_protection(pbp); // expected-error@*:* {{T must be a hazard-protectable type}}

  TwoBases tb;
  TwoBases* tbp = &tb;
  h.reset_protection(tbp); // expected-error@*:* {{T must be a hazard-protectable type}}

  DifferentBaseTypes db;
  DifferentBaseTypes* dbp = &db;
  h.reset_protection(dbp); // expected-error@*:* {{T must be a hazard-protectable type}}
  std::atomic<DifferentBaseTypes*> dbsrc{dbp};
  (void)h.try_protect(dbp, dbsrc); // expected-error@*:* {{T must be a hazard-protectable type}}
  static_cast<std::hazard_pointer_obj_base<DifferentBaseTypes>&>(db)
      .retire(); // expected-error@*:* {{T must be a hazard-protectable type}}
}
