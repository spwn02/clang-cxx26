//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___NET_RESOLVER_H
#define _LIBCPP___NET_RESOLVER_H

#include <__config>
#include <__coroutine/coroutine_handle.h>
#include <__cstddef/size_t.h>
#include <__io/execution_context.h>
#include <__io/executor.h>
#include <__io/io_env.h>
#include <__io/support/config.h>
#include <__io_context/blocking_service.h>
#include <__io_stream/error.h>
#include <__memory/addressof.h>
#include <__memory/shared_ptr.h>
#include <__net/endpoint.h>
#include <__net/resolver_error.h>
#include <__net/support/posix_resolve.h>
#include <__system_error/errc.h>
#include <__system_error/error_code.h>
#include <__type_traits/is_same.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/move.h>
#include <__vector/vector.h>
#include <string>
#include <string_view>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_SOCKETS

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// P4100R1 [8.9]; provisional specification: Corosio's resolve_flags and reverse_flags (the hints of getaddrinfo and the
// flags of getnameinfo, with portable values).
enum class resolve_flags : unsigned {
  none               = 0,
  passive            = 0x01,
  numeric_host       = 0x04,
  numeric_service    = 0x08,
  address_configured = 0x20,
  v4_mapped          = 0x800,
  all_matching       = 0x100
};

enum class reverse_flags : unsigned {
  none             = 0,
  numeric_host     = 0x01,
  numeric_service  = 0x02,
  name_required    = 0x04,
  datagram_service = 0x08
};

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr resolve_flags operator|(resolve_flags __a, resolve_flags __b) noexcept {
  return static_cast<resolve_flags>(static_cast<unsigned>(__a) | static_cast<unsigned>(__b));
}
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr resolve_flags operator&(resolve_flags __a, resolve_flags __b) noexcept {
  return static_cast<resolve_flags>(static_cast<unsigned>(__a) & static_cast<unsigned>(__b));
}
_LIBCPP_HIDE_FROM_ABI constexpr resolve_flags& operator|=(resolve_flags& __a, resolve_flags __b) noexcept {
  return __a = __a | __b;
}
_LIBCPP_HIDE_FROM_ABI constexpr resolve_flags& operator&=(resolve_flags& __a, resolve_flags __b) noexcept {
  return __a = __a & __b;
}
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr reverse_flags operator|(reverse_flags __a, reverse_flags __b) noexcept {
  return static_cast<reverse_flags>(static_cast<unsigned>(__a) | static_cast<unsigned>(__b));
}
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr reverse_flags operator&(reverse_flags __a, reverse_flags __b) noexcept {
  return static_cast<reverse_flags>(static_cast<unsigned>(__a) & static_cast<unsigned>(__b));
}
_LIBCPP_HIDE_FROM_ABI constexpr reverse_flags& operator|=(reverse_flags& __a, reverse_flags __b) noexcept {
  return __a = __a | __b;
}
_LIBCPP_HIDE_FROM_ABI constexpr reverse_flags& operator&=(reverse_flags& __a, reverse_flags __b) noexcept {
  return __a = __a & __b;
}

