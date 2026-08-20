// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifdef _LIBCPP_ENABLE_EXPERIMENTAL
export namespace std {
  // [execpol.type], execution policy type trait
  using std::is_execution_policy;
  using std::is_execution_policy_v;
} // namespace std

export namespace std::execution {
  // [execpol.seq], sequenced execution policy
  using std::execution::sequenced_policy;

  // [execpol.par], parallel execution policy
  using std::execution::parallel_policy;

  // [execpol.parunseq], parallel and unsequenced execution policy
  using std::execution::parallel_unsequenced_policy;

  // [execpol.unseq], unsequenced execution policy
  using std::execution::unsequenced_policy;

  // [execpol.objects], execution policy objects
  using std::execution::par;
  using std::execution::par_unseq;
  using std::execution::seq;
  using std::execution::unseq;
} // namespace std::execution
#endif // _LIBCPP_ENABLE_EXPERIMENTAL

#if _LIBCPP_STD_VER >= 26
export namespace std {
  // [exec.queryable]
  // `queryable` is exposition-only, not exported.

  // [exec.fwd.env]
  using std::forwarding_query_t;
  using std::forwarding_query;

  // [exec.get.allocator]
  using std::get_allocator_t;
  using std::get_allocator;

  // [exec.get.stop.token]
#if _LIBCPP_HAS_THREADS
  using std::get_stop_token_t;
  using std::get_stop_token;
  using std::stop_token_of_t;
#endif
} // namespace std

export namespace std::execution {
  // [exec.prop], [exec.env]
  using std::execution::env;
  using std::execution::prop;

  // [exec.get.env]
  using std::execution::env_of_t;
  using std::execution::get_env_t;
  using std::execution::get_env;
} // namespace std::execution
#endif // _LIBCPP_STD_VER >= 26
