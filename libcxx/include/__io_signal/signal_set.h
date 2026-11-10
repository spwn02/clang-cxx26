//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_SIGNAL_SIGNAL_SET_H
#define _LIBCPP___IO_SIGNAL_SIGNAL_SET_H

#include <__config>
#include <__coroutine/coroutine_handle.h>
#include <__cstddef/size_t.h>
#include <__io/execution_context.h>
#include <__io/executor.h>
#include <__io/io_env.h>
#include <__io/support/config.h>
#include <__mutex/lock_guard.h>
#include <__mutex/mutex.h>
#include <__system_error/errc.h>
#include <__system_error/error_code.h>
#include <__system_error/throw_system_error.h>
#include <__type_traits/is_convertible.h>
#include <__type_traits/is_same.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/move.h>
#include <__vector/vector.h>

#if _LIBCPP_HAS_IO_POSIX_SIGNALS
#  include <__io/support/posix_signal.h>
#  include <__io_context/op.h>
#  include <__io_context/reactor.h>
#  include <__io_stream/error.h>
#endif

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_POSIX_SIGNALS

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

class signal_set;
class __signal_service;

// A wait for a signal. The result is the signal number.
struct __signal_op : __op_base {
  __signal_service* __svc_;
  signal_set* __set_;
  int __signo_ = 0;

  _LIBCPP_HIDE_FROM_ABI __signal_op(__signal_service* __s, signal_set* __set) noexcept : __svc_(__s), __set_(__set) {
    __cancel_ = &__cancel_cb;
  }
  _LIBCPP_HIDE_FROM_ABI __signal_op(__signal_op&& __o) noexcept
      : __op_base(std::move(__o)), __svc_(__o.__svc_), __set_(__o.__set_) {
    __cancel_ = &__cancel_cb;
  }
  _LIBCPP_HIDE_FROM_ABI static void __cancel_cb(__op_base*) noexcept;
};

// The process-wide part of signal handling. Signal dispositions belong to the process, not to a context: the registry
// counts, per signal, how many signal sets (of any context) have the signal registered, remembers the flags it was
// installed with, and lists the services of all contexts so that a delivered signal reaches every set that registered it.
// Lock order: registry mutex, then the mutex of a service.
struct __signal_registry {
  mutex __m_;
  size_t __count_[__posix_max_signal]  = {};
  unsigned __flags_[__posix_max_signal] = {};
  bool __fixed_[__posix_max_signal]     = {}; // __flags_ was chosen by a registration that did not say dont_care
  vector<__signal_service*> __services_;
};

// Never destroyed: a signal can arrive and be drained while static objects are destroyed.
_LIBCPP_HIDE_FROM_ABI inline __signal_registry& __get_signal_registry() {
  static __signal_registry* __r = new __signal_registry;
  return *__r;
}

// Per-context: the signal sets of the context and their waits. It uses the reactor to watch the signal pipe.
class __signal_service final : public execution_context::service {
  friend class signal_set;
  friend struct __signal_op;

  __reactor_service& __reactor_;
  mutex __m_;
  vector<signal_set*> __sets_;
  bool __shut_down_    = false;
  bool __source_added_ = false;
  int __pipe_fd_       = -1;

public:
  _LIBCPP_HIDE_FROM_ABI explicit __signal_service(execution_context& __ctx)
      : __reactor_(__ctx.use_service<__reactor_service>()) {
    __signal_registry& __r = __get_signal_registry();
    lock_guard<mutex> __lk(__r.__m_);
    __r.__services_.push_back(this);
  }

  _LIBCPP_HIDE_FROM_ABI ~__signal_service() override {
    {
      __signal_registry& __r = __get_signal_registry();
      lock_guard<mutex> __lk(__r.__m_);
      for (size_t __i = 0; __i != __r.__services_.size(); ++__i) {
        if (__r.__services_[__i] == this) {
          __r.__services_.erase(__r.__services_.begin() + __i);
          break;
        }
      }
    }
    if (__source_added_)
      __reactor_.__remove_source(__pipe_fd_);
  }

  // Abandons the waits that are still pending (the coroutines are destroyed, not resumed).
  _LIBCPP_HIDE_FROM_ABI void shutdown() override;

  // The callback of the reactor when the signal pipe is readable: delivers the signals that arrived to all contexts.
  _LIBCPP_HIDE_FROM_ABI static void __pipe_ready() noexcept;
  _LIBCPP_HIDE_FROM_ABI static void __deliver(int __sig) noexcept;

  _LIBCPP_HIDE_FROM_ABI bool __start_wait(__signal_op& __op);
  _LIBCPP_HIDE_FROM_ABI void __cancel_op(__signal_op* __op) noexcept;
};