// One result of a forward resolution: an endpoint, and the host name and service name that were looked up (the
// arguments of resolve(), not a canonical name). Deviation: Corosio's resolve() completes with plain endpoints; the
// entry converts to its endpoint, so a resolver_results can be passed to connect() like a vector of endpoints.
class resolver_entry {
  io::endpoint __ep_;
  string __host_;
  string __service_;

public:
  _LIBCPP_HIDE_FROM_ABI resolver_entry() = default;
  _LIBCPP_HIDE_FROM_ABI resolver_entry(const io::endpoint& __e, string __h, string __s)
      : __ep_(__e), __host_(std::move(__h)), __service_(std::move(__s)) {}

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI io::endpoint endpoint() const noexcept { return __ep_; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI string_view host_name() const noexcept { return __host_; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI string_view service_name() const noexcept { return __service_; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI operator io::endpoint() const noexcept { return __ep_; }
};

// The result of a forward resolution: a sequence of entries in the order of the system (at least one on success).
class resolver_results {
  vector<resolver_entry> __v_;

public:
  using value_type      = resolver_entry;
  using const_reference = const resolver_entry&;
  using reference       = const_reference;
  using const_iterator  = vector<resolver_entry>::const_iterator;
  using iterator        = const_iterator;
  using size_type       = size_t;

  _LIBCPP_HIDE_FROM_ABI resolver_results() = default;
  _LIBCPP_HIDE_FROM_ABI explicit resolver_results(vector<resolver_entry> __v) : __v_(std::move(__v)) {}

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI const_iterator begin() const noexcept { return __v_.begin(); }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI const_iterator end() const noexcept { return __v_.end(); }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI const_iterator cbegin() const noexcept { return __v_.begin(); }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI const_iterator cend() const noexcept { return __v_.end(); }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI size_t size() const noexcept { return __v_.size(); }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool empty() const noexcept { return __v_.empty(); }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI const resolver_entry& operator[](size_t __i) const noexcept { return __v_[__i]; }
};

// The result of a reverse resolution: the host name and the service name of an endpoint.
class reverse_resolver_result {
  string __host_;
  string __service_;

public:
  _LIBCPP_HIDE_FROM_ABI reverse_resolver_result() = default;
  _LIBCPP_HIDE_FROM_ABI reverse_resolver_result(string __h, string __s)
      : __host_(std::move(__h)), __service_(std::move(__s)) {}
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI const string& host_name() const noexcept { return __host_; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI const string& service_name() const noexcept { return __service_; }
};

// The name of the local machine (gethostname). Synchronous; needs no context.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline io_result<string> host_name() {
  string __n;
  if (error_code __ec = io::__sys_host_name(__n))
    return io_result<string>(__ec, string());
  return io_result<string>(error_code(), std::move(__n));
}

class resolver;

// A request: its input and output, shared between the awaiting coroutine and the helper thread that runs it (through
// the shared data of a detachable __blocking_op), so that a request that was cancelled (the coroutine has moved on) can
// still finish on its thread and be discarded.
struct __resolve_job {
  bool __reverse_ = false;
  string __host_;
  string __service_;
  unsigned __flags_ = 0;
  io::endpoint __ep_;
  // The result, written by the helper thread.
  error_code __ec_;
  vector<io::endpoint> __eps_;
  string __rhost_;
  string __rservice_;
};

// The call of the helper thread: getaddrinfo or getnameinfo, which block.
_LIBCPP_HIDE_FROM_ABI inline void __run_resolve_job(void* __p) noexcept {
  __resolve_job& __j = *static_cast<__resolve_job*>(__p);
#  if _LIBCPP_HAS_EXCEPTIONS
  try {
#  endif
    if (__j.__reverse_)
      __j.__ec_ = io::__sys_reverse_resolve(__j.__ep_, __j.__flags_, __j.__rhost_, __j.__rservice_);
    else
      __j.__ec_ = io::__sys_resolve(__j.__host_, __j.__service_, __j.__flags_, __j.__eps_);
#  if _LIBCPP_HAS_EXCEPTIONS
  } catch (...) {
    __j.__ec_ = make_error_code(errc::not_enough_memory);
  }
#  endif
}

// An operation of a resolver: the base of the two awaitables. It lives in the frame of the awaiting coroutine. The
// lookups run on the helper threads of the __blocking_service of the context (shared with the file operations, up to
// four threads, started lazily) as detachable operations: a lookup that is cancelled completes at once with
// error::canceled, even while its call is in progress (the result of that call is discarded).
class __resolve_op : public __blocking_op {
protected:
  shared_ptr<__resolve_job> __job_;

  _LIBCPP_HIDE_FROM_ABI
  __resolve_op(__blocking_service* __s, const resolver* __o, shared_ptr<__resolve_job> __j) noexcept
      : __blocking_op(__s, __o), __job_(std::move(__j)) {
    __detached_work_ = &__run_resolve_job;
    __data_          = __job_;
  }
  _LIBCPP_HIDE_FROM_ABI __resolve_op(__resolve_op&&) noexcept = default;
};

// P4100R1 [8.9]; provisional specification: Corosio's resolver. Translates host names and service names to endpoints
// and endpoints to names (getaddrinfo, getnameinfo), from a coroutine. The calls block, so they run on helper threads
// of the context (see __blocking_service); the awaiting coroutine resumes on its own executor. The operations are
// IoAwaitables and are cancelled through the stop token of the awaiting coroutine or through cancel(): they complete at
// once with error::canceled (the result of a lookup that is already running is discarded). Failures of the name service
// are resolver_error codes (see <net>), the errno value for a system failure.
//
// The execution_context must be an io_context (the completions go through its reactor). A resolver must not have
// concurrent operations (the same rule as Corosio); distinct resolvers can be used concurrently. The resolver must
// outlive its operations; moving it requires that none is pending.
class resolver {
  __blocking_service* __svc_;

  class __forward_awaitable : public __resolve_op {
  public:
    _LIBCPP_HIDE_FROM_ABI __forward_awaitable(const resolver& __r, shared_ptr<__resolve_job> __j) noexcept
        : __resolve_op(__r.__svc_, &__r, std::move(__j)) {}
    _LIBCPP_HIDE_FROM_ABI __forward_awaitable(__forward_awaitable&&) noexcept = default;

    _LIBCPP_HIDE_FROM_ABI io_result<resolver_results> await_resume() {
      __finish();
      error_code __ec = __ec_ ? __ec_ : __job_->__ec_;
      if (__ec)
        return io_result<resolver_results>(__ec, resolver_results());
      vector<resolver_entry> __v;
      __v.reserve(__job_->__eps_.size());
      for (const io::endpoint& __e : __job_->__eps_)
        __v.emplace_back(__e, __job_->__host_, __job_->__service_);
      if (__v.empty())
        return io_result<resolver_results>(make_error_code(errc::no_such_device_or_address), resolver_results());
      return io_result<resolver_results>(error_code(), resolver_results(std::move(__v)));
    }
  };

  class __reverse_awaitable : public __resolve_op {
  public:
    _LIBCPP_HIDE_FROM_ABI __reverse_awaitable(const resolver& __r, shared_ptr<__resolve_job> __j) noexcept
        : __resolve_op(__r.__svc_, &__r, std::move(__j)) {}
    _LIBCPP_HIDE_FROM_ABI __reverse_awaitable(__reverse_awaitable&&) noexcept = default;

    _LIBCPP_HIDE_FROM_ABI io_result<reverse_resolver_result> await_resume() {
      __finish();
      error_code __ec = __ec_ ? __ec_ : __job_->__ec_;
      if (__ec)
        return io_result<reverse_resolver_result>(__ec, reverse_resolver_result());
      return io_result<reverse_resolver_result>(
          error_code(), reverse_resolver_result(std::move(__job_->__rhost_), std::move(__job_->__rservice_)));
    }
  };

public:
  _LIBCPP_HIDE_FROM_ABI explicit resolver(execution_context& __ctx)
      : __svc_(&__ctx.use_service<__blocking_service>()) {}
  template <Executor _Ex>
    requires(!is_same_v<remove_cvref_t<_Ex>, resolver>)
  _LIBCPP_HIDE_FROM_ABI explicit resolver(const _Ex& __ex) : resolver(__ex.context()) {}

  resolver(const resolver&)            = delete;
  resolver& operator=(const resolver&) = delete;

  // Precondition: no operation is pending on __o.
  _LIBCPP_HIDE_FROM_ABI resolver(resolver&& __o) noexcept : __svc_(__o.__svc_) {}
  // Cancels the operations of *this; precondition: no operation is pending on __o.
  _LIBCPP_HIDE_FROM_ABI resolver& operator=(resolver&& __o) noexcept {
    if (this != std::addressof(__o)) {
      cancel();
      __svc_ = __o.__svc_;
    }
    return *this;
  }
  _LIBCPP_HIDE_FROM_ABI ~resolver() { cancel(); }

  // Looks up the host and the service (a name or a decimal port number; either may be empty, not both) for a stream
  // socket; completes with the endpoints in the order of the system. Failure: resolver_error (host_not_found if the
  // host or the service is unknown) and an empty result.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI __forward_awaitable
  resolve(string_view __host, string_view __service, resolve_flags __flags = resolve_flags::none) {
    auto __j        = std::make_shared<__resolve_job>();
    __j->__host_    = string(__host);
    __j->__service_ = string(__service);
    __j->__flags_   = static_cast<unsigned>(__flags);
    return __forward_awaitable(*this, std::move(__j));
  }

  // Looks up the names of the endpoint: the host name (the numeric address with reverse_flags::numeric_host or if no
  // name is found) and the service name.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI __reverse_awaitable
  resolve(const io::endpoint& __ep, reverse_flags __flags = reverse_flags::none) {
    auto __j        = std::make_shared<__resolve_job>();
    __j->__reverse_ = true;
    __j->__ep_      = __ep;
    __j->__flags_   = static_cast<unsigned>(__flags);
    return __reverse_awaitable(*this, std::move(__j));
  }

  // Cancels the pending operations of the resolver; each completes with error::canceled.
  _LIBCPP_HIDE_FROM_ABI void cancel() noexcept { __svc_->__cancel_owner(this); }
};

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_IO_SOCKETS

#endif // _LIBCPP___NET_RESOLVER_H
