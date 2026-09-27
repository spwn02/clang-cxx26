//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include <__config>

#if _LIBCPP_HAS_THREADS

#  include <__execution/get_stop_token.h>
#  include <__execution/system_context_replaceability.h>
#  include <condition_variable>
#  include <stop_token>
#  include <cstddef>
#  include <memory>
#  include <mutex>
#  include <optional>
#  include <span>
#  include <thread>
#  include <typeinfo>
#  include <vector>

// See docs/design/parallel_scheduler_p2079.md, Pass 3b: this default backend is deliberately
// self-contained, with its OWN worker pool -- NOT a reuse of
// <__execution/parallel_scheduler.h>'s __get_parallel_pool() singleton, which is
// _LIBCPP_HIDE_FROM_ABI/hidden-visibility and therefore gets a SEPARATE per-DSO copy of its
// function-local static (by design, for header-only consumers); reusing it here, in code
// compiled directly into this shared library, would silently create a second, disjoint pool
// instance instead of sharing one. That would only matter for a caller that mixes
// get_parallel_scheduler() and a direct query_parallel_scheduler_backend()->schedule(...) call
// in the same unreplaced program -- a real but narrow QoI gap (two independent pools instead of
// one shared one), not a correctness bug: the actual normative requirement this Pass delivers,
// that REPLACING the backend is honored process-wide, does not depend on the default backend's
// own pool identity at all. Documented here rather than silently accepted.

_LIBCPP_BEGIN_NAMESPACE_STD

namespace execution {
namespace system_context_replaceability {

namespace {

// A minimal, self-contained worker pool, structurally identical to
// <__execution/parallel_scheduler.h>'s __parallel_pool but scoped privately to this backend
// (see the file comment above for why this isn't shared with that one).
class __backend_pool {
public:
  explicit __backend_pool(size_t __n) {
    __workers_.reserve(__n);
    for (size_t __i = 0; __i < __n; ++__i) {
      __workers_.emplace_back([this] { __worker_loop(); });
    }
    for (auto& __t : __workers_) {
      __t.detach();
    }
  }

  __backend_pool(const __backend_pool&)            = delete;
  __backend_pool& operator=(const __backend_pool&) = delete;

  void __enqueue(void (*__fn)(void*), void* __ctx) {
    {
      lock_guard<mutex> __lock(__mtx_);
      __queue_.push_back({__fn, __ctx});
    }
    __cv_.notify_one();
  }

private:
  struct __item {
    void (*__fn)(void*);
    void* __ctx;
  };

  void __worker_loop() {
    while (true) {
      __item __it;
      {
        unique_lock<mutex> __lock(__mtx_);
        __cv_.wait(__lock, [this] { return !__queue_.empty(); });
        __it = __queue_.front();
        __queue_.erase(__queue_.begin());
      }
      __it.__fn(__it.__ctx);
    }
  }