// P4100R1 [8.8] (Paper 9); provisional specification: Corosio's signal_set. Waits from a coroutine for one of a set of
// process signals (sigaction() plus a self-pipe underneath).
//
// Signal handling is process-global: a signal has one disposition for the whole process, however many signal sets (of
// however many contexts) register it. The first registration of a signal installs the handler, the last removal or
// destruction restores the disposition that was in effect before. All registrations of one signal must use the same
// flags (or dont_care): the first one that is not dont_care decides, and a later add with different flags fails with
// errc::invalid_argument. A signal that arrives while the set is registered for it but nobody waits is remembered (once
// per arrival) and delivered to the next wait. The signals are delivered by a thread that runs the context of the
// set; as long as a wait is pending run() does not return.
//
// A wait on a set must not overlap another wait on the same set (the second one completes with
// errc::operation_in_progress). The signal_set must outlive the awaitables and the context must outlive the set.
class signal_set {
public:
  // The flags of the registration. The values are abstract, not the SA_* values; they are mapped to sigaction() flags
  // (SA_RESTART, SA_NOCLDSTOP, SA_NOCLDWAIT, SA_NODEFER, SA_RESETHAND) so that this header needs no <signal.h> macros
  // and the interface can be used on platforms without sigaction. A flag that the platform lacks makes add() fail
  // with errc::operation_not_supported.
  enum flags_t : unsigned {
    none          = 0,
    restart       = 1u << 0, // SA_RESTART
    no_child_stop = 1u << 1, // SA_NOCLDSTOP
    no_child_wait = 1u << 2, // SA_NOCLDWAIT
    no_defer      = 1u << 3, // SA_NODEFER
    reset_handler = 1u << 4, // SA_RESETHAND
    dont_care     = 1u << 16 // compatible with any flags another set registered the signal with
  };

  _LIBCPP_HIDE_FROM_ABI friend constexpr flags_t operator|(flags_t __a, flags_t __b) noexcept {
    return static_cast<flags_t>(static_cast<unsigned>(__a) | static_cast<unsigned>(__b));
  }
  _LIBCPP_HIDE_FROM_ABI friend constexpr flags_t operator&(flags_t __a, flags_t __b) noexcept {
    return static_cast<flags_t>(static_cast<unsigned>(__a) & static_cast<unsigned>(__b));
  }
  _LIBCPP_HIDE_FROM_ABI friend constexpr flags_t& operator|=(flags_t& __a, flags_t __b) noexcept { return __a = __a | __b; }
  _LIBCPP_HIDE_FROM_ABI friend constexpr flags_t& operator&=(flags_t& __a, flags_t __b) noexcept { return __a = __a & __b; }
  _LIBCPP_HIDE_FROM_ABI friend constexpr flags_t operator~(flags_t __a) noexcept {
    return static_cast<flags_t>(~static_cast<unsigned>(__a));
  }

private:
  friend class __signal_service;

  __signal_service* __svc_;
  bool __reg_[__posix_max_signal]             = {};
  size_t __undelivered_[__posix_max_signal]  = {};
  __signal_op* __waiter_                      = nullptr;

  // Registers the signal of this set; precondition: the registry lock is held.
  _LIBCPP_HIDE_FROM_ABI error_code __ensure_pipe_locked() {
    if (__svc_->__source_added_)
      return error_code();
    int __fd;
    if (error_code __ec = __posix_signal_ensure_pipe(__fd))
      return __ec;
    if (error_code __ec = __svc_->__reactor_.__add_source(__fd, &__signal_service::__pipe_ready))
      return __ec;
    __svc_->__pipe_fd_      = __fd;
    __svc_->__source_added_ = true;
    return error_code();
  }

  _LIBCPP_HIDE_FROM_ABI error_code __remove_locked(__signal_registry& __r, int __sig) {
    if (!__reg_[__sig])
      return error_code();
    {
      lock_guard<mutex> __lk(__svc_->__m_);
      __reg_[__sig]         = false;
      __undelivered_[__sig] = 0;
    }
    if (--__r.__count_[__sig] == 0) {
      __r.__flags_[__sig] = 0;
      __r.__fixed_[__sig] = false;
      return __posix_signal_restore(__sig);
    }
    return error_code();
  }

public:
  class __wait_awaiter : public __signal_op {
  public:
    _LIBCPP_HIDE_FROM_ABI explicit __wait_awaiter(signal_set& __s) noexcept : __signal_op(__s.__svc_, &__s) {}
    _LIBCPP_HIDE_FROM_ABI __wait_awaiter(__wait_awaiter&& __o) noexcept : __signal_op(std::move(__o)) {}

    _LIBCPP_HIDE_FROM_ABI bool await_ready() const noexcept { return false; }

