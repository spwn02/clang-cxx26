//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_SYSTEM_CONTEXT_REPLACEABILITY_H
#define _LIBCPP___EXECUTION_SYSTEM_CONTEXT_REPLACEABILITY_H

#include <__config>
#include <__exception/exception_ptr.h>
#include <__memory/shared_ptr.h>
#include <__type_traits/decay.h>
#include <__utility/move.h>
#include <cstddef>
#include <new>
#include <optional>
#include <span>
#include <typeinfo>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

namespace execution {
namespace system_context_replaceability {

// See docs/design/parallel_scheduler_p2079.md, Pass 3b, for the full design rationale,
// including a documented discrepancy between this fork's naming and the published paper's own
// (self-inconsistent) synopsis.
//
// [exec.sysctx.replaceability]: receiver_proxy lets a REPLACED backend complete or query a
// type-erased frontend receiver without knowing its concrete type. set_value/set_error/
// set_stopped are called BY the backend, on a thread belonging to the backend's own execution
// context, exactly once, to signal the outcome of a scheduled operation.
class receiver_proxy {
public:
  // A user-declared destructor (even defaulted) suppresses the implicit copy constructor and
  // deprecates falling back to it (-Wdeprecated-copy-with-dtor, a hard error under this
  // codebase's -Werror -- caught via a real regression in __parallel_opstate's own defaulted
  // move constructor, which multiply-inherits this class). Declared explicitly instead,
  // matching <__execution/parallel_scheduler.h>'s own __parallel_task_base precedent.
  _LIBCPP_HIDE_FROM_ABI receiver_proxy() noexcept                       = default;
  _LIBCPP_HIDE_FROM_ABI receiver_proxy(receiver_proxy&&) noexcept       = default;
  receiver_proxy(const receiver_proxy&)                                 = delete;
  receiver_proxy& operator=(const receiver_proxy&)                      = delete;
  receiver_proxy& operator=(receiver_proxy&&)                           = delete;
  _LIBCPP_HIDE_FROM_ABI virtual ~receiver_proxy() = default;

  virtual void set_value() noexcept                 = 0;
  virtual void set_error(std::exception_ptr) noexcept = 0;
  virtual void set_stopped() noexcept               = 0;

  // The paper leaves try_query's underlying dispatch exposition-only ("_query-env_"); this is
  // this fork's concrete shape for it. Only one case is paper-mandated:
  // try_query<inplace_stop_token, get_stop_token_t>(get_stop_token_t{}) must return a populated
  // optional when the real receiver's environment answers get_stop_token with an
  // inplace_stop_token -- see the frontend's __receiver_proxy_impl in
  // <__execution/system_context_default_backend.h> for the one case this fork implements.
  template <class _ResultTp, class _Query>
  _LIBCPP_HIDE_FROM_ABI optional<_ResultTp> try_query(_Query __q) noexcept {
    alignas(_ResultTp) unsigned char __storage[sizeof(_ResultTp)];
    if (__query_env(typeid(_Query), typeid(_ResultTp), std::addressof(__q), static_cast<void*>(__storage))) {
      _ResultTp* __p = std::launder(reinterpret_cast<_ResultTp*>(__storage));
      optional<_ResultTp> __result(std::in_place, std::move(*__p));
      __p->~_ResultTp();
      return __result;
    }
    return nullopt;
  }

protected:
  // Exposition-only "_query-env_" in the paper. __query is a pointer to a live _Query object
  // (as passed to try_query); on a match, placement-construct a _ResultTp into __result_storage and return true.
  _LIBCPP_HIDE_FROM_ABI virtual bool
  __query_env(const type_info& __query_type, const type_info& __result_type, const void* __query, void* __result_storage) noexcept = 0;
};

// [exec.sysctx.replaceability]: bulk_item_receiver_proxy additionally lets the backend invoke
// one bulk work-item range at a time, [begin, end), exactly once per range, covering the whole
// requested [0, count) with no overlaps and no gaps across all execute() calls for one
// scheduling operation.
class bulk_item_receiver_proxy : public receiver_proxy {
public:
  virtual void execute(size_t __begin, size_t __end) noexcept = 0;
};

// [exec.sysctx.replaceability]: parallel_scheduler_backend is the ABI a replacement backend
// implements. Every member is passed a span<byte> the backend may use as scratch storage for
// the scheduling operation (e.g. an intrusive task-list node) to avoid a heap allocation
// crossing the ABI boundary; the span's lifetime is at least as long as the operation itself.
class parallel_scheduler_backend {
public:
  // See receiver_proxy's identical special-member declarations above for why these are
  // explicit rather than left to default alongside the user-declared destructor.
  _LIBCPP_HIDE_FROM_ABI parallel_scheduler_backend() noexcept                            = default;
  parallel_scheduler_backend(const parallel_scheduler_backend&)                          = delete;
  parallel_scheduler_backend(parallel_scheduler_backend&&)                               = delete;
  parallel_scheduler_backend& operator=(const parallel_scheduler_backend&)               = delete;
  parallel_scheduler_backend& operator=(parallel_scheduler_backend&&)                    = delete;
  _LIBCPP_HIDE_FROM_ABI virtual ~parallel_scheduler_backend() = default;

  virtual void schedule(receiver_proxy&, span<byte>) noexcept                                     = 0;
  virtual void schedule_bulk_chunked(size_t, bulk_item_receiver_proxy&, span<byte>) noexcept   = 0;
  virtual void schedule_bulk_unchunked(size_t, bulk_item_receiver_proxy&, span<byte>) noexcept = 0;
};

// [exec.sysctx.replaceability]: called by the frontend every time a parallel_scheduler needs
// its active backend. Weak, link-time replaceable -- a program may define a strong, non-weak
// definition of this exact symbol to install its own backend for the whole program, the same
// mechanism this fork already uses for replaceable operator new/delete
// (libcxx/src/new.cpp). The default definition lives in
// libcxx/src/system_context_replaceability.cpp, backed by this fork's Pass-1 __parallel_pool.
//
// IMPORTANT for anyone replacing this symbol: unlike operator new/delete (declared at global
// scope), this function lives inside std::execution::system_context_replaceability, which is
// itself nested inside libc++'s own inline ABI-versioned namespace (std::__1). A naive
// `namespace std::execution::system_context_replaceability { ... }` reopen block does NOT
// resolve through that inline namespace -- it silently creates an unrelated namespace with a
// DIFFERENT mangled name, and the "replacement" is silently never called (confirmed directly:
// this exact mistake was caught while testing this Pass). Define the override with a
// QUALIFIED-ID function definition instead, which correctly resolves via ordinary qualified
// lookup to the existing (versioned) declaration:
//   namespace scr = std::execution::system_context_replaceability;
//   std::shared_ptr<scr::parallel_scheduler_backend> scr::query_parallel_scheduler_backend() {
//     ...
//   }
_LIBCPP_OVERRIDABLE_FUNC_VIS shared_ptr<parallel_scheduler_backend> query_parallel_scheduler_backend();

// Internal use only: always the real default backend singleton, bypassing any program-level
// replacement of query_parallel_scheduler_backend() above. Lets the frontend
// (<__execution/parallel_scheduler.h>) detect whether the active backend has actually been
// replaced, so it can keep its existing zero-overhead direct-pool dispatch when it hasn't.
_LIBCPP_EXPORTED_FROM_ABI shared_ptr<parallel_scheduler_backend> __get_default_parallel_scheduler_backend();

} // namespace system_context_replaceability
} // namespace execution

#endif // _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___EXECUTION_SYSTEM_CONTEXT_REPLACEABILITY_H