  mutex __mtx_;
  condition_variable __cv_;
  vector<__item> __queue_;
  vector<thread> __workers_;
};

__backend_pool& __get_backend_pool() {
  static __backend_pool* __pool =
      new __backend_pool(std::thread::hardware_concurrency() == 0 ? 1 : static_cast<size_t>(std::thread::hardware_concurrency()));
  return *__pool;
}

// [exec.sysctx.replaceability]: try_query<inplace_stop_token, get_stop_token_t> is the paper's
// one mandated case; ask the proxy generically and fall back to never_stop_token semantics
// (report "not stopped") if the frontend's receiver doesn't answer with exactly that type.
bool __proxy_stop_requested(receiver_proxy& __proxy) {
  optional<inplace_stop_token> __tok = __proxy.try_query<inplace_stop_token>(get_stop_token_t{});
  return __tok.has_value() && __tok->stop_requested();
}

struct __schedule_ctx {
  receiver_proxy* __proxy;
};

void __run_schedule(void* __raw) {
  auto* __ctx = static_cast<__schedule_ctx*>(__raw);
  if (__proxy_stop_requested(*__ctx->__proxy)) {
    __ctx->__proxy->set_stopped();
  } else {
    __ctx->__proxy->set_value();
  }
}

struct __bulk_ctx {
  bulk_item_receiver_proxy* __proxy;
  size_t __begin;
  size_t __end;
  shared_ptr<atomic<size_t>> __remaining; // completes the proxy once every chunk finishes
};

void __run_bulk_chunk(void* __raw) {
  auto* __ctx = static_cast<__bulk_ctx*>(__raw);
  bulk_item_receiver_proxy& __proxy = *__ctx->__proxy;
  if (!__proxy_stop_requested(__proxy)) {
    __proxy.execute(__ctx->__begin, __ctx->__end);
  }
  if (__ctx->__remaining->fetch_sub(1, memory_order_acq_rel) == 1) {
    if (__proxy_stop_requested(__proxy)) {
      __proxy.set_stopped();
    } else {
      __proxy.set_value();
    }
  }
  delete __ctx;
}

class __default_parallel_scheduler_backend final : public parallel_scheduler_backend {
public:
  void schedule(receiver_proxy& __proxy, span<byte> __storage) noexcept override {
    static_assert(sizeof(__schedule_ctx) <= 64, "must fit the frontend's guaranteed scratch size");
    auto* __ctx = ::new (static_cast<void*>(__storage.data())) __schedule_ctx{&__proxy};
    __get_backend_pool().__enqueue(&__run_schedule, __ctx);
  }

  // A minimal but correct bulk implementation: this backend does not need the frontend's
  // scratch span (each chunk's context is heap-allocated, freed by __run_bulk_chunk once that
  // chunk completes) -- schedule_bulk_chunked and schedule_bulk_unchunked only differ in
  // whether the backend or the caller owns the per-index split; both invoke execute() once
  // per contiguous [begin, end) range covering all of [0, count) exactly once, with no overlap.
  void schedule_bulk_chunked(size_t __count, bulk_item_receiver_proxy& __proxy, span<byte>) noexcept override {
    __dispatch_bulk(__count, __proxy, /*__one_index_per_chunk=*/false);
  }

  void schedule_bulk_unchunked(size_t __count, bulk_item_receiver_proxy& __proxy, span<byte>) noexcept override {
    __dispatch_bulk(__count, __proxy, /*__one_index_per_chunk=*/true);
  }

private:
  void __dispatch_bulk(size_t __count, bulk_item_receiver_proxy& __proxy, bool __one_index_per_chunk) {
    if (__count == 0) {
      // Nothing to run; complete immediately, matching a zero-size shape's vacuous truth.
      if (__proxy_stop_requested(__proxy)) {
        __proxy.set_stopped();
      } else {
        __proxy.set_value();
      }
      return;
    }
    size_t __num_chunks = __one_index_per_chunk ? __count : std::min<size_t>(__count, std::thread::hardware_concurrency() == 0
                                                                                            ? 1
                                                                                            : std::thread::hardware_concurrency());
    auto __remaining = std::make_shared<atomic<size_t>>(__num_chunks);
    size_t __base    = __count / __num_chunks;
    size_t __extra   = __count % __num_chunks;
    size_t __begin   = 0;
    for (size_t __i = 0; __i < __num_chunks; ++__i) {
      size_t __len = __base + (__i < __extra ? 1 : 0);
      size_t __end = __begin + __len;
      auto* __ctx  = new __bulk_ctx{&__proxy, __begin, __end, __remaining};
      __get_backend_pool().__enqueue(&__run_bulk_chunk, __ctx);
      __begin = __end;
    }
  }
};

} // namespace

shared_ptr<parallel_scheduler_backend> __get_default_parallel_scheduler_backend() {
  static shared_ptr<parallel_scheduler_backend> __backend = std::make_shared<__default_parallel_scheduler_backend>();
  return __backend;
}

_LIBCPP_WEAK shared_ptr<parallel_scheduler_backend> query_parallel_scheduler_backend() {
  return __get_default_parallel_scheduler_backend();
}

} // namespace system_context_replaceability
} // namespace execution

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_THREADS