    _LIBCPP_HIDE_FROM_ABI bool await_suspend(coroutine_handle<> __h, const io_env* __env) {
      __bind(__h, __env);
      if (__env->stop_token.stop_requested()) {
        __ec_ = make_error_code(error::canceled);
        return false;
      }
      __arm_stop(__env);
      return __svc_->__start_wait(*this);
    }

    // The signal number; with an error (error::canceled, ...) the number is 0.
    _LIBCPP_HIDE_FROM_ABI io_result<int> await_resume() noexcept {
      __finish();
      return io_result<int>(__ec_, __ec_ ? 0 : __signo_);
    }
  };

  _LIBCPP_HIDE_FROM_ABI explicit signal_set(execution_context& __ctx)
      : __svc_(&__ctx.use_service<__signal_service>()) {
    lock_guard<mutex> __lk(__svc_->__m_);
    __svc_->__sets_.push_back(this);
  }

  // Throws system_error if a signal cannot be added.
  template <class... _Sigs>
    requires(is_convertible_v<_Sigs, int> && ...)
  _LIBCPP_HIDE_FROM_ABI signal_set(execution_context& __ctx, int __sig, _Sigs... __sigs) : signal_set(__ctx) {
    __add_or_throw(__sig);
    (__add_or_throw(__sigs), ...);
  }

  template <Executor _Ex>
  _LIBCPP_HIDE_FROM_ABI explicit signal_set(const _Ex& __ex) : signal_set(__ex.context()) {}

  template <Executor _Ex, class... _Sigs>
    requires(is_convertible_v<_Sigs, int> && ...)
  _LIBCPP_HIDE_FROM_ABI signal_set(const _Ex& __ex, int __sig, _Sigs... __sigs)
      : signal_set(__ex.context(), __sig, __sigs...) {}

  signal_set(const signal_set&)            = delete;
  signal_set& operator=(const signal_set&) = delete;

  // Restores the dispositions of the signals that no other set has registered and completes a pending wait with
  // error::canceled.
  _LIBCPP_HIDE_FROM_ABI ~signal_set() {
    (void)clear();
    cancel();
    lock_guard<mutex> __lk(__svc_->__m_);
    for (size_t __i = 0; __i != __svc_->__sets_.size(); ++__i) {
      if (__svc_->__sets_[__i] == this) {
        __svc_->__sets_.erase(__svc_->__sets_.begin() + __i);
        break;
      }
    }
  }

  // Adds the signal to the set (no effect if it is in the set already with these flags). Fails with
  // errc::invalid_argument for a signal number that is out of range, that cannot be caught (SIGKILL, SIGSTOP) or that
  // another set registered with different flags.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code add(int __sig, flags_t __flags = none) {
    if (__sig < 1 || __sig >= __posix_max_signal)
      return make_error_code(errc::invalid_argument);
    unsigned __f      = static_cast<unsigned>(__flags);
    bool __dont_care  = (__f & dont_care) != 0;
    if (__dont_care && __f != static_cast<unsigned>(dont_care))
      return make_error_code(errc::invalid_argument);
    if (!__dont_care) {
      if (__f & ~__sig_flag_all)
        return make_error_code(errc::invalid_argument);
      if (!__posix_flags_supported(__f))
        return make_error_code(errc::operation_not_supported);
    }
    __signal_registry& __r = __get_signal_registry();
    lock_guard<mutex> __lk(__r.__m_);
    if (error_code __ec = __ensure_pipe_locked())
      return __ec;
    unsigned __eff = __dont_care ? 0u : __f;
    if (__r.__count_[__sig] == 0) {
      if (error_code __ec = __posix_signal_install(__sig, __eff))
        return __ec;
      __r.__flags_[__sig] = __eff;
      __r.__fixed_[__sig] = !__dont_care;
    } else if (!__dont_care) {
      if (__r.__fixed_[__sig]) {
        if (__r.__flags_[__sig] != __eff)
          return make_error_code(errc::invalid_argument);
      } else { // every registration so far said dont_care: this one decides
        if (error_code __ec = __posix_signal_install(__sig, __eff))
          return __ec;
        __r.__flags_[__sig] = __eff;
        __r.__fixed_[__sig] = true;
      }
    }
    if (!__reg_[__sig]) {
      lock_guard<mutex> __lk2(__svc_->__m_);
      __reg_[__sig]         = true;
      __undelivered_[__sig] = 0;
      ++__r.__count_[__sig];
    }
    return error_code();
  }

  // Removes the signal from the set (no effect if it is not in it). When no set has the signal any more, its previous
  // disposition is restored.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code remove(int __sig) {
    if (__sig < 1 || __sig >= __posix_max_signal)
      return make_error_code(errc::invalid_argument);
    __signal_registry& __r = __get_signal_registry();
    lock_guard<mutex> __lk(__r.__m_);
    return __remove_locked(__r, __sig);
  }

  // Removes all signals; returns the first error of restoring a disposition (all signals are removed regardless).
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code clear() {
    error_code __first;
    __signal_registry& __r = __get_signal_registry();
    lock_guard<mutex> __lk(__r.__m_);
    for (int __sig = 1; __sig != __posix_max_signal; ++__sig) {
      error_code __ec = __remove_locked(__r, __sig);
      if (__ec && !__first)
        __first = __ec;
    }
    return __first;
  }

  // Completes a pending wait with error::canceled. The registered signals and the signals already remembered are kept.
  _LIBCPP_HIDE_FROM_ABI void cancel() noexcept {
    __signal_op* __op;
    {
      lock_guard<mutex> __lk(__svc_->__m_);
      __op = __waiter_;
      if (!__op)
        return;
      __waiter_      = nullptr;
      __op->__state_ = __op_base::__done;
      __op->__ec_    = make_error_code(error::canceled);
    }
    __svc_->__reactor_.__complete(__op);
  }

  // An IoAwaitable that completes with io_result<int>: the error_code and the number of the signal that arrived.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI __wait_awaiter wait() noexcept { return __wait_awaiter(*this); }

