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

  // [exec.set.value], [exec.set.error], [exec.set.stopped]
  using std::execution::set_error;
  using std::execution::set_error_t;
  using std::execution::set_stopped;
  using std::execution::set_stopped_t;
  using std::execution::set_value;
  using std::execution::set_value_t;

  // [exec.cmplsig]
  using std::execution::completion_signatures;

  // [exec.recv.concepts]
  using std::execution::receiver;
  using std::execution::receiver_of;
  using std::execution::receiver_tag;

  // [exec.opstate.general], [exec.opstate.start]
  using std::execution::operation_state;
  using std::execution::operation_state_tag;
  using std::execution::start;
  using std::execution::start_t;

  // [exec.snd.concepts]
  using std::execution::enable_sender;
  using std::execution::sender;
  using std::execution::sender_tag;
  using std::execution::tag_of_t;

  // [exec.getcomplsigs]
  using std::execution::completion_signatures_of_t;
  using std::execution::get_completion_signatures;
  using std::execution::sender_in;

  // [exec.cmplsig]: value_types_of_t/error_types_of_t/sends_stopped
  using std::execution::error_types_of_t;
  using std::execution::sends_stopped;
  using std::execution::value_types_of_t;

  // [exec.domain.default], [exec.get.domain]
  using std::execution::default_domain;
  using std::execution::get_domain;
  using std::execution::get_domain_t;

  // [exec.snd.transform], [exec.snd.apply]
  using std::execution::apply_sender;
  using std::execution::transform_sender;

  // [exec.connect]
  using std::execution::connect;
  using std::execution::connect_result_t;
  using std::execution::connect_t;

  // [exec.async.ops]
  using std::execution::dependent_sender_error;

  // [exec.get.fwd.progress]
  using std::execution::forward_progress_guarantee;
  using std::execution::get_forward_progress_guarantee;
  using std::execution::get_forward_progress_guarantee_t;

  // [exec.sched]
  using std::execution::scheduler;
  using std::execution::scheduler_tag;

  // [exec.schedule]
  using std::execution::schedule;
  using std::execution::schedule_t;
  using std::execution::schedule_result_t;

  // [exec.just]
  using std::execution::just;
  using std::execution::just_error;
  using std::execution::just_error_t;
  using std::execution::just_stopped;
  using std::execution::just_stopped_t;
  using std::execution::just_t;

  // [exec.read.env]
  // read_env's type is unspecified per [execution.syn]; only the CPO object itself is exported.
  using std::execution::read_env;

  // [exec.get.scheduler], [exec.get.start.scheduler], [exec.get.delegation.scheduler],
  // [exec.get.compl.sched]
  using std::execution::get_completion_scheduler;
  using std::execution::get_completion_scheduler_t;
  using std::execution::get_delegation_scheduler;
  using std::execution::get_delegation_scheduler_t;
  using std::execution::get_scheduler;
  using std::execution::get_scheduler_t;
  using std::execution::get_start_scheduler;
  using std::execution::get_start_scheduler_t;

  // [exec.run.loop]
#if _LIBCPP_HAS_THREADS
  using std::execution::run_loop;
#endif

  // [exec.adapt.obj]
  using std::execution::operator|;
  using std::execution::sender_adaptor_closure;

  // [exec.then]
  using std::execution::then;
  using std::execution::then_t;
} // namespace std::execution

#if _LIBCPP_HAS_THREADS
export namespace std::this_thread {
  // [exec.sync.wait]
  using std::this_thread::sync_wait;
} // namespace std::this_thread
#endif // _LIBCPP_HAS_THREADS
#endif // _LIBCPP_STD_VER >= 26
