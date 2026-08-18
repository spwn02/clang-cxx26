// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

export namespace std {
  using std::hive_limits;

  // [hive], class template hive
  using std::hive;

  using std::swap;

  // erasure
  using std::erase;
  using std::erase_if;

  namespace pmr {
    using std::pmr::hive;
  }
} // namespace std