private:
  _LIBCPP_HIDE_FROM_ABI void __add_or_throw(int __sig) {
    if (error_code __ec = add(__sig))
      std::__throw_system_error(__ec.value(), "signal_set: cannot add the signal");
  }
};

inline void __signal_op::__cancel_cb(__op_base* __b) noexcept {
  __signal_op* __op = static_cast<__signal_op*>(__b);
  __op->__svc_->__cancel_op(__op);
}

inline bool __signal_service::__start_wait(__signal_op& __op) {
  lock_guard<mutex> __lk(__m_);
  signal_set& __s = *__op.__set_;
  __op.__state_   = __op_base::__done;
  if (__shut_down_ || __op.__cancel_requested_) {
    __op.__ec_ = make_error_code(error::canceled);
    return false;
  }
  if (__s.__waiter_) {
    __op.__ec_ = make_error_code(errc::operation_in_progress);
    return false;
  }
  for (int __sig = 1; __sig != __posix_max_signal; ++__sig) {
    if (__s.__undelivered_[__sig] != 0) {
      --__s.__undelivered_[__sig];
      __op.__signo_ = __sig;
      __op.__ec_    = error_code();
      return false;
    }
  }
  __s.__waiter_ = &__op;
  __op.__state_ = __op_base::__pending;
  __reactor_.__op_started();
  return true;
}

inline void __signal_service::__cancel_op(__signal_op* __op) noexcept {
  {
    lock_guard<mutex> __lk(__m_);
    if (__op->__state_ == __op_base::__idle) {
      __op->__cancel_requested_ = true;
      return;
    }
    if (__op->__state_ == __op_base::__done)
      return;
    __op->__set_->__waiter_ = nullptr;
    __op->__state_          = __op_base::__done;
    __op->__ec_             = make_error_code(error::canceled);
  }
  __reactor_.__complete(__op);
}

inline void __signal_service::shutdown() {
  vector<coroutine_handle<>> __handles;
  {
    lock_guard<mutex> __lk(__m_);
    __shut_down_ = true;
    for (signal_set* __s : __sets_) {
      if (__signal_op* __op = __s->__waiter_) {
        __s->__waiter_ = nullptr;
        __op->__state_ = __op_base::__done;
        __handles.push_back(__op->__cont_.h);
      }
    }
  }
  for (coroutine_handle<> __h : __handles)
    if (__h)
      __h.destroy();
}

inline void __signal_service::__deliver(int __sig) noexcept {
  vector<__signal_op*> __done;
  {
    __signal_registry& __r = __get_signal_registry();
    lock_guard<mutex> __lk(__r.__m_);
    for (__signal_service* __svc : __r.__services_) {
      lock_guard<mutex> __lk2(__svc->__m_);
      for (signal_set* __s : __svc->__sets_) {
        if (!__s->__reg_[__sig])
          continue;
        if (__signal_op* __op = __s->__waiter_) {
          __s->__waiter_ = nullptr;
          __op->__state_ = __op_base::__done;
          __op->__signo_ = __sig;
          __op->__ec_    = error_code();
          __done.push_back(__op);
        } else {
          ++__s->__undelivered_[__sig];
        }
      }
    }
  }
  for (__signal_op* __op : __done) {
    __reactor_service& __rs = __op->__svc_->__reactor_;
    __rs.__complete(__op);
  }
}

inline void __signal_service::__pipe_ready() noexcept {
  int __buf[64];
  while (int __n = __posix_signal_drain(__buf, 64))
    for (int __i = 0; __i != __n; ++__i)
      __deliver(__buf[__i]);
}

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_IO_POSIX_SIGNALS

#endif // _LIBCPP___IO_SIGNAL_SIGNAL_SET_H
